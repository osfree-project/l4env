INTERFACE:

EXTENSION class Thread
{
public:

  // Constructor
  Thread (Space* space, L4_uid id,
	  unsigned short init_prio, unsigned short mcp);

  void sys_id_nearest();
  void sys_thread_ex_regs();
};

IMPLEMENTATION[v2x0]:

#include "space_index_util.h"

/** Thread's space index.
    @return thread's space index.
 */
PUBLIC inline 
Space_index Thread::space_index() const // returns task number
{ return Space_index(id().task()); }

/** Chief's space index.
    @return thread's chief's space index.
 */
PUBLIC inline 
Space_index Thread::chief_index() const // returns chief number
{ return Space_index(id().chief()); }

/** Kill this thread.  If this thread has local number 0, kill its
    address space (task) as well.  This function does not handle
    recursion.
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

  if (id().lthread() == 0)
    {
      //
      // Flush our address space
      //

      // It might be faster to flush our address space before actually
      // deleting the subtasks: Under the assumption that child tasks
      // got many of their mappings from this task, flushing mappings
      // early will flush their mappings as well and speed up their
      // deletion.  However, this would yield increased uglyness
      // because subtasks could still be scheduled and do page faults.
      // XXX check /how/ ugly...

      fpage_unmap(space(), L4_fpage(0,0,L4_fpage::WHOLE_SPACE,0), true, false);

      //for small spaces...
      kill_small_space();

      // 
      //  Delete our address space
      // 
      delete space();
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

/** Kill a subtask of this thread's task.  
    This must be executed on thread 0 of the chief of the task to be killed.
    @param subtask a task number
    @return false if this thread does not exists or has been 
            killed already.  Otherwise, true.
    @pre id().id.lthread == 0 
         && Space_index_util::chief(subtask) == space_index()
 */
PUBLIC 
bool
Thread::kill_task(Space_index subtask)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() == Thread_invalid)
    return false;
  
  // "this" is thread 0 of the chief of the task to be killed.
  assert(id().lthread() == 0);
  assert(Space_index_util::chief(subtask) == space_index());

  // Traverse the tree of threads that need to be deleted.  Lock them
  // preorder and delete them post-order.

  // Only threads with local thread number 0 ("0-threads") are
  // considered to have children, and they are traversed in the
  // following order: First, all the other threads of the given
  // 0-thread's task need to be traversed (locked and killed;
  // according to this rule, they have no children) so that they
  // cannot create new subtasks.  Second, the 0-threads of the
  // subtasks need to be traversed.

  class kill_iter		// Iterator class -- also handles locking.
  {
  private:
    Thread* _top;
    Thread* _thread_0;
    Thread* _thread;

    static Thread* next_sibling (Thread* thread_0)
    {
      // Find threads belonging to the same task.  We assume here that
      // new threads of the same task are always enqueued /after/
      // thread 0.
      if (thread_0->present_next != thread_0
	  && thread_0->present_next->space() == thread_0->space())
	{
	  return thread_0->present_next;
	}

      return 0;
    }

    static Thread* next_child (Thread* parent)
    {
      // We assume here that subtasks of a task are always enqueued
      // /before/ thread 0 of a task.
      if (parent->present_prev != parent
	  && parent->present_prev->chief_index() == parent->space_index())
	{
	  return parent->present_prev;
	}

      return 0;
    }

    void advance_to_next()
    {
      if (_thread == _top)
	{
	  _thread = 0;
	  return;
	}

      for (;;)
	{
	  _thread = next_sibling (_thread_0);
	  if (_thread)
	    {
	      _thread->thread_lock()->lock();
	      return;
	    }
	  
	  _thread = next_child (_thread_0);
	  if (_thread)
	    {
	      _thread_0 = _thread;
	      _thread_0->thread_lock()->lock();
	      continue;
	    }

	  // _thread_0 has no siblings and no children.  Return it next.
	  _thread = _thread_0;

	  // Move up to parent: next candidate for returning.
	  if (_thread != _top)
	    _thread_0 = lookup_first_thread(_thread->chief_index());

	  return;
	}
    }

  public:
    kill_iter (Thread* top)
      : _top (top), _thread_0 (top), _thread (0)
    {
      _top->thread_lock()->lock();
      advance_to_next();
    }

    Thread* operator->() const 
    { return _thread; }

    operator Thread* () const
    { return _thread; }

    kill_iter& operator++ ()
    { 
      advance_to_next();
      return *this;
    }
  }; // class kill_iter
  
  // Back to the main proper of the function: Use the iterator we just
  // defined to kill the task.

  for (kill_iter next_to_kill (lookup_first_thread(subtask)); 
       next_to_kill;
       ++next_to_kill)
    {
      if (next_to_kill->kill())	// Turn in in locked state -- lock will be
        delete next_to_kill;	// cleared in destructor.

      else			// Someone else was faster, unlock
        next_to_kill->thread_lock()->clear();
    }

  return true;
}

IMPLEMENT inline
void
Thread::kill_all()
{
  kill_task (Space_index (Config::boot_id.task()));
}
