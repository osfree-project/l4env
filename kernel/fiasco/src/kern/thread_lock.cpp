INTERFACE:

#include "switch_lock.h"
#include "context.h"		// Switch_hint
#include "panic.h"

enum Switch_hint
{
  SWITCH_DONT_CARE = 0,
  SWITCH_ACTIVATE_RECEIVER,
  SWITCH_ACTIVATE_SENDER
};

/** Thread lock.  
    This lock uses the basic priority-inheritance mechanism (Switch_lock)
    and extends it in two ways: First, it has a hinting mechanism that
    allows a locker to specify whether the clear() operation should 
    switch to the thread that was locked.  Second, it maintains the current
    locker for Context; this locker automatically gets CPU time allocated
    to the locked thread (Context's ``donatee''); 
    Context::switch_to() uses that hint.
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
Context* 
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
Thread_lock::set_switch_hint(Switch_hint hint)
{
  _switch_hint = hint;
}

/** Lock a context.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC
bool
Thread_lock::test_and_set()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (_switch_lock.test_and_set ()) // Get the lock
    return true;

  context()->set_donatee (current()); // current get time of context

  set_switch_hint (SWITCH_DONT_CARE);

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
  check ( test_and_set () == false );
}

/** Free the lock.
    First return the CPU to helper or next lock owner, whoever has the higher 
    priority, given that thread's priority is higher that our's.
    Finally, switch to locked thread if that thread has a higher priority,
    and/or the switch hint says we should.
 */
PUBLIC
void
Thread_lock::clear()
{
  Switch_hint hint = _switch_hint; // Save hint before unlocking.

  {
    Lock_guard<Cpu_lock> guard (&cpu_lock);

    // Reset donatee before clearing switch lock: In
    // _switch_lock.clear(), we might atomically (kernel lock is held)
    // switch to a new lock owner which overwrites the donatee.
    context()->set_donatee (0);
    _switch_lock.clear();
  }

  // switch back so sender of request if we're not ready to run
  // and higher priorized
  if (context() != current())	// it's not ourself -- could switch back
    {
      if ((context()->state() & Thread_running)) // able to run?
	{
	  if ((hint == SWITCH_ACTIVATE_RECEIVER
	       || (! test()	// more lockers left -- not ready to run
		   && current()->sched()->prio() < context()->sched()->prio()))
	      
	      && (hint != SWITCH_ACTIVATE_SENDER))
	    {
	      // then switch to receiver of the last locked operation
	      current()->switch_to(context());
	    }
	  else if (! context()->in_ready_list())
	    {
	      context()->ready_enqueue();
	    }
	}
    }
}

/** Lock owner. 
    @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC inline 
Context *
Thread_lock::lock_owner() const
{
  return _switch_lock.lock_owner();
}

/** Is lock set?.
    @return true if lock is set.
 */
PUBLIC inline 
bool
Thread_lock::test()
{
  return _switch_lock.test();
}
