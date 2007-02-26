INTERFACE:

EXTENSION class Thread
{
public:

  // Constructors
  Thread (Thread* space_specifier, L4_uid id, L4_uid local_id,
	  unsigned short init_prio, unsigned short mcp);
  Thread (Space* space, L4_uid id, L4_uid local_id,
	  unsigned short init_prio, unsigned short mcp);
};

IMPLEMENTATION[v4]:

/** Kill this thread.  If this thread is the last one in its task, kill its
    address space (task) as well.
    @return false if thread does not exists or has already
            been killed.  Otherwise, true.
 */
PRIVATE
bool
Thread::kill()
{
  assert (current() == thread_lock()->lock_owner());

  if (state() == Thread_invalid)
    return false;

  assert(in_present_list());
  
  // If sending another request, finish it before killing this thread
  if (lock_cnt() > 0)
    {
      assert (state() & Thread_running);

      while(lock_cnt() > 0)
	{
	  current()->switch_to(this);

	  // Thread "this" will switch back to us ("current()") as
	  // soon as its lock count drops to 0.
	}
    }

  // De-register us from the thread counter.
  // If we're the last thread in our space:
  if (space()->thread_remove() == 0) {
    // Flush our address space
    fpage_unmap (space(), 
		 L4_fpage (0, 0, 0, L4_fpage::WHOLE_SPACE, 0), 
		 true, false);
    kill_small_space();		// for small spaces...
    delete space();		// Delete our address space
  }

  //
  // Kill this thread
  //

  // But first prevent it from being woken up by asynchronous events

  // if attached to irqs, detach
  if (_irq)
    _irq->free(this);

  // if timeout active, abort
  if (_timeout)
    _timeout->reset();

  state_change(0, Thread_dead);

  // if other threads are sending me requests, abort those

  // XXX we abolished the old request mechanism but didnt
  // come up with a solution what to do with pending 
  // requests after a thread has been killed
  //  for (request_t *s = _request_stack.dequeue(); 
  //       s; 
  //       s = _request_stack.dequeue())
  // {
  //      s->set_ret(request_t::Req_aborted);
  //    }

  // XXX what happens with requests that are enqueued from now on?

  // if other threads want to send me IPC messages, abort these
  // operations 

  // XXX that's quite difficult: how to traverse a list that may
  // change at all times?

  // if engaged in IPC operation, stop it
  if (in_sender_list())
    sender_dequeue(receiver()->sender_list());
  
  // dequeue from system queues
  assert(in_present_list());
  present_dequeue();

  if (in_ready_list())
    ready_dequeue();

  return true;
}

/** Find the last thread in present list that belongs to this thread's space.
 * @pre this thread in present list
 */
PRIVATE inline 
Thread* Thread::lookup_last_on_space()
{
  Thread *o = this;
  Thread *n;

  for (;;) {
    n = o->present_next;
    if (n->space() != o->space()) return o;	// task border found.
    if (n == this) return n;		// oh.. there's one space only
    o = n;
  }
}

IMPLEMENT inline
void
Thread::kill_all()
{
  Thread *o, *n;
  bool kill_s0 = false;
  bool tasks_alive = true;

  // Walk through the present list, blast everything that moves.
  // Make sure all non-sigma0 tasks are killed before sigma0, since
  // otherwise address space flush operations will fail.
  // Assumption: sigma0 doesn't create tasks.

  for (n = this;;) {
    o = n;
    n = o->present_next;
    
    if (n == this) {			// done one pass
      if (!tasks_alive) {
	if (kill_s0)			// done
	  return;
	kill_s0 = tasks_alive = true;	// start killing s0 threads
      }
      else 
	tasks_alive = false;
    }
      
    if ((o != this) && (kill_s0 || !o->space()->is_sigma0())) {
      tasks_alive = true;
      o->thread_lock()->lock();

      if (o->kill())			// Try to kill thread
        delete o;
      else				// Someone else was faster, unlock
        o->thread_lock()->clear();
    }
  }
}
