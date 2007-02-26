IMPLEMENTATION[ia32-v4]:

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
        Sender         (id, 0),	// select optimized version of constructor
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
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Return_frame *r = regs();
  r->esp = 0;
  r->eip = 0;
  if(Config::enable_io_protection)
    r->eflags = EFLAGS_IOPL_K | EFLAGS_IF | 2;   // ei
  else
    r->eflags = EFLAGS_IOPL_U | EFLAGS_IF | 2;     // XXX iopl=kernel
  r->cs = Kmem::gdt_code_user | SEL_PL_U;
  r->ss = Kmem::gdt_data_user | SEL_PL_U;

  // make sure the thread's kernel stack is mapped in its address space
  _space->kmem_update(this);
  
  _pager = _preempter = _ext_preempter = nil_thread;

}  

