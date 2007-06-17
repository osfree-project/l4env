INTERFACE:

#include "types.h"
#include "config.h"
#include "fpu_state.h"
#include "l4_types.h"
#include "per_cpu_data.h"
#include "sched_context.h"

class Entry_frame;
class Mem_space;
class Space;
class Thread_lock;

/** An execution context.  A context is a runnable, schedulable activity.
    It carries along some state used by other subsystems: A lock count,
    and stack-element forward/next pointers.
 */
class Context
{
  friend class Jdb_thread_list;

public:

  /**
   * Definition of different scheduling modes
   */
  enum Sched_mode {
    Periodic	= 0x1,	///< 0 = Conventional, 1 = Periodic
    Nonstrict	= 0x2,	///< 0 = Strictly Periodic, 1 = Non-strictly periodic
  };

  /**
   * Definition of different helping modes
   */
  enum Helping_mode {
    Helping,
    Not_Helping,
    Ignore_Helping
  };

  Mword is_tcb_mapped() const;
  Mword state() const;

  /**
   * Size of a Context (TCB + kernel stack)
   */
  static const size_t size = Config::thread_block_size;

  /**
   * Initialize cpu time of the idle thread.
   */
  void init_switch_time();

  /**
   * Return consumed CPU time.
   * @return Consumed CPU time in usecs
   */
  Cpu_time consumed_time();

  /**
   * Get the kernel UTCB pointer.
   * @return UTCB pointer, or 0 if there is no UTCB
   */
  Utcb* utcb() const;
  
  /**
   * Get the local ID of the context.
   */
  Local_id local_id() const;

  /**
   * Set the local ID of the context.
   * Does not touch the kernel UTCB pointer, since
   * we would need space() to do the address translation.
   *
   * After setting the local ID and mapping the UTCB area, use
   * Thread::utcb_init() to set the kernel UTCB pointer and initialize the
   * UTCB.
   */
  void local_id (Local_id id);

protected:
  /**
   * Update consumed CPU time during each context switch and when
   *        reading out the current thread's consumed CPU time.
   */
  void update_consumed_time();
  
  /**
   * Set the kernel UTCB pointer.
   * Does NOT keep the value of _local_id in sync.
   * @see local_id (Local_id id);
   */
  void utcb (Utcb *u);

  Mword			_state;
  Mword   *		_kernel_sp;
  void *_utcb_handler;

private:
  friend class Jdb;
  friend class Jdb_tcb;

  /// low level page table switching stuff
  void switchin_context() asm ("switchin_context_label") FIASCO_FASTCALL;

  /// low level fpu switching stuff
  void switch_fpu (Context *t);

  /// low level cpu switching stuff
  void switch_cpu (Context *t);

  Space *		_space;
  Context *		_donatee;
  Context *		_helper;

  // Lock state
  // how many locks does this thread hold on other threads
  // incremented in Thread::lock, decremented in Thread::clear
  // Thread::kill needs to know
  int			_lock_cnt;
  Thread_lock * const	_thread_lock;
  
  Local_id _local_id;
  Utcb *_utcb;

  // The scheduling parameters.  We would only need to keep an
  // anonymous reference to them as we do not need them ourselves, but
  // we aggregate them for performance reasons.
  Sched_context		_sched_context;
  Sched_context *	_sched;
  Unsigned64		_period;
  Sched_mode		_mode;
  unsigned short	_mcp;

  // Pointer to floating point register state
  Fpu_state		_fpu_state;

  // Implementation-specific consumed CPU time (TSC ticks or usecs)
  Cpu_time		_consumed_time;

  Context *		_ready_next;
  Context *		_ready_prev;

  static Per_cpu<bool> _schedule_in_progress;
  static Per_cpu<Sched_context *> _current_sched asm ("CONTEXT_CURRENT_SCHED");
  static Per_cpu<Cpu_time> _switch_time asm ("CONTEXT_SWITCH_TIME");
  static Per_cpu<unsigned> _prio_highest asm ("CONTEXT_PRIO_HIGHEST");
  static Per_cpu<Context *[256]> _prio_next asm ("CONTEXT_PRIO_NEXT");
};


IMPLEMENTATION:

