
/*
 * Fiasco Thread Code
 * Shared between UX and native IA32.
 */
INTERFACE:

EXTENSION class Thread 
{
protected:
  x86_gate *_idt;
  unsigned short _idt_limit;

  static int (*nested_trap_handler)(trap_state *state);

};


IMPLEMENTATION[ia32-ux]:

#include <flux/x86/seg.h>
#include <flux/x86/base_trap.h>


#include "config.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "processor.h"
#include "regdefs.h"
#include "std_macros.h"
#include "thread.h"
#include "vmem_alloc.h"

int (*Thread::nested_trap_handler)(trap_state *state);

/** 
 * The global page fault handler switch.
 * Handles page-fault monitoring, classification of page faults based on
 * virtual-memory area they occured in, page-directory updates for kernel
 * faults, IPC-window updates, and invocation of paging function for
 * user-space page faults (handle_page_fault_pager).
 * @param pfa page-fault virtual address
 * @param error_code CPU error code
 * @return true if page fault could be resolved, false otherwise
 * @exception longjmp longjumps to recovery location if page-fault
 *                    handling fails (i.e., return value would be false),
 *                    but recovery location has been installed           
 */
PUBLIC inline NEEDS ["config.h", "cpu.h", "kdb_ke.h", "kmem.h", "vmem_alloc.h"]
bool 
Thread::handle_page_fault (Address pfa,
                           unsigned error_code,
                           unsigned eip)
{
  if (Config::monitor_page_faults)
    {
      if (_last_pf_address == pfa && _last_pf_error_code == error_code)
        {
          if (!log_page_fault)
            printf("*P[%x,%x,%x]\n", pfa, error_code & 0xffff, eip);
          else
            putchar('\n');

          kdb_ke("PF happened twice");
        }

      _last_pf_address = pfa;
      _last_pf_error_code = error_code;

      // (See also corresponding code in Thread::handle_ipc_page_fault()
      //                          and in Thread::handle_slow_trap.)
    }

  // TODO: put this into a debug_page_fault_handler
  if (EXPECT_FALSE (log_page_fault))
    page_fault_log (pfa, error_code, eip);

  L4_msgdope ipc_code(0);

  // Check for page fault in user memory area
  if (EXPECT_TRUE (Kmem::user_fault_addr (pfa, error_code)))
    {
      // Make sure that we do not handle page faults that do
      // belong to this thread.
      assert (space() == current_space());

      if (EXPECT_FALSE (space_index() == Config::sigma0_taskno))
        {
          // special case: sigma0 can map in anything from the kernel

          if (((Cpu::features() & FEAT_PSE)
               ? space()->v_insert((pfa & Config::SUPERPAGE_MASK),
                                   (pfa & Config::SUPERPAGE_MASK),
                                   Config::SUPERPAGE_SIZE, 
                                   Space::Page_writable    
                                   | Space::Page_user_accessible)
               : space()->v_insert((pfa & Config::PAGE_MASK),    
                                   (pfa & Config::PAGE_MASK),    
                                   Config::PAGE_SIZE, 
                                   Space::Page_writable
                                   | Space::Page_user_accessible))
              != Space::Insert_err_nomem)
            return true;

          ipc_code.error (L4_msgdope::REMAPFAILED);
          goto error;
        }

      // user mode page fault -- send pager request
      if (!(ipc_code = handle_page_fault_pager(pfa, error_code)).has_error())
        return true;

      goto error;
    }

  // Check for page fault in small address space
  else if (EXPECT_FALSE (Kmem::smas_fault_addr (pfa)))
    {
      if (handle_smas_page_fault (pfa, error_code, &ipc_code))
        return true;

      goto error;
    }
  
  // Check for page fault in kernel memory region caused by user mode
  else if (EXPECT_FALSE ((error_code & PF_ERR_USERMODE)))
    return false;             // disallow access after mem_user_max

  // Check for page fault in IO bit map or in delimiter byte behind IO bitmap
  // assume it is caused by an input/output instruction and fall through to
  // handle_slow_trap
  else if (EXPECT_FALSE (Kmem::iobm_fault_addr (pfa)))
    return false;

  // We're in kernel code faulting on a kernel memory region

  // Check for page fault in IPC window. Mappings for this should never
  // be present in the global master page dir (case below), because the
  // IPC window mappings are always flushed on context switch.
  else if (EXPECT_TRUE (Kmem::ipcw_fault_addr (pfa, error_code)))
    {
      if (!(ipc_code = handle_ipc_page_fault(pfa)).has_error())
        return true;

      goto error;
    }

  // A page is not present but a mapping exists in the global page dir.
  // Update our page directory by copying from the master pdir
  // This is the only path that should be executed with interrupts
  // disabled if the page faulter also had interrupts disabled.   
  // thread_page_fault() takes care of that.
  else if ((!(error_code & PF_ERR_PRESENT)) &&
           Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != 0xffffffff)
    {
      current_space()->kmem_update(pfa);
      return true;
    }

  // Check for page fault in kernel's TCB area
  else if (Kmem::tcbs_fault_addr (pfa))
    {
      if (!(error_code & PF_ERR_PRESENT))   // page not present
        {
          // in case of read fault, just map in the shared zero page
          // otherwise -> allocate
          if (!Vmem_alloc::page_alloc((void*)(pfa & Config::PAGE_MASK), 0,
                                      (error_code & PF_ERR_WRITE) ?
                                            Vmem_alloc::ZERO_FILL :
                                            Vmem_alloc::ZERO_MAP)) 
            panic("can't alloc kernel page");
        }
      else
        { 
          // protection fault
          // this can only be because we have the zero page mapped
          Vmem_alloc::page_free(reinterpret_cast<void*>(pfa & Config::PAGE_MASK), 0);
          if (! Vmem_alloc::page_alloc((void*)(pfa & Config::PAGE_MASK), 0,  
                                       Vmem_alloc::ZERO_FILL))
            {
              // error could mean: someone else was faster allocating
              // a page there, or we just don't have any pages left; verify
              if (Kmem::virt_to_phys(reinterpret_cast<void*>(pfa)) 
                  == 0xffffffff)
                panic("can't alloc kernel page");

              // otherwise, there's a page mapped.  continue
            }
        }

      current_space()->kmem_update(pfa);
      return true;
    }

     
  printf ("KERNEL: no page fault handler for 0x%x, error 0x%x, eip %08x\n",
          pfa, error_code, eip);

  // An error occurred.  Our last chance to recover is an exception
  // handler a kernel function may have set.
 error:

  if (_recover_jmpbuf)
    longjmp (*_recover_jmpbuf, ipc_code.raw());

  return false;
}

