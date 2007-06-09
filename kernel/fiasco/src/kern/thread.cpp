INTERFACE:

#include <csetjmp>             // typedef jmp_buf
#include "l4_types.h"
#include "config.h"
#include "deadline_timeout.h"
#include "helping_lock.h"
#include "mem_layout.h"
#include "preemption.h"
#include "receiver.h"
#include "sender.h"
#include "space.h"		// Space_index
#include "thread_lock.h"
#include "utcb.h"

class Irq_alloc;
class Return_frame;
class Syscall_frame;
class Task;

/** A thread.  This class is the driver class for most kernel functionality.
 */
class Thread : public Receiver, public Sender
{
  friend class Jdb;
  friend class Jdb_bt;
  friend class Jdb_tcb;
  friend class Jdb_thread_list;
  friend class Jdb_list_threads;
  friend class Jdb_list_timeouts;
  friend class Jdb_tbuf_show;

public:

  typedef void (Utcb_copy_func)(Thread *sender, Thread *receiver);

  /**
   * Constructor.
   *
   * @param task the task the thread should reside in.
   * @param id user-visible thread ID of the sender.
   * @param init_prio initial priority.
   * @param mcp maximum controlled priority.
   *
   * @post state() != Thread_invalid.
   */
  Thread (Task* task, Global_id id,
	  unsigned short init_prio, unsigned short mcp);


  int handle_page_fault (Address pfa, Mword error, Mword pc, 
      Return_frame *regs);

  /**
   * Task number for debugging purposes.
   * May be changed to show sth. more useful for the debugger.
   * Do not rely on this method in kernel code.
   * @see: d_threadno()
   */
  Task_num d_taskno();

  /**
   * Thread number for debugging purposes.
   * @see: d_taskno()
   */
  LThread_num d_threadno();

  void sys_ipc();

private:
  Thread(const Thread&);	///< Default copy constructor is undefined
  void *operator new(size_t);	///< Default new operator undefined

  bool handle_sigma0_page_fault (Address pfa);
  bool handle_smas_page_fault (Address pfa, Mword error, Ipc_err &ipc_code);

  /**
   * Additional things to do before killing, when using small spaces.
   */
  void kill_small_space();

  /**
   * Return small address space the task is in.
   */
  Mword small_space();

  /**
   * Return to user.
   *
   * This function is the default routine run if a newly
   * initialized context is being switch_exec()'ed.
   */
  static void user_invoke();

  bool associate_irq(Irq_alloc *irq);
  void disassociate_irq(Irq_alloc *irq);

public:
  static bool pagein_tcb_request(Return_frame *regs);

  inline Mword user_ip() const;
  inline void user_ip(Mword);

  inline Mword user_sp() const;
  inline void user_sp(Mword);

  inline Mword user_flags() const;

protected:
  /**
   * Move the task this thread belongs to to the given small address space
   */
  void set_small_space (Mword nr);

  // implementation details follow...

  // DATA

  // Preemption IPC sender role
  Preemption _preemption;

  // Deadline Timeout
  Deadline_timeout _deadline_timeout;

  // Another critical TCB cache line:
  Task*        _task;
  Thread_lock  _thread_lock;

  // More ipc state
  Thread *_pager, *_ext_preempter;
  Thread *present_next, *present_prev;
  Thread *_cap_handler;
//  Irq_alloc *_irq;

  // long ipc state
  L4_rcv_desc _target_desc;	// ipc buffer in receiver's address space
  unsigned _pagein_status_code;

  Address _vm_window0, _vm_window1; // data windows for the
				// IPC partner's address space (for long IPC)
  jmp_buf *_recover_jmpbuf;	// setjmp buffer for page-fault recovery

  // debugging stuff
  Address _last_pf_address;
  unsigned _last_pf_error_code;

  unsigned _magic;
  static const unsigned magic = 0xf001c001;

  // for trigger_exception
  Address _exc_ip;
  unsigned _irq;

  static Helping_lock tcb_area_lock;

  // Constructor
  Thread (Task* task, L4_uid id);

  // Thread killer
  void kill_all();
};


IMPLEMENTATION:

#include <cassert>
#include <cstdlib>		// panic()
#include <cstring>
#include "atomic.h"
#include "entry_frame.h"
#include "feature.h"
#include "fpu_alloc.h"
#include "globals.h"
#include "irq_alloc.h"
#include "kdb_ke.h"
#include "logdefs.h"
#include "map_util.h"
#include "ram_quota.h"
#include "ram_quota_alloc.h"
#include "sched_context.h"
#include "space_index_util.h"
#include "std_macros.h"
#include "task.h"
#include "thread_state.h"
#include "timeout.h"

KIP_KERNEL_FEATURE("multi_irq");
KIP_KERNEL_FEATURE("exception_ipc");

Helping_lock Thread::tcb_area_lock;

PRIVATE static
Thread *
Thread::maybe_create(Task *task, L4_uid id, bool &n)
{
  Thread *t = id_to_tcb (id);
  Address tcb_page = (Address)t & Config::PAGE_MASK;
  while (1)
    {
      n = false;
      Lock_guard<Helping_lock> area_guard(&tcb_area_lock);

	{
	  Lock_guard<Cpu_lock> cpu_guard(&cpu_lock);
	  if (EXPECT_TRUE(t->thread_lock()->lock_dirty() 
		!= Thread_lock::Invalid))
	    {
	      if (!t->exists())
		n = true;

	      return t;
	    }
	}


      if (EXPECT_FALSE(! task->ram_quota()->alloc(Config::PAGE_SIZE)))
	return 0;

      if (EXPECT_FALSE(! Vmem_alloc::page_alloc((void*)tcb_page,
	      Vmem_alloc::ZERO_FILL)))
	{
	  task->ram_quota()->free(Config::PAGE_SIZE);
	  return 0;
	}

      current_mem_space()->kmem_update((void*)tcb_page);
      if (EXPECT_TRUE(t->thread_lock()->lock() != Thread_lock::Invalid))
	{
	  n = true;
	  return t;
	}
    }
}

PUBLIC static
Thread *
Thread::create(Task *task, L4_uid id)
{
  bool is_new = false;
  Thread *t = maybe_create(task, id, is_new);
  if (is_new)
    new (t) Thread(task, id);

  return t;
}

PUBLIC static
Thread *
Thread::create(Task *task, L4_uid id, unsigned short prio, unsigned short mcp)
{
  bool is_new = false;
  Thread *t = maybe_create(task, id, is_new);
  if (is_new)
    new (t) Thread(task, id, prio, mcp);

  return t;
}

PUBLIC inline NEEDS[Thread::thread_lock]
void
Thread::kill_lock()
{
  thread_lock()->lock();
}

PUBLIC static
void
Thread::destroy(Ram_quota *q, void *_t)
{
  Thread *t = (Thread*)_t;
  void *tcb;
  Address tcb_page = (Address)t & Config::PAGE_MASK;

    {
      Lock_guard<Cpu_lock> cpu_guard(&cpu_lock);
      if (EXPECT_FALSE(!t->is_tcb_mapped()))
	return;

      Thread_lock::Lock_context lc = t->thread_lock()->clear_no_switch_dirty();

      for (Address thread = tcb_page; thread < tcb_page + Config::PAGE_SIZE;
	  thread += Config::thread_block_size)
	{
	  Thread *x = (Thread*)thread;
	  if (x->state() != Thread_invalid 
	      || x->thread_lock()->test())
	    {
	      t->thread_lock()->switch_dirty(lc);
	      return;
	    }
	}
      tcb = Vmem_alloc::page_unmap((void*)tcb_page);
      t->thread_lock()->switch_dirty(lc);
    }

  Mapped_allocator::allocator()->q_free(q, Config::PAGE_SHIFT, tcb);
  
  // are we the last task on this quota ??
  if (q->current() == 0)
    Ram_quota_alloc::free(q);
}

/** Class-specific allocator.
    This allocator ensures that threads are allocated at a fixed virtual
    address computed from their thread ID.
    @param id thread ID
    @return address of new thread control block
 */