#include <cassert>
#include "atomic.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "fpu.h"
#include "globals.h"		// current()
#include "lock_guard.h"
#include "logdefs.h"
#include "mem_layout.h"
#include "processor.h"
#include "space.h"
#include "std_macros.h"
#include "thread_state.h"
#include "timer.h"
#include "timeout.h"

Per_cpu<Sched_context *> Context::_current_sched DEFINE_PER_CPU;
Per_cpu<bool> Context::_schedule_in_progress DEFINE_PER_CPU;
Per_cpu<Context *[256]> Context::_prio_next DEFINE_PER_CPU;
Per_cpu<unsigned> Context::_prio_highest DEFINE_PER_CPU;
Per_cpu<Cpu_time> Context::_switch_time DEFINE_PER_CPU;

PUBLIC inline
unsigned
Context::cpu() const
{ return 0; }



/** Initialize a context.  After setup, a switch_exec to this context results
    in a return to user code using the return registers at regs().  The
    return registers are not initialized however; neither is the space_context
    to be used in thread switching (use set_space_context() for that).
    @pre (_kernel_sp == 0)  &&  (* (stack end) == 0)
    @param thread_lock pointer to lock used to lock this context
    @param space_context the space context
 */
PUBLIC inline NEEDS ["atomic.h", "entry_frame.h"]
Context::Context (Thread_lock *thread_lock,
		  Space* space,
		  unsigned short prio,
                  unsigned short mcp,
		  Unsigned64 quantum)
       : _space     (space),
	 _helper            (this),
         _thread_lock       (thread_lock),
         _sched_context     (this, 0, prio, quantum),
         _sched             (&_sched_context),
         _period            (0),
         _mode              (Sched_mode (0)),
         _mcp               (mcp),
         _consumed_time     (0)
{
  // NOTE: We do not have to synchronize the initialization of
  // _space_context because it is constant for all concurrent
  // invocations of this constructor.  When two threads concurrently
  // try to create a new task, they already synchronize in
  // sys_task_new() and avoid calling us twice with different
  // space_context arguments.

  Mword *init_sp = reinterpret_cast<Mword*>
    (reinterpret_cast<Mword>(this) + size - sizeof (Entry_frame));

  // don't care about errors: they just mean someone else has already
  // set up the tcb

  cas (&_kernel_sp, (Mword *) 0, init_sp);
}

/** Destroy context.
 */
PUBLIC virtual
Context::~Context()
{
  // If this context owned the FPU, noone owns it now
  if (Fpu::is_owner(this)) {
    Fpu::set_owner(0);
    Fpu::disable();
  }
}

/** @name State manipulation */
//@{
//-

#if 0
/**
 * State flags.
 * @return context's state flags
 */
PUBLIC inline
Mword
Context::state() const
{
  return _state;
}
#endif

/**
 * Does the context exist? .
 * @return true if this context has been initialized.
 */
PUBLIC inline NEEDS ["thread_state.h"]
Mword
Context::exists() const
{
  return state() != Thread_invalid;
}

/**
 * Atomically add bits to state flags.
 * @param bits bits to be added to state flags
 * @return 1 if none of the bits that were added had been set before
 */
PUBLIC inline NEEDS ["atomic.h"]
void
Context::state_add (Mword const bits)
{
  atomic_or (&_state, bits);
}

/**
 * Add bits in state flags. Unsafe (non-atomic) and
 *        fast version -- you must hold the kernel lock when you use it.
 * @pre cpu_lock.test() == true
 * @param bits bits to be added to state flags
 */ 
PUBLIC inline
void
Context::state_add_dirty (Mword bits)
{ 
  _state |=bits;
}

/**
 * Atomically delete bits from state flags.
 * @param bits bits to be removed from state flags
 * @return 1 if all of the bits that were removed had previously been set
 */
PUBLIC inline NEEDS ["atomic.h"]
void
Context::state_del (Mword const bits)
{
  atomic_and (&_state, ~bits);
}

/**
 * Delete bits in state flags. Unsafe (non-atomic) and
 *        fast version -- you must hold the kernel lock when you use it.
 * @pre cpu_lock.test() == true
 * @param bits bits to be removed from state flags
 */
