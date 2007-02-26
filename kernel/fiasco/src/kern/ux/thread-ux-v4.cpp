IMPLEMENTATION[ux-v4]:

/** Constructor.
    Creates a thread in an existing address space.
    @param space_specifier the thread specifying the address space
    @param id user-visible thread ID of the sender
    @param local_id local thread ID == UTCB pointer
    @param init_prio initial priority 
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread (Thread* space_specifier, L4_uid id, L4_uid local_id,
	        unsigned short init_prio, unsigned short mcp)
      : Receiver (&_thread_lock, space_specifier->space(),
	         init_prio, mcp, Config::default_time_slice, local_id),
        Sender         (id, 0),		// select optimized version of constructor
        _preemption    (id),
        _sched_timeout (this)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  _space = space_specifier->space();
  common_construct();

  // enqueue thread in present list after the space specifying thread
  present_enqueue (space_specifier);

  state_add(Thread_dead);
  
  _space->thread_add();

  // ok, we're ready to go!
}

/** Constructor.
    Creates a thread in a new address space.
    @param space the address space
    @param id user-visible thread ID of the sender
    @param local_id local thread ID == UTCB pointer
    @param init_prio initial priority 
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread (Space* space, L4_uid id, L4_uid local_id,
	        unsigned short init_prio, unsigned short mcp)
       : Receiver (&_thread_lock, space,
                  init_prio, mcp, Config::default_time_slice, local_id),
         Sender         (id, 0),	// select optimized version of constructor
         _preemption    (id),
         _sched_timeout (this)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  _space = space;
  common_construct();

  // enqueue thread after the creators' task
  present_enqueue
    ((Thread::lookup(thread_lock()->lock_owner()))->lookup_last_on_space());

  state_add(Thread_dead);

  _space->thread_add();
  
  // ok, we're ready to go!
}


/** Common thread constructor code.
 * Shared between the two v4 thread constructors.
 * @pre _space set to the thread's address space.
 */
PUBLIC inline NOEXPORT
void Thread::common_construct()
{
  _magic = magic;
  _irq = 0;
  _idt_limit = 0;
  _recover_jmpbuf = 0;
  _timeout = 0;

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
}

/**
 * Return to user.
 * This function is the default routine run if a newly initialized context
 * is being switch_to()'ed.
 */                   
PROTECTED static void Thread::user_invoke() {

  assert (current()->state() & Thread_running);

  asm volatile
    ("  movl %%eax,%%esp \n"    // set stack pointer to regs structure
     "  xorl %%ecx,%%ecx \n"     // clean out user regs
     "  xorl %%edx,%%edx \n"
     "  xorl %%esi,%%esi \n"
     "  xorl %%edi,%%edi \n" 
     "  xorl %%ebx,%%ebx \n"
     "  xorl %%ebp,%%ebp \n"
     "  xorl %%eax,%%eax \n"
     "  iret             \n"
     :                          // no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs()))
     );
}
