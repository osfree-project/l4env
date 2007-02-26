IMPLEMENTATION[ux-v2x0]:

IMPLEMENT
Thread::Thread (Space* space, L4_uid id,
                unsigned short init_prio, unsigned short mcp)
      : Receiver (&_thread_lock, space,
                  init_prio, mcp, Config::default_time_slice),
        Sender         (id, 0),
        _preemption    (id),
        _sched_timeout (this)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;			// Someone else was faster in initializing!
    				// That's perfectly OK.

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Config::thread_block_size-sizeof(Thread)-64);

  _magic          = magic;
  _space          = space;
  _irq            = 0;
  _idt_limit      = 0;
  _recover_jmpbuf = 0;
  _timeout        = 0;

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;

  // Allocate FPU state now because it indirectly calls current()
  // save_state runs on a signal stack and current() doesn't work there.
  Fpu_alloc::alloc_state(fpu_state());
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Return_frame *r = regs();
  r->esp = 0;
  r->eip = 0;
  r->cs  = Emulation::get_kernel_cs() & ~1;		// force iret trap
  r->ss  = Emulation::get_kernel_ss();

  if(Config::enable_io_protection)
    r->eflags = EFLAGS_IOPL_K | EFLAGS_IF | 2;
  else
    r->eflags = EFLAGS_IOPL_U | EFLAGS_IF | 2;     // XXX iopl=kernel

  _pager = _preempter = _ext_preempter = nil_thread;

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())->space_index()) {

    // same task -> enqueue after creator
    present_enqueue (Thread::lookup(thread_lock()->lock_owner()));

  } else { 

    // other task -> enqueue in front of this task
    present_enqueue (lookup_first_thread(Thread::lookup(thread_lock()
                   ->lock_owner())->space_index())->present_prev);
    // that's safe because thread 0 of a task is always present
  }

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

/**
 * Return to user.
 * This function is the default routine run if a newly initialized context
 * is being switch_to()'ed.
 */                   
PROTECTED static void Thread::user_invoke() {

  assert (current()->state() & Thread_running);

  asm volatile
    ("	cmpl $2    , %%edx      \n\t"     // Sigma0
     "	je   1f                 \n\t"
     "  cmpl $4    , %%edx      \n\t"     // Rmgr
     "  je   1f                 \n\t"
     "  xorl %%ebx , %%ebx      \n\t"     // don't pass info if other
     "  xorl %%ecx , %%ecx      \n\t"     // Sigma0 or Rmgr
     "1:                        \n\t" 
     "  movl %%eax , %%esp      \n\t"
     "  xorl %%edx , %%edx      \n\t"
     "  xorl %%esi , %%esi      \n\t"     // clean out user regs      
     "  xorl %%edi , %%edi      \n\t"                           
     "  xorl %%ebp , %%ebp      \n\t"
     "  xorl %%eax , %%eax      \n\t"
     "  iret                    \n\t"
     :                          	// no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs())),
       "b" (Kmem::virt_to_phys(Boot_info::mbi_virt())),
       "c" (Kmem::virt_to_phys(Kmem::info())),
       "d" ((unsigned) Thread::lookup(current())->id().task()));
}