PUBLIC inline
void
Context::state_del_dirty (Mword bits)
{
  _state &=~bits;
}

/**
 * Atomically delete and add bits in state flags, provided the
 *        following rules apply (otherwise state is not changed at all):
 *        - Bits that are to be set must be clear in state or clear in mask
 *        - Bits that are to be cleared must be set in state
 * @param mask Bits not set in mask shall be deleted from state flags
 * @param bits Bits to be added to state flags
 * @return 1 if state was changed, 0 otherwise
 */
PUBLIC inline NEEDS ["atomic.h"]
Mword
Context::state_change_safely (Mword const mask, Mword const bits)
{
  Mword old;

  do
    {
      old = _state;
      if (old & bits & mask | ~old & ~mask)
        return 0;
    }
  while (!cas (&_state, old, old & mask | bits));

  return 1;
}

/**
 * Atomically delete and add bits in state flags.
 * @param mask bits not set in mask shall be deleted from state flags
 * @param bits bits to be added to state flags
 */
PUBLIC inline NEEDS ["atomic.h"]
Mword
Context::state_change (Mword const mask, Mword const bits)
{
  return atomic_change (&_state, mask, bits);
}

/**
 * Delete and add bits in state flags. Unsafe (non-atomic) and
 *        fast version -- you must hold the kernel lock when you use it.
 * @pre cpu_lock.test() == true
 * @param mask Bits not set in mask shall be deleted from state flags
 * @param bits Bits to be added to state flags
 */
PUBLIC inline
void
Context::state_change_dirty (Mword const mask, Mword const bits)
{
  _state &= mask;
  _state |= bits;
}

//@}
//-

/** Return the space context.
    @return space context used for this execution context.
            Set with set_space_context().
 */
PUBLIC inline
Space *
Context::space() const
{
  return _space;
}

/** Convenience function: Return memory space. */
PUBLIC inline NEEDS["space.h"]
Mem_space*
Context::mem_space() const
{
  return _space->mem_space();
}

/** Thread lock.
    @return the thread lock used to lock this context.
 */
PUBLIC inline
Thread_lock *
Context::thread_lock() const
{
  return _thread_lock;
}

PUBLIC inline
unsigned short
Context::mcp() const
{
  return _mcp;
}

/** Registers used when iret'ing to user mode.
    @return return registers
 */
PUBLIC inline NEEDS["entry_frame.h"]
Entry_frame *
Context::regs() const
{
  return reinterpret_cast<Entry_frame *>
    (reinterpret_cast<Mword>(this) + size - sizeof(Entry_frame));
}

/** @name Lock counting
    These functions count the number of locks
    this context holds.  A context must not be deleted if its lock
    count is nonzero. */
//@{
//-

/** Increment lock count.
    @post lock_cnt() > 0 */
PUBLIC inline
void
Context::inc_lock_cnt()
{
  _lock_cnt++;
}

/** Decrement lock count.
    @pre lock_cnt() > 0
 */
PUBLIC inline
void
Context::dec_lock_cnt()
{
  _lock_cnt--;
}

/** Lock count.
    @return lock count
 */
PUBLIC inline
int
Context::lock_cnt() const
{
  return _lock_cnt;
}

//@}

/**
 * Switch active timeslice of this Context.
 * @param next Sched_context to switch to
 */
PUBLIC
void
Context::switch_sched (Sched_context * const next)
{
  // Ensure CPU lock protection
  assert (cpu_lock.test());

  // If we're leaving the global timeslice, invalidate it
  // This causes schedule() to select a new timeslice via set_current_sched()
  if (sched() == current_sched(cpu()))
    invalidate_sched();

  // Refill old timeslice
  sched()->set_left (sched()->quantum());

  // Ensure the new timeslice has a full quantum
  assert (next->left() == next->quantum());

  if (!in_ready_list())
    {
      set_sched (next);
      ready_enqueue();
    }

  else if (sched()->prio() != next->prio() 
      || _prio_next.cpu(cpu())[next->prio()] != this)
    {
      ready_dequeue();
      set_sched (next);
      ready_enqueue();
    }

  else if (next->prio())
    {
      set_sched (next);
      _prio_next.cpu(cpu())[next->prio()] = _ready_next;
    }
}

