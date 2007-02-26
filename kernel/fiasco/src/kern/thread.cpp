INTERFACE:

#include <setjmp.h>             // typedef jmp_buf

#include "l4_types.h"

#include "config.h"
#include "threadid.h"
#include "space.h"		// Space_index
#include "sender.h"
#include "receiver.h"
#include "thread_lock.h"

class timeout_t;
class Irq_alloc;
class Return_frame;
class Syscall_frame;


/** A thread.  This class is the driver class for most kernel functionality.
 */
class Thread : public Receiver, public Sender
{

  friend class Jdb;
  friend class jdb_thread_list;

public:
  bool handle_smas_page_fault(Address pfa, unsigned error_code,
			      L4_msgdope *ipc_code);

  template < typename T > 
  void copy_from_user( void *kdst, void const *usrc, size_t n );
  template < typename T > 
  void copy_to_user  ( void *udst, void const *ksrc, size_t n );
  template < typename T > 
  T peek_user( T const *addr );
  template < typename T > 
  void poke_user( T *addr, T value );

private:
  Thread(const Thread&);	///< Default copy constructor is undefined
  void *operator new(size_t);	///< Default new operator undefined

protected:
  // implementation details follow...

  // DATA
  /* offset */
  // Another critical TCB cache line:
  /*    */   Space*       _space;
  /* 20 */   Thread_lock  _thread_lock;

  // More ipc state
  Thread *_pager, *_preempter, *_ext_preempter;
  
  Thread *present_next, *present_prev;
  Irq_alloc *_irq;
  

  // long ipc state
  L4_rcv_desc _target_desc;	// ipc buffer in receiver's address space
  unsigned _pagein_error_code;

  vm_offset_t _vm_window0, _vm_window1; // data windows for the
  				// IPC partner's address space (for long IPC)
  jmp_buf *_recover_jmpbuf;	// setjmp buffer for page-fault recovery
  L4_timeout _pf_timeout;	// page-fault timeout specified by partner

  // debugging stuff
  vm_offset_t _last_pf_address;
  unsigned _last_pf_error_code;

  unsigned _magic;
  static const unsigned magic = 0xf001c001;


  // Constructor
  Thread (Space* space, L4_uid id);

public:

  // Constructor
  Thread (Space* space, L4_uid id,
	  int init_prio, unsigned short mcp);

  Mword sys_ipc		        (Syscall_frame *regs);
  Mword sys_id_nearest	        (Syscall_frame *regs);
  Mword sys_fpage_unmap	        (Syscall_frame *regs);
  Mword sys_thread_switch	(Syscall_frame *regs);
  Mword sys_thread_schedule	(Syscall_frame *regs);
  Mword sys_thread_ex_regs	(Syscall_frame *regs);
  Mword sys_task_new		(Syscall_frame *regs);

private:
  void kill_small_space (void);
  Mword small_space(void);
  void set_small_space(Mword nr);
};

IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>		// panic()


#include "entry_frame.h"
#include "thread_state.h"
#include "thread_util.h"
#include "space_index_util.h"
#include "irq_alloc.h"
#include "timer.h"
#include "atomic.h"
#include "fpu_alloc.h"
#include "globals.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "kdb_ke.h"
#include "map_util.h"
#include "logdefs.h"


/** Class-specific allocator.
    This allocator ensures that threads are allocated at a fixed virtual
    address computed from their thread ID.
    @param id thread ID
    @return address of new thread control block
 */
PUBLIC inline 
void *
Thread::operator new(size_t, threadid_t id)
{
  // Allocate TCB in TCB space.  Actually, do not allocate anything,
  // just return the address.  Allocation happens on the fly in
  // Thread::handle_page_fault().
  return static_cast<void*>(id.lookup());
}

/** Deallocator.  This function currently does nothing: We do not free up
    space allocated to thread-control blocks.
 */
PUBLIC 
void 
Thread::operator delete(void *)
{
  // XXX should check if all thread blocks on a given page are free
  // and deallocate (or mark free) the page if so.  this should be
  // easy to detect since normally all threads of a given task are
  // destroyed at once at task deletion time
}

/** Cut-down version of Thread constructor.  Do only what's necessary
    to get the bootstrap thread started -- skip all fancy stuff, no locking
    is necessary.
    @param space the address space
    @param id user-visible thread ID of the sender
 */
IMPLEMENT
Thread::Thread (Space* space, L4_uid id)
  : Receiver (&_thread_lock, space),
    Sender (id)
{
  _magic = magic;
  _space = space;

  *reinterpret_cast<void(**)()> (--kernel_sp) = user_invoke;
}

/** Destructor.  Reestablish the Context constructor's precondition.
    @pre current() == thread_lock()->lock_owner()
         && state() == Thread_dead
    @pre lock_cnt() == 0
    @post (kernel_sp == 0)  &&  (* (stack end) == 0)  &&  !exists()
 */
