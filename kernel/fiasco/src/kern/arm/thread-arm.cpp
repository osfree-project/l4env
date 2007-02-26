IMPLEMENTATION[arm]:

#include <cassert>
#include <cstdio>

#include "globals.h"
#include "thread_state.h"
#include "vmem_alloc.h"


enum {
  FSR_STATUS_MASK = 0x0d,
  FSR_TRANSL      = 0x05,
  FSR_DOMAIN      = 0x09,
  FSR_PERMISSION  = 0x0d,


};



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
Thread::initialize(Address ip, Address sp,
		   Thread* pager, Thread* preempter,
		   Address *o_ip = 0, 
		   Address *o_sp = 0,
		   Thread* *o_pager = 0, 
		   Thread* *o_preempter = 0,
		   Address *o_eflags = 0)
{

  (void)o_eflags;
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() == Thread_invalid)
    return false;

  Entry_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_preempter) *o_preempter = _preempter;
  if (o_sp) *o_sp = r->sp();
  if (o_ip) *o_ip = r->pc();
  //if (o_eflags) *o_eflags = r->eflags;

  if (ip != 0xffffffff)
    {
      r->pc( ip );
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
  if (sp != 0xffffffff) r->sp( sp );
  
  state_change(~Thread_dead, Thread_running);

  return true;
}



/** Return to user.  This function is the default routine run if a newly  
    initialized context is being switch_to()'ed.
 */
PROTECTED static 
void Thread::user_invoke() 
{
//   while (! (current()->state() & Thread_running))
//     current()->schedule();
  assert (current()->state() & Thread_running);
  printf("user_invoke of %p @ %08x\n",current(),nonull_static_cast<Return_frame*>(current()->regs())->pc );

  asm volatile
    ("  mov sp, %[stack_p] \n"    // set stack pointer to regs structure
     // TODO clean out user regs
     "  ldr lr, [sp], #4   \n"
     "  msr spsr, lr       \n"
     "  ldmia sp!, {pc}^   \n"
     :  
     : 
     [stack_p] "r" (nonull_static_cast<Return_frame*>(current()->regs()))
     );

  puts("should never be reached");
  while(1) {
    current()->state_del(Thread_running);
    current()->schedule();
  };

  // never returns here
}