/**
 * The low-level page fault handler called from entry.S.  We're invoked with
 * interrupts turned off.  Apart from turning on interrupts in almost
 * all cases (except for kernel page faults in TCB area), just forwards
 * the call to Thread::handle_page_fault().
 * @param pfa page-fault virtual address
 * @param error_code CPU error code
 * @return true if page fault could be resolved, false otherwise                      
 */
extern "C"
int
thread_page_fault (      Mword * const esp,
		   const Mword pfa,
                   const Mword error_code,
                   const Mword                /* edx */,
                   const Mword                /* ecx */,
                   const Mword                /* eax */,
                   const Mword                /* ebp (was error) */,
                   const Mword eip,
                   const Mword                /* xcs */,
                   const Mword eflags)
{
  extern unsigned gdb_trap_recover;		// in OSKit's gdb_trap.c

  // If we're in the GDB stub -- let generic handler handle it
  if (EXPECT_FALSE( gdb_trap_recover ))
    return false;

  // Pagefault in user mode or interrupts were enabled
  if ((error_code & PF_ERR_USERMODE) || (eflags & EFLAGS_IF))
    Proc::sti();

  // Pagefault in kernel mode and interrupts were disabled
  else {

    // page fault in kernel memory region, not present, but mapping exists
    if (!Kmem::user_fault_addr (pfa, error_code) &&
        !(error_code & PF_ERR_PRESENT) &&
        Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != 0xffffffff) {

      // We've interrupted a context in the kernel with disabled interrupts,
      // the page fault address is in the kernel region, the error code is
      // "not mapped" (as opposed to "access error"), and the region is
      // actually valid (that is, mapped in Kmem's shared page directory,
      // just not in the currently active page directory)
      // Remain cli'd !!!

      // Test for special case
      if (Kmem::pagein_tcb_request(eip))
	{
	  // skip faulting instruction
	  esp[6] += 2;		// eip
	  // tell program that a pagefault occured we cannot handle
	  esp[4] = 0xffffffff;	// eax

	  return true;
	}

    } else if (!Config::conservative &&
               Kmem::user_fault_addr (pfa, error_code)) {
      
      // No error -- just enable interrupts.
      Proc::sti();

    } else {

      // Error: We interrupted a cli'd kernel context touching kernel space
      if (!Thread::log_page_fault)
        printf("*P[%x,%x,%x] ", pfa, error_code & 0xffff, eip);

      kdb_ke ("page fault in cli mode");
    }
  }

  return current_thread()->handle_page_fault (pfa, error_code, eip);
}