PUBLIC virtual
Thread::~Thread()		// To be called in locked state.
{
  assert(current() == thread_lock()->lock_owner());
  assert(state() == Thread_dead);
  assert(_magic == magic);

  unsigned long *init_sp = reinterpret_cast<unsigned long*>
    (reinterpret_cast<unsigned long>(this) + size - sizeof(Entry_frame));

  kernel_sp = 0;
  *--init_sp = 0;
  state_change (0, Thread_invalid);
  Fpu_alloc::free_state(fpu_state());

  thread_lock()->clear();

  // NOTE: It is possible that current() (this's locker) is deleted
  // right now (after it has cleared the lock), prior to invoking
  // Context's destructor.  Make sure all superclass destructors are
  // empty!  They might never be called!
}

/** Lookup function: Find Thread instance that owns a given Context. 
    @param c a context
    @return the thread that owns the context
 */
PUBLIC static inline
Thread*
Thread::lookup (Context* c)
{
  return reinterpret_cast<Thread*>(c);
}

/** Currently executing thread.
    @return currently executing thread.
 */
inline
Thread*
current_thread ()
{
  return Thread::lookup(current ());
}

//
// state requests/manipulation
//

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


/** Address space.
    @return pointer to thread's address space.
 */
PUBLIC inline 
Space *
Thread::space() const
{ 
  return _space; 
}

/** Thread lock.
    Overwrite Context's version of thread_lock() with a semantically
    equivalent, but more efficient version.
    @return lock used to synchronized accesses to thread.
 */
PUBLIC inline 
Thread_lock *
Thread::thread_lock()
{ 
  return &_thread_lock; 
}




/** Modify scheduling parameters.
    @param param new scheduling parameters.  Set only if
                 param.sched_param != 0xffffffff
    @param ext_preempter new external preempter.  Set only if != 0.
    @param o_param returns old scheduling parameters
    @param time returns current time consumption
    @param o_ext_preempter returns current external preempter
    @param ipcpartner return current IPC partner if there is one, 0 otherwise
    @return false if !exists() or maximum controlled priority
            of caller is not high enough
    @pre o_param != 0 && o_ext_preempter != 0 && ipcpartner != 0 && time != 0
 */
PUBLIC 
bool
Thread::set_sched_param(L4_sched_param param, Thread* ext_preempter, 
			L4_sched_param *o_param, Unsigned64 *time,
			Thread **o_ext_preempter, Sender **ipcpartner)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if (state() == Thread_invalid)
    return false;

  // call legal?
  if (sched()->prio() > thread_lock()->lock_owner()->sched()->mcp())
    return false;

  // query current state
  unsigned s = state();

  if (s & (Thread_polling | Thread_receiving))
    *ipcpartner = partner();
  else
    *ipcpartner = 0;

  if (s & Thread_dead) s = 0xf;
  else if (s & Thread_polling) s = (s & Thread_running) ? 8 : 0xd;
  else if (s & (Thread_waiting|Thread_receiving)) 
    s = (s & Thread_running) ? 8 : 0xc;
  else s = 0;

  o_param->error(s);

  Unsigned64 timeslice = sched()->timeslice() * Config::microsec_per_tick;

  o_param->time(timeslice);
  o_param->prio(sched()->prio());
  o_param->small(small_space()); 

  *o_ext_preempter = _ext_preempter;

  *time = sched()->get_total_cputime();

  // set new state
  if (param.is_valid())
    {
      if (param.prio() <= thread_lock()->lock_owner()->sched()->mcp())
	if (sched()->prio() != param.prio())
	  {
	    // We need to protect the priority manipulation so that
	    // this thread can not be preempted an ready-enqueued
	    // according to a wrong priority
	    cpu_lock.lock();
	    if (in_ready_list())
	      ready_dequeue();	// need to re-queue in ready queue
				// according to new prio
	    sched()->set_prio (param.prio());
	    cpu_lock.clear();
	  }
      
      timeslice = param.time();
      if(timeslice!=(Unsigned64)-1)
	{
	  if(timeslice)
	    {
	      timeslice = timeslice / Config::microsec_per_tick;
	      
	      if (timeslice == 0)
		timeslice = 1;
	    }
	  
	  sched()->set_timeslice(timeslice);
	}
    }

    set_small_space(param.small());
  
  if (ext_preempter)
    {
      if (ext_preempter->exists())
	_ext_preempter = ext_preempter;
      else
	kdb_ke("sig_sched: invalid ext_preempter");
    }

  return true;
}


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

      fpage_unmap(space(), L4_fpage(0,0,L4_fpage::WHOLE_SPACE,0),  true, false);

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

    void advance_to_next ()
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
      _top->thread_lock ()->lock ();
      advance_to_next ();
    }

    Thread* operator->() const 
    { return _thread; }

    operator Thread* () const
    { return _thread; }

    kill_iter& operator++ ()
    { 
      advance_to_next ();
      return *this;
    }
  }; // class kill_iter
  
  // Back to the main proper of the function: Use the iterator we just
  // defined to kill the task.

  for (kill_iter next_to_kill (lookup_first_thread(subtask)); 
       next_to_kill;
       ++next_to_kill)
    {
      next_to_kill->kill();
      delete next_to_kill;	// Turn in in locked state -- lock will be
                                // cleared in destructor.
    }

  return true;
}

