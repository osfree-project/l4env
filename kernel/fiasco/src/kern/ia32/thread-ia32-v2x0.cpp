IMPLEMENTATION[ia32-v2x0]:

/** Constructor.
    @param space the address space
    @param id user-visible thread ID of the sender
    @param init_prio initial priority 
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread (Space* space, L4_uid id,
	        unsigned short init_prio, unsigned short mcp)
      : Receiver (&_thread_lock, space,
                  init_prio, mcp, Config::default_time_slice),
        Sender         (id, 0),	// select optimized version of constructor
        _preemption    (id),
        _sched_timeout (this)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() != Thread_invalid)
    return;                     // Someone else was faster in initializing!
                                // That's perfectly OK.

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Config::thread_block_size-sizeof(Thread)-64);

  _magic = magic;
  _space = space;
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