/**
 * Select a different context for running and activate it.
 */
PUBLIC
void
Context::schedule()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);


  CNT_SCHEDULE;

  // Ensure only the current thread calls schedule
  assert (this == current());
  
  Context ** const prio_next = _prio_next.cpu(cpu());
  unsigned &prio_highest = _prio_highest.cpu(cpu());
  bool &schedule_in_progress = _schedule_in_progress.cpu(cpu());

  // Nested invocations of schedule() are bugs
  assert (!schedule_in_progress);

  // Enqueue current thread into ready-list to schedule correctly
  update_ready_list();

  // Select a thread for scheduling.
  Context *next_to_run;

  for (;;)
    {
      next_to_run = prio_next[prio_highest];

      // Ensure ready-list sanity
      assert (next_to_run);
      
      if (EXPECT_TRUE (next_to_run->state() & Thread_ready))
        break;

      next_to_run->ready_dequeue();
 
      schedule_in_progress = true;

      cpu_lock.clear();
      Proc::irq_chance();
      cpu_lock.lock();

      schedule_in_progress = false;
    }

  switch_to_locked (next_to_run);
}

/**
 * Return if there is currently a schedule() in progress
 */
PUBLIC inline
bool
Context::schedule_in_progress()
{
  return _schedule_in_progress.cpu(cpu());
}

/**
 * Return true if s can preempt the current scheduling context, false otherwise
 */
PUBLIC static inline NEEDS ["globals.h"]
bool
Context::can_preempt_current (Sched_context const * const s)
{
  assert (current()->cpu() == s->owner()->cpu());

  // XXX: Workaround for missing priority boost implementation
  if (current()->sched()->prio() >= s->prio())
    return false;
  
  Sched_context const * const cs = current_sched(s->owner()->cpu());

  return !cs || cs->prio() < s->prio() || cs == s;
}

/**
 * Return currently active global Sched_context.
 */
PUBLIC static inline
Sched_context *
Context::current_sched(unsigned cpu)
{
  return _current_sched.cpu(cpu);
}

/**
 * Set currently active global Sched_context.
 */
PROTECTED
void
Context::set_current_sched (Sched_context * const sched)
{
  // Save remainder of previous timeslice or refresh it, unless it had
  // been invalidated
  Sched_context *&s = _current_sched.cpu(cpu());
  Timeout * const tt = timeslice_timeout.cpu(cpu());
  if (s)
    {
      Signed64 left = tt->get_timeout();
      s->set_left (left > 0 ? left : s->quantum ());
      LOG_SCHED_SAVE;
    }

  assert (sched);

  // Program new end-of-timeslice timeout
  tt->reset();
  tt->set (Timer::system_clock() + sched->left(), cpu());

  // Make this timeslice current
  s = sched;

  LOG_SCHED_LOAD;
}

/**
 * Invalidate (expire) currently active global Sched_context.
 */
PROTECTED inline NEEDS["logdefs.h","timeout.h"]
void
Context::invalidate_sched()
{
  LOG_SCHED_INVALIDATE;
  _current_sched.cpu(cpu()) = 0;
}

/**
 * Return Context's Sched_context with id 'id'; return time slice 0 as default.
 * @return Sched_context with id 'id' or 0
 */
PUBLIC inline
Sched_context *
Context::sched_context (unsigned short const id = 0)
{
  if (EXPECT_TRUE (!id))
    return &_sched_context;

  for (Sched_context *tmp = _sched_context.next();
      tmp != &_sched_context; tmp = tmp->next())
    if (tmp->id() == id)
      return tmp;

  return 0;
}

/**
 * Return Context's currently active Sched_context.
 * @return Active Sched_context
 */
PUBLIC inline
Sched_context *
Context::sched() const
{
  return _sched;
}

/**
 * Set Context's currently active Sched_context.
 * @param sched Sched_context to be activated
 */
PROTECTED inline
void
Context::set_sched (Sched_context * const sched)
{
  _sched = sched;
}

/**
 * Return Context's real-time period length.
 * @return Period length in usecs
 */
PUBLIC inline
Unsigned64
Context::period() const
{
  return _period;
}

