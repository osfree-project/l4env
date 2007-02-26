INTERFACE:

#include "types.h"

#include "config.h"
#include "fpu_state.h"
#include "sched_context.h"

class Space_context;
class Thread_lock;
class Entry_frame;

/** An execution context.  A context is a runnable, schedulable activity. 
    It carries along some state used by other subsystems: A lock count,
    and stack-element forward/next pointers.
 */
class Context
{
public:
  typedef Mword Status;
  static const size_t size = Config::thread_block_size;

  Mword switch_to( Context *t );
  Status state() const;
  Mword exists() const;
  Mword state_add( Status );
  Mword state_del( Status );
  Mword state_change( Status mask, Status bits );
  Mword state_change_safely( Status mask, Status bits );
  void state_change_dirty( Status mask, Status bits );
  Space_context *space_context() const;
  Thread_lock *thread_lock() const;
  unsigned short mcp() const;
  Entry_frame *regs() const;
  void inc_lock_cnt();
  void dec_lock_cnt();
  int lock_cnt() const;
  Mword in_ready_list() const;
  void ready_enqueue();
  void ready_dequeue();

  void schedule();
  Context *get_next() const;
  void set_next( Context * );
  Context *donatee() const;
  void set_donatee( Context * );
  void set_kernel_sp( Mword *sp );
  Fpu_state *const fpu_state();

  void set_sched (Sched_context *sched);
  Sched_context *sched();
  Sched_context *timesharing_sched();

  static Sched_context *current_sched();
  static void set_current_sched (Sched_context *sched);
  
  static void timer_tick (bool reschedule);
  
private:
  friend class Jdb;
  friend class Jdb_tcb;

  /// low level switching stuff
  void switchin_context() asm("call_switchin_context");
  /// low level switching stuff
  void switch_fpu( Context *t );
  /// low level switching stuff
  bool switch_cpu( Context *t );

  virtual void preemption_event (Sched_context *sched) = 0;

  Space_context *_space_context;
  Context *_stack_link;	// Links entries in Stack<>
  Context *_donatee;

  // Lock state
  // how many locks does this thread hold on other threads
  // incremented in Thread::lock, decremented in Thread::clear
  // Thread::kill needs to know
  int _lock_cnt;
  Thread_lock *const _thread_lock;

  // The scheduling parameters.  We would only need to keep an
  // anonymous reference to them as we do not need them ourselves, but
  // we aggregate them for performance reasons.
  Sched_context	_timesharing_sched;
  Sched_context *_sched;

  unsigned short _mcp;

  // Pointer to floating point register state
  Fpu_state _fpu_state;

  static Sched_context *_current_sched;

  // CLASS DATA
protected:
  Status _state;
  Context *ready_next;
  Context *ready_prev;
  Mword   *kernel_sp;