/** Raise a machine exception for the task.
    This function modifies a thread's user stack pointer and user instruction 
    pointer to emulate a machine exception.
    @param ts trap state that is being posted
    @param handler pointer to user-level handler function the thread has 
                   installed
    @return false if thread is not running or has been 
            reinititialized using initialize().  Otherwise true.
 */
PUBLIC inline NEEDS [<flux/x86/base_trap.h>]
bool
Thread::raise_exception(trap_state *ts, vm_offset_t handler)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (! (state() & Thread_running)
      || (state() & Thread_cancel))
    return false;

  ts->eip = handler;
  ts->esp -= ((ts->trapno >= 0x0a && ts->trapno <= 0x0e)
	      || ts->trapno == 0x08
	      || ts->trapno == 0x11) // need to push error code?
    ? 4 * 4
    : 3 * 4;

  return true;
}

/** The catch-all trap entry point.  Called by assembly code when a 
    CPU trap (that's not specially handled, such as system calls) occurs.
    Just forwards the call to Thread::handle_slow_trap().
    @param state trap state
    @return 0 if trap has been consumed by handler;
           -1 if trap could not be handled.
 */
extern "C" int
thread_handle_trap(trap_state *state)
{
  return current_thread()->handle_slow_trap(state);
}

// We are entering with disabled interrupts!
extern "C" void
thread_timer_interrupt(void)
{

  thread_timer_interrupt_arch();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif

  // update clock and handle timeouts
  timer::update_system_clock();

  // unlock; also, re-enable interrupts
  cpu_lock.clear();

  // need to reschedule?
  Context::timer_tick();
}

// 
// Public services
// 

/** (Re-) Ininialize a thread and make it running.
    This call also cancels IPC.
    @param eip new user instruction pointer.  Set only if != 0xffffffff.
    @param esp new user stack pointer.  Set only if != 0xffffffff.
    @param o_eip return current instruction pointer if pointer != 0
    @param o_esp return current stack pointer if pointer != 0
    @param o_pager return current pager if pointer != 0
    @param o_preempter return current internal preempter if pointer != 0
    @param o_eflags return current eflags register if pointer != 0
    @return false if !exists(); true otherwise
 */
PUBLIC
bool
Thread::initialize(vm_offset_t eip, vm_offset_t esp,
		     Thread* pager, Thread* preempter,
		     vm_offset_t *o_eip = 0, 
		     vm_offset_t *o_esp = 0,
		     Thread* *o_pager = 0, 
		     Thread* *o_preempter = 0,
		     vm_offset_t *o_eflags = 0)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() == Thread_invalid)
    return false;

  Return_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_preempter) *o_preempter = _preempter;
  if (o_esp) *o_esp = r->esp;
  if (o_eip) *o_eip = r->eip;
  if (o_eflags) *o_eflags = r->eflags;

  if (eip != 0xffffffff)
    {
      r->eip = eip;
      if (r->cs & 0x80)
	{
	  // this cannot happen in Fiasco UX
	  extern Mword leave_from_sysenter_by_iret;
	  Mword **ret_from_disp_syscall = reinterpret_cast<Mword**>(regs())-4;
	  r->cs &= ~0x80;
	  *ret_from_disp_syscall = &leave_from_sysenter_by_iret;
	}
      if (! (state() & Thread_dead))
	{
#if 0
	  kdb_ke("reseting non-dead thread");
#endif
	  // cancel ongoing IPC or other activity
	  state_change(~Thread_ipc_in_progress, Thread_cancel);
	}
    }

  if (pager != 0) _pager = pager;
  if (preempter != 0) _preempter = preempter;
  if (esp != 0xffffffff) r->esp = esp;
  
  state_change(~Thread_dead, Thread_running);

  return true;
}