/**
 * Set Context's real-time period length.
 * @param period New period length in usecs
 */
PROTECTED inline
void
Context::set_period (Unsigned64 const period)
{
  _period = period;
}

/**
 * Return Context's scheduling mode.
 * @return Scheduling mode
 */
PUBLIC inline
Context::Sched_mode
Context::mode() const
{
  return _mode;
}

/**
 * Set Context's scheduling mode.
 * @param mode New scheduling mode
 */
PUBLIC inline
void
Context::set_mode (Context::Sched_mode const mode)
{
  _mode = mode;
}

// queue operations

// XXX for now, synchronize with global kernel lock
//-

/**
 * Enqueue current() if ready to fix up ready-list invariant.
 */
PRIVATE inline NOEXPORT
void
Context::update_ready_list()
{
  assert (this == current());

  if (state() & Thread_ready)
    ready_enqueue();
}

/**
 * Check if Context is in ready-list.
 * @return 1 if thread is in ready-list, 0 otherwise
 */
PUBLIC inline
Mword
Context::in_ready_list() const
{
  return _ready_next != 0;
}

/**
 * Enqueue context in ready-list.
 */
PUBLIC
void
Context::ready_enqueue()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Don't enqueue threads which are already enqueued
  if (EXPECT_FALSE (in_ready_list()))
    return;

  // Don't enqueue threads that are not ready or have no own time
  if (EXPECT_FALSE (!(state() & Thread_ready) || !sched()->left()))
    return;

  unsigned short prio = sched()->prio();

  unsigned &prio_highest = _prio_highest.cpu(cpu());

  if (prio > prio_highest)
    prio_highest = prio;

  Context ** const prio_next = _prio_next.cpu(cpu());

  if (!prio_next[prio])
    prio_next[prio] = _ready_next = _ready_prev = this;

  else
    {
      _ready_next = prio_next[prio];
      _ready_prev = prio_next[prio]->_ready_prev;
      prio_next[prio]->_ready_prev = _ready_prev->_ready_next = this;

      // Special care must be taken wrt. the position of current() in the ready
      // list. Logically current() is the next thread to run at its priority.
      if (this == current())
        prio_next[prio] = this;
    }
}

/**
 * Remove context from ready-list.
 */
PUBLIC inline NEEDS ["cpu_lock.h", "lock_guard.h", "std_macros.h"]
void
Context::ready_dequeue()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Don't dequeue threads which aren't enqueued
  if (EXPECT_FALSE (!in_ready_list()))
    return;

  unsigned short prio = sched()->prio();

  Context ** const prio_next = _prio_next.cpu(cpu());

  if (prio_next[prio] == this)
    prio_next[prio] = _ready_next == this ? 0 : _ready_next;

  _ready_prev->_ready_next = _ready_next;
  _ready_next->_ready_prev = _ready_prev;
  _ready_next = 0;				// Mark dequeued

  unsigned &prio_highest = _prio_highest.cpu(cpu());

  while (!prio_next[prio_highest] && prio_highest)
    prio_highest--;
}

/** Helper.  Context that helps us by donating its time to us. It is
    set by switch_exec() if the calling thread says so.
    @return context that helps us and should be activated after freeing a lock.
*/
PUBLIC inline
Context *
Context::helper() const
{
  return _helper;
}

PUBLIC inline
void
Context::set_helper (enum Helping_mode const mode)
{
  switch (mode)
    {
    case Helping:
      _helper = current();
      break;
    case Not_Helping:
      _helper = this;
      break;
    case Ignore_Helping:
      // don't change _helper value
      break;
    }
}

/** Donatee.  Context that receives our time slices, for example
    because it has locked us.
    @return context that should be activated instead of us when we're
            switch_exec()'ed.
*/
PUBLIC inline
Context *
Context::donatee() const
{
  return _donatee;
}

PUBLIC inline
void
Context::set_donatee (Context * const donatee)
{
  _donatee = donatee;
}

PUBLIC inline
Mword *
Context::get_kernel_sp() const
{
  return _kernel_sp;
}

PUBLIC inline
void
Context::set_kernel_sp (Mword * const esp)
{
  _kernel_sp = esp;
}