  static Context *prio_first[256] asm ("CONTEXT_PRIO_FIRST");
  static Context *prio_next[256]  asm ("CONTEXT_PRIO_NEXT");
  static unsigned prio_highest    asm ("CONTEXT_PRIO_HIGHEST");
  static int timeslice_ticks_left;
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
#include "processor.h"
#include "space_context.h"
#include "std_macros.h"
#include "thread_state.h"

Sched_context *	Context::_current_sched;
int		Context::timeslice_ticks_left;
Context *	Context::prio_first[256];
Context *	Context::prio_next[256];
unsigned	Context::prio_highest;

/** Initialize a context.  After setup, a switch_to to this context results 
    in a return to user code using the return registers at regs().  The 
    return registers are not initialized however; neither is the space_context
    to be used in thread switching (use set_space_context() for that).
    @pre (kernel_sp == 0)  &&  (* (stack end) == 0)
    @param thread_lock pointer to lock used to lock this context 
    @param space_context the space context 
 */
PUBLIC inline NEEDS ["atomic.h", "entry_frame.h"]
Context::Context (Thread_lock *thread_lock,
		  Space_context* space_context,
		  unsigned short prio,
                  unsigned short mcp,
		  unsigned short timeslice)
       : _space_context (space_context),
         _thread_lock (thread_lock),
         _timesharing_sched (this, 0, prio, timeslice),
         _sched (&_timesharing_sched),
         _mcp (mcp)
{
  // NOTE: We do not have to synchronize the initialization of
  // _space_context because it is constant for all concurrent
  // invocations of this constructor.  When two threads concurrently
  // try to create a new task, they already synchronize in
  // sys_task_new() and avoid calling us twice with different
  // space_context arguments.

  Mword *init_sp = reinterpret_cast<Mword*>
    (reinterpret_cast<Mword>(this) + size - sizeof(Entry_frame));

  // don't care about errors: they just mean someone else has already
  // set up the tcb

  smp_cas(&kernel_sp, (Mword*)0, init_sp);
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

/** State flags.
    @return context's state flags
 */
IMPLEMENT inline 
Context::Status Context::state() const
{ 
  return _state; 
}

/** Does the context exist? .
    @return true if this context has been initialized.
 */
IMPLEMENT inline NEEDS ["thread_state.h"]
Mword
Context::exists() const
{ 
  return _state != Thread_invalid; 
}

/** Atomically add bits to state flags. 
    @param bits bits to be added to state flags
    @return true if none of the bits that were added had been set before
 */
IMPLEMENT inline NEEDS ["atomic.h"]
Mword Context::state_add( Status bits )
{ 
  Mword ret;  
  Status old;
  do 
    {
      old = _state;
      ret = (_state & bits) == 0;
    }
  while (! smp_cas(&_state, old, old | bits));

  return ret;
}

/** Atomically delete bits from state flags. 
    @param bits bits to be removed from state flags
    @return true if all of the bits that were removed had previously been set
 */
IMPLEMENT inline NEEDS ["atomic.h"]
Mword Context::state_del( Status bits )
{ 
  Mword ret;  
  Status old;
  do 
    {
      old = _state;
      ret = (_state & bits) == bits;
    }
  while (! smp_cas(&_state, old, old & ~bits));

  return ret;
}

/** Atomically add and delete bits from state flags. 
    @param mask bits not set in mask shall be deleted from state flags
    @param bits bits to be added to state flags
    @return true if all of the bits that were removed had previously been set,
                 and all of the bits that were set previously were clear
 */
IMPLEMENT inline NEEDS ["atomic.h"]
Mword Context::state_change( Status mask, Status bits )
{
  Mword ret;  
  Status old;
  do 
    {
      old = _state;
      ret = ((old & bits & mask) == 0) && ((old & ~mask) == ~mask);
    }
  while (! smp_cas(&_state, old, (old & mask) | bits));

  return ret;
}

/** Atomically add and delete bits from state flags, provided that all
    of the modified bits actually change.  This method works like
    state_change, however it only actually modifies the state word if
    state_change would return true.
 
    @param mask bits not set in mask shall be deleted from state flags
    @param bits bits to be added to state flags
    @return true if modification was successful (i.e., all modified 
            bits changed); false if modification was unsuccessful (i.e.,
	    state word was not changed)
 */
IMPLEMENT inline NEEDS ["atomic.h"] 
Mword Context::state_change_safely( Status mask, Status bits )
{
  Mword ret;  
  Status old;
  do 
    {
      old = _state;
      ret = ((old & bits & mask) == 0) && ((old & ~mask) == ~mask);
      if (! ret)
	return false;
    }
  while (! smp_cas(&_state, old, (old & mask) | bits));

  return true;
}


/** Modify state flags.  Unsafe (nonatomic) and fast version -- you must hold
    kernel lock when you use it.
    @pre cpu_lock.test() == true
    @param mask bits not set in mask shall be deleted from state flags
    @param bits bits to be added to state flags
 */
IMPLEMENT inline 
void Context::state_change_dirty( Status mask, Status bits )
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
IMPLEMENT inline 
Space_context *Context::space_context() const
{ 
  return _space_context; 
}

/** Thread lock.
    @return the thread lock used to lock this context.
 */
IMPLEMENT inline 
Thread_lock *Context::thread_lock() const
{ 
  return _thread_lock; 
}


IMPLEMENT inline
unsigned short 
Context::mcp() const
{ 
  return _mcp; 
}       

/** Registers used when iret'ing to user mode.
    @return return registers
 */
IMPLEMENT inline NEEDS["entry_frame.h"]
Entry_frame *Context::regs() const
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
IMPLEMENT inline 
void Context::inc_lock_cnt()
{
  _lock_cnt++;
}

/** Decrement lock count.
    @pre lock_cnt() > 0
 */
IMPLEMENT inline 
void Context::dec_lock_cnt()
{
  _lock_cnt--;
}

/** Lock count.
    @return lock count
 */
IMPLEMENT inline 
int Context::lock_cnt() const
{
  return _lock_cnt;
}

//@}


// 
// The scheduler
// 


/** The Scheduler. Select a different context for running, and
    activate that context's time slice.  Save the current context's
    time slice.  The current context is added or removed to/from the
    ready list now, if necessary, depending on its state. 
 */
IMPLEMENT
void
Context::schedule()
{
  // Careful!  At this point, we may (due to a context switch) be
  // dequeued from the ready list but then set ready again, so
  // that our "ready_next" pointer becomes invalid
  cpu_lock.lock();

  if (in_ready_list())
    {
      if (! (state() & Thread_running))
	ready_dequeue();
    }
  else    
    {
      // we're not in the ready list.  if we're ready nontheless,
      // enqueue now so that we can be scheduled correctly

      if (state() & Thread_running)
	ready_enqueue();
    }

  // Global timeslice handling.
  // 
  // The timeslice (in timeslice_ticks_left) is independent from who
  // actually called schedule().  It stays the same across switch_to()'s.
  // 
  // Here in schedule() we save the time slice of the thread that is
  // currently running and reload the time slice of the thread we
  // activate.  Timeslice bookkeeping is always relative to the thread
  // which owns the timeslice.

  // Check if the old global timeslice is still valid. It may become
  // invalid if the owning thread died and the timeslice died with it.
  if (current_sched())
    {
      LOG_SCHED_SAVE;

      // Save remaining timeslice or refill if the timeslice expired.
      current_sched()->set_ticks_left (timeslice_ticks_left <= 0    ?
                                       current_sched()->timeslice() :
                                       timeslice_ticks_left);

      if (timeslice_ticks_left <= 0)			// Timeslice expired
        {
          Context *owner = current_sched()->owner();

#ifdef CONFIG_PREEMPTION_IPC
          // Fire a preemption event
          current_sched()->set_preemption_time (Kernel_info::kip()->clock);
          owner->preemption_event (current_sched());
#endif

          // Check if this is a real-time Sched_context.
          // Dequeue from old prio, switch timeslice and enqueue with new prio
          if (current_sched() != owner->timesharing_sched())
            {
              owner->ready_dequeue();
              owner->set_sched (current_sched()->next());
              owner->ready_enqueue();
            }

          // Otherwise it is the timesharing Sched_context. No priority change.
          // If we are ready, activate the next thread of the same priority.
          else if (owner->in_ready_list())
            prio_next[current_sched()->prio()] = 
              owner->ready_next->sched()->prio() == current_sched()->prio() ?
                owner->ready_next : prio_first[current_sched()->prio()];
        }
    }

  // Select a thread for scheduling.
  Context *next_to_run;

  for (;;)
    {
      next_to_run = prio_next[prio_highest];
      assert (next_to_run);

      if (next_to_run->state() & Thread_running) 
	break;

      next_to_run->ready_dequeue();

      cpu_lock.clear();
      Proc::irq_chance();
      cpu_lock.lock();
    }
	
  // Load new global timeslice
  set_current_sched (next_to_run->sched());

  LOG_SCHED_LOAD;

  if (next_to_run != this)
    switch_to (next_to_run);

  cpu_lock.clear();
}

/**
 * Timer tick.
 * Schedule if the timeslice has run out or if this tick has woken up a
 * higher-priority thread (reschedule).
 */
IMPLEMENT inline NEEDS ["globals.h"]
void
Context::timer_tick (bool reschedule)
{
  if (!Config::fine_grained_cputime)
    current()->sched()->update_cputime();

  // timeslice_ticks_left can become -1 if thread_switch sets ticks_left
  // to 0 and then a timer interrupt hits before schedule() is called or
  // if the global timeslice has been invalidated due to a thread kill.
  if (--timeslice_ticks_left <= 0 || reschedule)
    current()->schedule();
}

IMPLEMENT inline
Sched_context *
Context::current_sched()
{
  return _current_sched;
}

IMPLEMENT inline
void
Context::set_current_sched (Sched_context *sched)
{
  // Reload global ticks_left from this timeslice. If the global timeslice
  // is being invalidated, also void all remaining ticks.
  timeslice_ticks_left = sched ? sched->ticks_left() : 0;

  // Make this timeslice current
  _current_sched = sched;
}

IMPLEMENT inline
Sched_context *
Context::sched()
{
  return _sched;
}

IMPLEMENT inline
void
Context::set_sched (Sched_context *sched)
{
  _sched = sched;
}

IMPLEMENT inline
Sched_context *
Context::timesharing_sched()
{
  return &_timesharing_sched;
}

// queue operations

// XXX for now, synchronize with global kernel lock
//-

/** Context in ready list? .
    @return true if thread is in ready list
 */
IMPLEMENT inline
Mword
Context::in_ready_list() const
{
  return ready_next != 0;
}

/** Enqueue context in ready list.
 */
IMPLEMENT inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h", "std_macros.h"]
void
Context::ready_enqueue()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  if (EXPECT_FALSE (in_ready_list()))
    return;

  unsigned short prio = sched()->prio();

  if (!prio_first[prio])	prio_first[prio] = this;
  if (!prio_next[prio])		prio_next[prio]  = this;
  if (prio > prio_highest)	prio_highest     = prio;

  // enqueue as the last tcb of this prio, i.e., just before the
  // first tcb of the next prio

  unsigned short i = prio;
  Context *sibling = 0;
  
  do 
    {
      if (++i == 256)
        i = 0;

      sibling = prio_first[i];	// find first tcb of next prio
    }
  while (!sibling);		// loop terminates at least at kernel_thread

  ready_next = sibling;
  ready_prev = sibling->ready_prev;
  sibling->ready_prev = ready_prev->ready_next = this;

  assert (ready_prev->sched()->prio() <= prio ||
          ready_prev->sched()->prio() == prio_highest);
}

/** Remove context from ready list. 
 */
IMPLEMENT inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h", "std_macros.h"]
void
Context::ready_dequeue()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  if (EXPECT_FALSE (!in_ready_list()))
    return;

