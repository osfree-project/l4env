INTERFACE:

#include "switch_lock.h"
#include "context.h"		// Switch_hint
#include "panic.h"

enum Switch_hint
{
  SWITCH_ACTIVATE_HIGHER = 0,	// Activate thread with higher priority
  SWITCH_ACTIVATE_LOCKEE,	// Activate thread that was locked
};

/** Thread lock.
    This lock uses the basic priority-inheritance mechanism (Switch_lock)
    and extends it in two ways: First, it has a hinting mechanism that
    allows a locker to specify whether the clear() operation should
    switch to the thread that was locked.  Second, it maintains the current
    locker for Context; this locker automatically gets CPU time allocated
    to the locked thread (Context's ``donatee''); 
    Context::switch_exec() uses that hint.
    To make clear, which stuff in the TCB the lock not protects:
    -the thread state
    -queues
    -the raw kernelstack
    The rest is protected with this lock, this includes the
    kernelstackpointer (kernel_sp).
 */
class Thread_lock
{
private:
  Switch_lock _switch_lock;
  Switch_hint _switch_hint;

  void lock_utcb();
  void unlock_utcb();

};

IMPLEMENTATION:

#include "globals.h"
#include "lock_guard.h"
#include "cpu_lock.h"
#include "thread_state.h"

/** Context this thread lock belongs to.
    @return context locked by this thread lock
 */
PRIVATE inline
Context * const
Thread_lock::context() const
{
  // We could have saved our context in our constructor, but computing
  // it this way is easier and saves space.  We can do this as we know
  // that thread_locks are always embedded in their corresponding
  // context.
  return context_of (this);
}

/** Set switch hint.
    @param hint a hint to the clear() function
 */
PUBLIC inline
void
Thread_lock::set_switch_hint (Switch_hint const hint)
{
  _switch_hint = hint;
}

/** Lock a context.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC
bool const
Thread_lock::test_and_set()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  if (_switch_lock.test_and_set ()) // Get the lock
    return true;

  lock_utcb();  

  context()->set_donatee (current()); // current get time of context

  set_switch_hint (SWITCH_ACTIVATE_HIGHER);

  return false;
}

/** Lock a thread.
    If the lock is occupied, enqueue in list of helpers and lend CPU
    to current lock owner until we are the lock owner.
 */
PUBLIC inline NEEDS["globals.h"]
void
Thread_lock::lock()
{
  check (test_and_set() == false);
}

/** Free the lock.
    First return the CPU to helper or next lock owner, whoever has the higher
    priority, given that thread's priority is higher that ours.
    Finally, switch to locked thread if that thread has a higher priority,
    and/or the switch hint says we should.
 */
PUBLIC
void
Thread_lock::clear()
{
  Switch_hint hint = _switch_hint;	// Save hint before unlocking

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  unlock_utcb();

  // Passing on the thread lock implies both passing _switch_lock
  // and setting context()'s donatee to the new owner.  This must be
  // accomplished atomically.  There are two cases to consider:
  // 1. _switch_lock.clear() does not switch to the new owner.
  // 2. _switch_lock.clear() switches to the new owner.
  // To cope with both cases, we set the owner both here (*) and in
  // Thread_lock::test_and_set().  The assignment that is executed
  // first is always atomic with _switch_lock.clear().  The
  // assignment that comes second is redundant.
  // Note: Our assignment (*) might occur at a time when there is no
  // lock owner or even when the context() has been killed.
  // Fortunately, it works anyway because contexts live in
  // type-stable memory.
  _switch_lock.clear();
  context()->set_donatee (_switch_lock.lock_owner());	// (*)

  // We had locked ourselves, remain current
  if (context() == current())
    return;

  // Unlocked thread not ready, remain current
  if (!(context()->state() & Thread_ready))
    return;

  // Switch to lockee's execution context if the switch hint says so
  if (hint == SWITCH_ACTIVATE_LOCKEE)
    {
      current()->switch_exec_locked (context(), Context::Not_Helping);
      return;
    }

  // Switch to lockee's execution context and timeslice if its priority
  // is higher than the current priority
  if (Context::can_preempt_current (context()->sched()))
    {
      current()->switch_to_locked (context());
      return;
    }

  context()->ready_enqueue();
}

/** Lock owner.
    @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC inline
Context * const
Thread_lock::lock_owner() const
{
  return _switch_lock.lock_owner();
}

/** Is lock set?.
    @return true if lock is set.
 */
PUBLIC inline
bool const
Thread_lock::test()
{
  return _switch_lock.test();
}


//-----------------------------------------------------------------------------
IMPLEMENTATION[!lipc]:

/** Dummy function to hold code in thread_lock generic.
 */
IMPLEMENT inline 
void
Thread_lock::lock_utcb() {}

/** Dummy function to hold code in thread_lock generic.
 */
IMPLEMENT inline 
void
Thread_lock::unlock_utcb() {}


//-----------------------------------------------------------------------------
IMPLEMENTATION[lipc]:

#include "l4_types.h"

/** Set the thread lock in the utcb. That mean's disabling LIPC.
    Should be atomically with the normal thread lock.
 */
IMPLEMENT inline  NEEDS ["l4_types.h", Thread_lock::context]
void
Thread_lock::lock_utcb()
{
  // We need to test for the UTCB, because "fresh" threads can be locked,
  // but they dont have an UTCB.

  if (context()->utcb())
    context()->utcb()->lock_dirty();
}

/** Clear the thread lock in the utcb. That might enable LIPC again.
 */
IMPLEMENT inline NEEDS ["l4_types.h", Thread_lock::context]
void
Thread_lock::unlock_utcb()
{
  // lock free now -> "unlock" UTCB
  // We need to test for the UTCB too, because on "killed" threads, the utcb is 
  // already gone

  if (context()->utcb())
    context()->utcb()->unlock_dirty();
}