PRIVATE inline
void *
Thread::operator new(size_t, Thread *t) throw ()
{
  // Allocate TCB in TCB space.  Actually, do not allocate anything,
  // just return the address.  Allocation happens on the fly in
  // Thread::handle_page_fault().
  return t;
}

/** Deallocator.  This function currently does nothing: We do not free up
    space allocated to thread-control blocks.
 */
PUBLIC inline
void
Thread::operator delete(void *t)
{
  // XXX should check if all thread blocks on a given page are free
  // and deallocate (or mark free) the page if so.  this should be
  // easy to detect since normally all threads of a given task are
  // destroyed at once at task deletion time
  destroy(reinterpret_cast<Ram_quota*>(((Thread*)t)->_cap_handler), t);
}

/** Cut-down version of Thread constructor; only for kernel threads
    Do only what's necessary to get a kernel thread started --
    skip all fancy stuff, no locking is necessary.
    @param task the address space
    @param id user-visible thread ID of the sender
 */
IMPLEMENT
Thread::Thread(Task* task, L4_uid id)
  : Receiver(&_thread_lock, task,
	     Config::kernel_prio, Config::kernel_mcp,
	     Config::default_time_slice),
    Sender(id), _preemption(id), _deadline_timeout(&_preemption),
    _task(task), _magic(magic), _exc_ip(~0UL)
{
  *reinterpret_cast<void(**)()>(--_kernel_sp) = user_invoke;

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Config::thread_block_size-sizeof(Thread)-64);
}

/** Destructor.  Reestablish the Context constructor's precondition.
    @pre current() == thread_lock()->lock_owner()
         && state() == Thread_dead
    @pre lock_cnt() == 0
    @post (_kernel_sp == 0)  &&  (* (stack end) == 0)  &&  !exists()
 */