  unsigned short prio = sched()->prio();

  if (prio_first[prio] == this)
    prio_first[prio] = ready_next->sched()->prio() == prio ?
      ready_next : 0;

  if (prio_next[prio] == this)
    prio_next[prio]  = ready_next->sched()->prio() == prio ?
      ready_next : prio_first[prio];

  if (!prio_first[prio] && prio_highest == prio)
    {
      assert (ready_next->sched()->prio() < prio);
      assert (ready_prev->sched()->prio() < prio);
      prio_highest = ready_prev->sched()->prio();
    }      

  ready_prev->ready_next = ready_next;
  ready_next->ready_prev = ready_prev;
  ready_next /* = ready_prev */ = 0;		// Mark dequeued
}

// needed for the Stack implementation

/** Next stack element.  In a stack of contexts (as maintained by
    switch_lock_t), return the next element. 
    @return next stack element
 */
IMPLEMENT inline
Context *Context::get_next() const
{ 
  return _stack_link;
}

/** Set next stack element.
    @param next next stack element
 */
IMPLEMENT inline
void Context::set_next( Context *next )   
{
  _stack_link = next;
}

/** Donatee.  Context that receives our time slices, for example
    because it has locked us. 
    @return context that should be activated instead of us when we're 
            switch_to()'ed.
*/
IMPLEMENT inline
Context *Context::donatee() const
{
  return _donatee;
}

