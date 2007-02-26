
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
#include "timeout.h"
#include "vmem_alloc.h"

int (*Thread::nested_trap_handler)(trap_state *state);


/** Check if the pagefault occured at a special place: At some places in the
    kernel we want to ensure that a specific address is mapped. The regular
    case is "mapped", the exception or slow case is "not mapped". The fastest
    way to check this is to touch into the memory. If there is no mapping for
    the address we get a pagefault. Now the pagefault exception handler can
    recognize that situation by scanning the code. The trick is that the
    assembler instruction "andl $0xffffffff, (%ecx)" _clears_ the carry flag
    normally (see Intel reference manual). The pager wants to inform the
    code that there was a pagefault and therefore _sets_ the carry flag. So
    the code has only to check if the carry flag is set. If yes, there was
    a pagefault at this instruction.
    @param eip pagefault address */
PUBLIC inline NOEXPORT
int
Thread::pagein_tcb_request (Address eip)
{
  return (   *(Unsigned8*) eip    == 0x83
 	  && *(Unsigned8*)(eip+1) == 0x21
	  && *(Unsigned8*)(eip+2) == 0xff);
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
  else 
    {
      // page fault in kernel memory region, not present, but mapping exists
      if (   Kmem::is_kmem_page_fault( pfa, error_code ) 
	  && !(error_code & PF_ERR_PRESENT))
	{
	  // We've interrupted a context in the kernel with disabled interrupts,
	  // the page fault address is in the kernel region, the error code is
	  // "not mapped" (as opposed to "access error"), and the region is
	  // actually valid (that is, mapped in Kmem's shared page directory,
	  // just not in the currently active page directory)
	  // Remain cli'd !!!

	  // Test for special case -- see function documentation
	  if (current_thread()->pagein_tcb_request(eip))
	    {
	      // skip faulting instruction
	      esp[6] += 3;		// eip
	      // tell program that a pagefault occured we cannot handle
	      esp[8] |= 1;		// set carry flag in EFLAGS

	      return true;
	    }
	} 
      else if (!Config::conservative &&
	       !Kmem::is_kmem_page_fault (pfa, error_code)) 
	{
      	  // No error -- just enable interrupts.
	  Proc::sti();
	} 
      else 
	{
       	  // Error: We interrupted a cli'd kernel context touching kernel space
	  if (!Thread::log_page_fault())
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
Thread::raise_exception(trap_state *ts, Address handler)
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
extern "C"
void
thread_timer_interrupt (void)
{
  thread_timer_interrupt_arch();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif

  Thread::handle_timer_interrupt();
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
Thread::initialize(Address eip, Address esp,
		   Thread* pager, Thread* preempter,
		   Address *o_eip = 0, Address *o_esp = 0,
		   Thread* *o_pager = 0, Thread* *o_preempter = 0,
		   Address *o_eflags = 0)
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
	  Mword **ret_from_disp_syscall = reinterpret_cast<Mword**>(regs())-1;
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

IMPLEMENT inline
bool Thread::handle_sigma0_page_fault( Address pfa ) 
{
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

  return false;
}


IMPLEMENT inline NEEDS ["config.h", "space_context.h", "std_macros.h"]
Mword
Thread::update_ipc_window (Address pfa, Address remote_pfa, Mword error_code)
{
  Space_context *remote = receiver()->space_context();
  bool writable;
  
  // If the remote address space has a mapping for the page fault address and
  // it is writable or we didn't want to write to it, then we can simply copy
  // the mapping into our address space via space()->remote_update().
  // Otherwise return 0 to trigger a pagein_request upstream.
  
  if (EXPECT_TRUE (remote->lookup (remote_pfa, &writable) != (Address) -1 &&
                  (writable || !(error_code & PF_ERR_WRITE))))
    {
      //careful: for SMAS current_space() != space()
      current_space()->remote_update (pfa, remote, remote_pfa, 1);

      // It's OK if the PF occurs again: This can happen if we're
      // preempted after the call to remote_update() above.  (This code
      // corresponds to code in Thread::handle_page_fault() that
      // checks for double page faults.)

      if (Config::monitor_page_faults)
        _last_pf_address = (Address) -1;

      return 1;
    }

  return 0;
}