PUBLIC virtual
Thread::~Thread()		// To be called in locked state.
{
  assert(current() == thread_lock()->lock_owner());
  assert(state() == Thread_dead);
  assert(_magic == magic);

  unsigned long *init_sp = reinterpret_cast<unsigned long*>
    (reinterpret_cast<unsigned long>(this) + size - sizeof(Entry_frame));

  _kernel_sp = 0;
  *--init_sp = 0;
  Fpu_alloc::free_state(fpu_state());
  destroy_utcb();
  state_change(0, Thread_invalid);
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
current_thread()
{
  return Thread::lookup(current());
}

PUBLIC inline
bool
Thread::exception_triggered() const
{ return _exc_ip != ~0UL; }

//
// state requests/manipulation
//

/** Task.
    @return pointer to thread's task.
 */
PUBLIC inline NEEDS["task.h"]
Task *
Thread::task() const
{
  return _task;
}

/** Thread lock.
    Overwrite Context's version of thread_lock() with a semantically
    equivalent, but more efficient version.
    @return lock used to synchronize accesses to the thread.
 */
PUBLIC inline
Thread_lock *
Thread::thread_lock()
{
  return &_thread_lock;
}

PUBLIC inline
Preemption *
Thread::preemption()
{
  return &_preemption;
}

PUBLIC inline NEEDS ["config.h", "timeout.h"]
void
Thread::handle_timer_interrupt()
{
  // XXX: This assumes periodic timers (i.e. bogus in one-shot mode)
  if (!Config::fine_grained_cputime)
    consume_time (Config::scheduler_granularity);

  // Check if we need to reschedule due to timeouts or wakeups
  if (Timeout::do_timeouts() && !Context::schedule_in_progress())
    {
      schedule();
      assert (timeslice_timeout->is_set());	// Coma check
    }
}

/**
 * Calculate TCB pointer from global thread ID.
 *
 * @param id the thread ID.
 * @return TCB pointer if the ID was valid, 0 otherwise.
 */
PUBLIC static inline NEEDS ["config.h", "mem_layout.h"]
Thread *
Thread::id_to_tcb (Global_id id)
{
  return reinterpret_cast <Thread *>
    (id.is_invalid() || id.gthread() >= Mem_layout::max_threads() ?
     0 : Mem_layout::Tcbs + (id.gthread() * Config::thread_block_size));
}

PUBLIC
void
Thread::halt()
{
  // Cancel must be cleared on all kernel entry paths. See slowtraps for
  // why we delay doing it until here.
  state_del (Thread_cancel);

  // we haven't been re-initialized (cancel was not set) -- so sleep
  if (state_change_safely (~Thread_ready, Thread_cancel | Thread_dead))
    while (! (state() & Thread_ready))
      schedule();
}

PUBLIC static
void
Thread::halt_current ()
{
  for (;;)
    {
      current_thread()->halt();
      kdb_ke("Thread not halted");
    }
}

PRIVATE static inline
void
Thread::user_invoke_generic()
{
  assert (current()->state() & Thread_ready);

  // release CPU lock explicitly, because
  // * the context that switched to us holds the CPU lock
  // * we run on a newly-created stack without a CPU lock guard
  cpu_lock.clear();
}

/**
 * Return first thread in an address space.
 */
PUBLIC static inline
Thread *
Thread::lookup_first_thread (unsigned space)
{ return id_to_tcb (Global_id (space, 0)); }

/** Thread's space index.
    @return thread's space index.
 */
PUBLIC inline
Space_index
Thread::space_index() const
{ return Space_index (id().task()); }

/** Chief's space index.
    @return thread's chief's space index.
 */
PUBLIC inline
Space_index
Thread::chief_index() const
{ return Space_index(space()->chief()); }

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
      assert (state() & Thread_ready);

      while (lock_cnt() > 0)
        {
          current()->switch_exec (this, Helping);

          // Thread "this" will switch back to us ("current()") as
          // soon as its lock count drops to 0.
          //
          // XXX This is not true anymore, either the thread will
          // switch back because we have helped it or because it
          // detected that someone is trying to kill it and therefore
          // called schedule which in turn switched back to us.
        }
    }

  state_change (0, Thread_dead);

  unset_utcb_ptr();

  _cap_handler = reinterpret_cast<typeof(_cap_handler)>(_task->ram_quota());

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

      fpage_unmap(space(), L4_fpage::all_spaces(), true, 0, Unmap_full);

      //for small spaces...
      kill_small_space();

      //
      //  Delete our address space
      //
      delete _task;
    }

  //
  // Kill this thread
  //

  // But first prevent it from being woken up by asynchronous events

  // if attached to irqs, detach
  if (_irq)
    Irq_alloc::free_all(this);

  // if IPC timeout active, reset it
  if (_timeout)
    _timeout->reset();

  {
    Lock_guard <Cpu_lock> guard (&cpu_lock);

    // Reset deadline timeout
    _deadline_timeout.reset();

    // Switch to time-sharing mode
    set_mode (Sched_mode (0));

    // Switch to time-sharing scheduling context
    if (sched() != sched_context())
      switch_sched (sched_context());

    if (current_sched()->owner() == this)
      current()->switch_to_locked(current());
  }

  // possibly dequeue from a wait queue
  wait_queue_kill();

  // if other threads want to send me IPC messages, abort these
  // operations
  {
    Lock_guard <Cpu_lock> guard (&cpu_lock);
    while (Sender *s = Sender::cast(sender_list()->head()))
      {
	s->ipc_receiver_aborted();
	Proc::preemption_point();
      }
  }

  // if engaged in IPC operation, stop it
  if (receiver())
    sender_dequeue (receiver()->sender_list());

  // if engaged in PIPC operation, stop it
  preemption()->set_pending (0);
  if (preemption()->receiver())
    preemption()->sender_dequeue (preemption()->receiver()->sender_list());

  // Deallocate all reservation scheduling contexts
  Sched_context *s, *tmp;
  for (s = sched_context()->next(); s != sched_context(); tmp = s,
       s = s->next(), tmp->dequeue(), delete tmp);

  // dequeue from system queues
  assert (in_present_list());
  present_dequeue();
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
	      _thread->kill_lock();
	      return;
	    }

	  _thread = next_child (_thread_0);
	  if (_thread)
	    {
	      _thread_0 = _thread;
	      _thread_0->kill_lock();
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
      _top->kill_lock();
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
      if (next_to_kill->kill())	// Turn in in locked state
        delete next_to_kill; // also clears the lock
      else
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

IMPLEMENT inline Task_num Thread::d_taskno() { return space_index(); }
IMPLEMENT inline LThread_num Thread::d_threadno() { return id().lthread(); }

IMPLEMENT
bool
Thread::associate_irq(Irq_alloc *irq)
{
  if (irq->alloc(this))
    {
      ++_irq;
      return true;
    }
  return false;
}

IMPLEMENT
void
Thread::disassociate_irq(Irq_alloc *irq)
{
  if (irq->free(this))
    --_irq;
}

/** (Re-) Ininialize a thread and make it ready.
    XXX Contrary to the L4-V2 spec we only cancel IPC if eip != 0xffffffff!
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
		   Thread* pager, Receiver* preempter,
		   Thread* cap_handler,
		   Address *o_ip = 0, Address *o_sp = 0,
		   Thread* *o_pager = 0, Receiver* *o_preempter = 0,
		   Thread* *o_cap_handler = 0,
		   Address *o_flags = 0,
		   bool no_cancel = 0,
		   bool alien = 0,
		   bool trigger_exception = 0)
{
  assert (current() == thread_lock()->lock_owner());

  if (state() == Thread_invalid)
    return false;

  Entry_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_cap_handler) *o_cap_handler = _cap_handler;
  if (o_preempter) *o_preempter = preemption()->receiver();
  if (o_sp) *o_sp = user_sp();
  if (o_ip) *o_ip = user_ip();
  if (o_flags) *o_flags = user_flags();

  if (pager)
    _pager = pager;

  if (preempter)
    preemption()->set_receiver (preempter);

  if (cap_handler)
    _cap_handler = cap_handler;

  // Changing the run state is only possible when the thread is not in
  // an exception.
  if (no_cancel && (state() & Thread_in_exception))
    // XXX Maybe we should return false here.  Previously, we actually
    // did so, but we also actually didn't do any state modification.
    // If you change this value, make sure the logic in
    // sys_thread_ex_regs still works (in particular,
    // ex_regs_cap_handler and friends should still be called).
    return true;

  if (state() & Thread_dead)	// resurrect thread
    state_change (~Thread_dead, Thread_ready);

  else if (!no_cancel)		// cancel ongoing IPC or other activity
    state_change (~(Thread_ipc_in_progress | Thread_delayed_deadline |
                    Thread_delayed_ipc), Thread_cancel | Thread_ready);

  if (trigger_exception)
    do_trigger_exception(r);

  if (ip != ~0UL)
    {
      user_ip(ip);

      if (alien)
	state_change (~Thread_dis_alien, Thread_alien);
      else
	state_del (Thread_alien);
    }

  if (sp != ~0UL)
    user_sp (sp);

  return true;
}


/** Clears the utcb pointer of the Thread
 *  Reason: To avoid a stale pointer after unmapping and deallocating
 *  the UTCB. Without this the Thread_lock::clear will access the UTCB
 *  after the unmapping the UTCB -> POOFFF.
 */
PUBLIC inline
void
Thread::unset_utcb_ptr()
{
  utcb(0);
  local_id(0);
}


PRIVATE
void
Thread::destroy_utcb()
{
  task()->free_utcb(id().lthread());
}


/** Setup the UTCB pointer of this thread.
 */
PRIVATE
void
Thread::setup_utcb_kernel_addr()
{
  Utcb *ptr = task()->alloc_utcb(id().lthread());
  if (!ptr)
    kdb_ke("could not allocate UTCB");

  utcb(ptr);
  local_id(task()->user_utcb(id().lthread()));

  _utcb_handler = 0;
}


PRIVATE static inline
void
Thread::copy_utcb_to_utcb(L4_msg_tag const &tag, Thread *snd, Thread *rcv)
{
  assert (cpu_lock.test());

  Utcb *snd_utcb = snd->access_utcb();
  Utcb *rcv_utcb = rcv->access_utcb();
  Mword s = tag.words();
  Mword r = Utcb::Max_words;

  Cpu::memcpy_mwords (rcv_utcb->values, snd_utcb->values, r < s ? r : s);
  if (tag.transfer_fpu() && rcv_utcb->inherit_fpu())
    snd->transfer_fpu(rcv);
}


PUBLIC
void
Thread::copy_utcb_to(L4_msg_tag const &tag, Thread* receiver)
{
  // we cannot copy trap state to trap state!
  assert(!this->_utcb_handler || !receiver->_utcb_handler);
  if (EXPECT_FALSE(this->_utcb_handler != 0))
    copy_ts_to_utcb(tag, this, receiver);
  else if (EXPECT_FALSE(receiver->_utcb_handler != 0))
    copy_utcb_to_ts(tag, this, receiver);
  else
    copy_utcb_to_utcb(tag, this, receiver);
}


PROTECTED inline
bool
Thread::ex_regs_permission_inter_task(Sys_ex_regs_frame *regs,
				      L4_uid *dst_id, Thread **dst,
				      Task **dst_task, Thread **dst_thread0)
{
  // inter-task ex-regs: also consider the task number
  if (EXPECT_TRUE(!regs->task()
	|| regs->task() == id().task()))
    return true;		// local ex_regs

  *dst_thread0 = lookup (L4_uid (regs->task(), 0), space());

  if (!*dst_thread0)
    return false;		// error

  {
    // avoid race between checking existence of thread and getting the
    // thread lock
    Lock_guard<Cpu_lock> guard(&cpu_lock);

    if (EXPECT_FALSE(!(*dst_thread0)->is_tcb_mapped()
	  || !(*dst_thread0)->exists()))
      return false;

    (*dst_thread0)->thread_lock()->lock_dirty();
  }

  // now we own the lock of thread 0 of the target task

  // take thread 0 of target as the ID template
  *dst_id = (*dst_thread0)->id();
  dst_id->lthread(regs->lthread());

  // Are we the pager of dst_id (just check address spaces)?
  Thread *dst_check = *dst = id_to_tcb(*dst_id);

  // if dst_check->_pager is nil (i.e. the destination thread does not
  // exist), we check the pager of thread0 of that address space
  if (!dst_check || !dst_check->is_tcb_mapped() 
      || !dst_check->_pager || !(*dst)->in_present_list())
    dst_check = *dst_thread0;

  if (dst_check->_pager->id().task() != id().task()
#ifdef CONFIG_TASK_CAPS
      && dst_check->_cap_handler->id().task() != id().task()
#endif
      )
    {
      WARN("Security violation: Not fault handler of %x.", dst_id->task());
      (*dst_thread0)->thread_lock()->clear();
      regs->old_eflags (~0U);
      return false;		// error
    }

  *dst_task = (*dst_thread0)->task();
  return true;			// success
}


PUBLIC inline
void
Thread::recover_jmp_buf(jmp_buf *b)
{ _recover_jmpbuf = b; }

//---------------------------------------------------------------------------
IMPLEMENTATION [!log]:

PUBLIC inline
unsigned Thread::sys_ipc_log(Syscall_frame *)
{ return 0; }

PUBLIC inline
unsigned Thread::sys_ipc_trace(Syscall_frame *)
{ return 0; }

static inline
void Thread::page_fault_log(Address, unsigned, unsigned)
{}

PUBLIC static inline
int Thread::log_page_fault()
{ return 0; }

PUBLIC inline
unsigned Thread::sys_fpage_unmap_log(Syscall_frame *)
{ return 0; }

//---------------------------------------------------------------------------
IMPLEMENTATION [!smas]:

IMPLEMENT inline
bool Thread::handle_smas_page_fault( Address, Mword, Ipc_err & )
{ return false; }

IMPLEMENT inline
void Thread::kill_small_space (void)
{}

IMPLEMENT inline
Mword Thread::small_space( void )
{ return 0; }

IMPLEMENT inline
void Thread::set_small_space( Mword /*nr*/)
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [!io]:

PUBLIC inline
bool
Thread::has_privileged_iopl()
{
  return false;
}


// ----------------------------------------------------------------------------
IMPLEMENTATION [!pl0_hack]:
PUBLIC static void
Thread::privilege_initialization()
{
}