IMPLEMENT inline
void Context::set_donatee( Context* donatee )
{
  _donatee = donatee;
}

IMPLEMENT inline
void Context::set_kernel_sp(Mword *esp)
{
  kernel_sp = esp;
}

IMPLEMENT inline
Fpu_state * const
Context::fpu_state()
{
  return &_fpu_state;
}

/** Switch to a specific different context.  If that context is currently
    locked, switch to its locker instead (except if current() is the locker).
    @pre current() == this  &&  current() != t
    @param t thread that shall be activated.  
    @return false if the context could not be activated, either because it was
            not runnable or because it was not initialized.
 */
IMPLEMENT
Mword
Context::switch_to(Context *t)
{
  assert (current() != t);
  assert (current() == this);

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Time-slice lending: if t is locked, switch to it's locker
  // instead, this is transitive
  while (t->donatee()                   // target thread not locked
         && t != t->donatee())          // target thread has lock itself
    {
      // Special case for Thread::kill(): If the locker is
      // current(), switch to the locked thread to allow it to
      // release other locks.  Do this only when the target thread
      // actually owns locks.
      if (t->donatee() == current())
        {
          if (t->lock_cnt() > 0)
            break;

          return 0;
        }

      t = t->donatee();
    }

  // Can only switch to running threads!
  if (! (t->state() & Thread_running))  
    return false;

  switch_fpu(t);

  if ((state() & Thread_running) && ! in_ready_list())
    ready_enqueue();

  LOG_CONTEXT_SWITCH;

  return switch_cpu(t);
}