/** Constructor.
    @param space the address space
    @param id user-visible thread ID of the sender
    @param init_prio initial priority 
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread(Space* space,
	       L4_uid id,
	       int init_prio, unsigned short mcp)
  : Receiver (&_thread_lock, space), 
    Sender (id, 0)           // select optimized version of constructor
{
  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;

  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;

  _space = space;
  _irq = 0;
  _recover_jmpbuf = 0;
  _timeout = 0;

  sched()->set_prio (init_prio);
  sched()->set_mcp (mcp);
  sched()->set_timeslice (Config::default_time_slice);
  sched()->set_ticks_left (Config::default_time_slice);
  sched()->reset_cputime ();
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->pc(0);

#warning _space->kmem_update(reinterpret_cast<Address>(this)); unimplemented
 // make sure the thread's kernel stack is mapped in its address space
  //  _space->kmem_update(reinterpret_cast<Address>(this));
  
  _pager = _preempter = _ext_preempter = nil_thread;

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())
                       ->space_index())
    {
      // same task -> enqueue after creator
      present_enqueue(Thread::lookup(thread_lock()->lock_owner()));
    }
  else
    { 
      // other task -> enqueue in front of this task
      present_enqueue(lookup_first_thread(Thread::lookup
					  (thread_lock()->lock_owner())
					  ->space_index())
                      ->present_prev);
      // that's safe because thread 0 of a task is always present
    }

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}


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
PUBLIC /*inline NEEDS ["config.h", "kdb_ke.h", "kmem.h", "vmem_alloc.h"]*/
bool Thread::handle_page_fault (Address pfa,
				Mword error_code,
				Mword eip)
{
  /* Only by reading the instruction and checking for the L-bit (20)
     we can find out whether we had a read or a write pagefault.  
     Inside the kernel we'll never have instruction fetch faults. */
  Mword read_fault = *((Mword*)eip) & (1 << 20);

  printf("Pagefault: @%08x, (%08x), ip=%08x\n", pfa, error_code,eip);
  if (Config::monitor_page_faults)
    {
      if (_last_pf_address == pfa && _last_pf_error_code == error_code)
        {
	  printf("P[%x,%x,%x]\n", pfa, error_code & 0xffff, eip);
          kdb_ke("PF happened twice");
        }

      _last_pf_address = pfa;
      _last_pf_error_code = error_code;

      // (See also corresponding code in Thread::handle_ipc_page_fault()
      //                          and in Thread::handle_slow_trap.)
    }

  L4_msgdope ipc_code(0);

  // Check for page fault in user memory area
  if (EXPECT_TRUE (Kmem::mem_user_max > pfa))
    {
      // Make sure that we do not handle page faults that do
      // belong to this thread.
      assert (space() == current_space());

      if (EXPECT_FALSE (space_index() == Config::sigma0_taskno))
        {
          // special case: sigma0 can map in anything from the kernel

          if (space()->v_insert((pfa & Config::SUPERPAGE_MASK),
				(pfa & Config::SUPERPAGE_MASK),
				Config::SUPERPAGE_SIZE, 
				Space::Page_writable    
				| Space::Page_user_accessible)
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
#warning HAVE TO Check for page fault in kernel memory region caused by user mode
#if 0
  else if (EXPECT_FALSE ((error_code & PF_ERR_USERMODE)))
    return false;             // disallow access after mem_user_max

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
#endif

  // Check for page fault in kernel's TCB area
  else if ((pfa >=Kmem::mem_tcbs) 
	   && (pfa <Kmem::mem_tcbs_end))
    {
      if ((error_code & FSR_STATUS_MASK)!=FSR_PERMISSION)   // page not present
        {
          // in case of read fault, just map in the shared zero page
          // otherwise -> allocate
	  printf("Allocate TCB: read_fault: %s\n", read_fault 
		 ? "yes (zero map)" 
		 : "no (alloc + zero fill)" );
 
          if (!Vmem_alloc::page_alloc((void*)(pfa & Config::PAGE_MASK), 0,
                                      read_fault 
				      ? Vmem_alloc::ZERO_MAP 
				      : Vmem_alloc::ZERO_FILL)) 
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
                  == (void*)0xffffffff)
                panic("can't alloc kernel page");

              // otherwise, there's a page mapped.  continue
            }
        }
#warning      current_space()->kmem_update(pfa);
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


#if 0

extern "C" Mword thread_page_fault(Mword pc, Mword psr)
{
  Thread *t = current_thread();

  // interrupts are turned off
  t->state_change_dirty(~Thread_cancel, 0);

  // Pagefault in user mode or interrupts were enabled
  //  if ((error_code & PF_ERR_USERMODE) || (eflags & EFLAGS_IF))
  //    Proc::sti();

  // Pagefault in kernel mode and interrupts were disabled
  //  else {

    // page fault in kernel memory region, not present, but mapping exists
  if ((pfa >= Kmem::mem_user_max)
        /*!(error_code & PF_ERR_PRESENT) &&*/
        /*Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != 0xffffffff*/) {

      // We've interrupted a context in the kernel with disabled interrupts,
      // the page fault address is in the kernel region, the error code is
      // "not mapped" (as opposed to "access error"), and the region is
      // actually valid (that is, mapped in Kmem's shared page directory,
      // just not in the currently active page directory)
      // Remain cli'd !!!

    } else if (!Config::conservative &&
               (pfa < Kmem::mem_user_max)) {
      
      // No error -- just enable interrupts.
      Proc::sti();

    } else {

      // Error: We interrupted a cli'd kernel context touching kernel space
      printf("PF[pfa=0x%x,pc=0x%x] ", pfa, pc);

      kdb_ke ("page fault in cli mode");
    }
	       //}

  return t->handle_page_fault (pfa, error, pc);

};
#endif

extern "C" {
  void user_pagefault() {}

  void kernel_pagefault( Mword pfa, Mword error_code, Mword pc ) 
  {
    puts("kernel_pagefault");
    Thread *t = current_thread();
    Mword read_fault = *((Mword*)pc) & (1 << 20);

        // page fault in kernel memory region, not present, but mapping exists
#if 0
    if ((pfa >= Kmem::mem_user_max)
	/*!(error_code & PF_ERR_PRESENT) &&*/
	/*Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != 0xffffffff*/) 
      {
      
      // We've interrupted a context in the kernel with disabled interrupts,
      // the page fault address is in the kernel region, the error code is
      // "not mapped" (as opposed to "access error"), and the region is
      // actually valid (that is, mapped in Kmem's shared page directory,
      // just not in the currently active page directory)
      // Remain cli'd !!!

      } 
  else 
#endif
    if (!Config::conservative &&
	(pfa < Kmem::mem_user_max)) 
      {
	
	// No error -- just enable interrupts.
	Proc::sti();
	t->handle_page_fault (pfa, error_code, pc);
	return;
	
      } 
    else if ((pfa >=Kmem::mem_tcbs) 
	     && (pfa <Kmem::mem_tcbs_end))
      {
	if ((error_code & FSR_STATUS_MASK)!=FSR_PERMISSION)   // page not present
	  {
	    // in case of read fault, just map in the shared zero page
	    // otherwise -> allocate
	    printf("Allocate TCB: read_fault: %s\n", read_fault 
		   ? "yes (zero map)" 
		   : "no (alloc + zero fill)" );
	    
	    if (!Vmem_alloc::page_alloc((void*)(pfa & Config::PAGE_MASK), 0,
					read_fault 
					? Vmem_alloc::ZERO_MAP 
					: Vmem_alloc::ZERO_FILL)) 
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
		    == (void*)0xffffffff)
		  panic("can't alloc kernel page");
		
		// otherwise, there's a page mapped.  continue
	      }
	  }
#warning      current_space()->kmem_update(pfa);
	
      } 
    else 
      {
	// Error: We interrupted a cli'd kernel context touching kernel space
	printf("PF[pfa=0x%x,pc=0x%x] ", pfa, pc);
	
	kdb_ke ("page fault in cli mode");
      }
    
  }
  void irq_handler() {}
};