PUBLIC inline
Fpu_state *
Context::fpu_state()
{
  return &_fpu_state;
}

/**
 * Add to consumed CPU time.
 * @param quantum Implementation-specific time quantum (TSC ticks or usecs)
 */
PUBLIC inline
void
Context::consume_time (Cpu_time const quantum)
{
  _consumed_time += quantum;
}

/**
 * Switch to scheduling context and execution context while not running under
 * CPU lock.
 */
PUBLIC inline NEEDS [<cassert>]
void
Context::switch_to (Context *t)
{
  // Call switch_to_locked if CPU lock is already held
  assert (!cpu_lock.test());

  // Grab the CPU lock
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  switch_to_locked (t);
}

/**
 * Switch scheduling context and execution context.
 * @param t Destination thread whose scheduling context and execution context
 *          should be activated.
 */
PUBLIC inline NEEDS [<cassert>]
void
Context::switch_to_locked (Context *t)
{
  // Must be called with CPU lock held
  assert (cpu_lock.test());

  // Switch to destination thread's scheduling context
  if (current_sched(cpu()) != t->sched())
    set_current_sched (t->sched());

  // XXX: IPC dependency tracking belongs here.

  // Switch to destination thread's execution context, no helping involved
  if (t != this)
    switch_exec_locked (t, Not_Helping);
}

/**
 * Switch execution context while not running under CPU lock.
 */
PUBLIC inline NEEDS [<cassert>]
void
Context::switch_exec (Context *t, enum Helping_mode mode)
{
  // Call switch_exec_locked if CPU lock is already held
  assert (!cpu_lock.test());

  // Grab the CPU lock
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  switch_exec_locked (t, mode);
}

/**
 * Switch to a specific different execution context.
 *        If that context is currently locked, switch to its locker instead
 *        (except if current() is the locker)
 * @pre current() == this  &&  current() != t
 * @param t thread that shall be activated.
 * @param mode helping mode; we either help, don't help or leave the
 *             helping state unchanged
 */
PUBLIC
void
Context::switch_exec_locked (Context *t, enum Helping_mode mode)
{
  // Must be called with CPU lock held
  assert (cpu_lock.test());
  assert (current() != t);
  assert (current() == this);
  assert (timeslice_timeout.cpu(cpu())->is_set()); // Coma check

  // only for logging
  Context *t_orig = t;
  (void)t_orig;

  // Time-slice lending: if t is locked, switch to its locker
  // instead, this is transitive
  while (t->donatee() &&		// target thread locked
         t->donatee() != t)		// not by itself
    {
      // Special case for Thread::kill(): If the locker is
      // current(), switch to the locked thread to allow it to
      // release other locks.  Do this only when the target thread
      // actually owns locks.
      if (t->donatee() == current())
        {
          if (t->lock_cnt() > 0)
            break;

          return;
        }

      t = t->donatee();
    }

  // Can only switch to ready threads!
  if (EXPECT_FALSE (!(t->state() & Thread_ready)))
    return;
  
  // Ensure kernel stack pointer is non-null if thread is ready
  assert (t->_kernel_sp);

  t->set_helper (mode);

  update_ready_list();

  LOG_CONTEXT_SWITCH;
  CNT_CONTEXT_SWITCH;

  switch_fpu (t);
  switch_cpu (t);
}

PUBLIC
GThread_num
Context::gthread_calculated()
{
  const Mword mask = Config::thread_block_size * Mem_layout::max_threads() - 1;

  return (((Address)this - Mem_layout::Tcbs) & mask) /
	 Config::thread_block_size;
}


PUBLIC inline
LThread_num
Context::lthread_calculated()
{
  return gthread_calculated() % L4_uid::threads_per_task();
}


PUBLIC inline
LThread_num
Context::task_calculated()
{
  return gthread_calculated() / L4_uid::threads_per_task();
}


IMPLEMENT inline
Local_id
Context::local_id() const
{
  return _local_id;
}

IMPLEMENT inline
void
Context::local_id (Local_id id)
{
  _local_id = id;
}

IMPLEMENT inline
Utcb *
Context::utcb() const
{
  return _utcb;
}

IMPLEMENT inline
void
Context::utcb (Utcb *u)
{
  _utcb = u;
}

