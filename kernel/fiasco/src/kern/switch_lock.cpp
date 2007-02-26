INTERFACE:

#include "stack.h"

class Context;

/** A lock that implements priority inheritance.
 */
class Switch_lock
{
  Context *_lock_owner;
  Stack<Context> _lock_stack; 
};


IMPLEMENTATION:

#include <cassert>

#include "atomic.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "context.h"
#include "globals.h"

// 
// Switch_lock inlines 
// 

/** Constructor. */
PUBLIC inline
Switch_lock::Switch_lock()
  : _lock_owner(0)
{
}

/** Lock owner. 
    @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC inline 
Context *
Switch_lock::lock_owner() const
{
  return _lock_owner;
}

/** Is lock set?.
    @return true if lock is set.
 */
PUBLIC inline 
bool 
Switch_lock::test() const
{
  return _lock_owner;
}

/** Try to acquire the lock.
    @return true if successful: current context is now the owner of the lock.
            false if lock has previously been set.  Returns false even
	    if the current context is already the lock owner.
 */
inline NEEDS["atomic.h"]
bool
Switch_lock::try_lock()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  bool ret = smp_cas(&_lock_owner, 
		     static_cast<Context*>(0), 
		     current());

  if (ret)
    current()->inc_lock_cnt();	// Do not lose this lock if current is deleted

  return ret;
}

/** Acquire the lock with priority inheritance.
    If the lock is occupied, enqueue in list of helpers and lend CPU 
    to current lock owner until we are the lock owner.
 */
PUBLIC
void
Switch_lock::lock()
{
  Context* locker = current();

  while (! try_lock())
    {
      _lock_stack.insert(locker);

      for(;;)
	{
	  // do all the comparisons sequentially , the current owner
	  // might interfere in clear otherwise
	  Lock_guard<Cpu_lock> guard (&cpu_lock);

	  // XXX which kind of interference, what about SMP (cpu local lock
	  // or global kernel lock ?)
	  if (!test())
	    {			// surprise: lock is now free
	      if(_lock_stack.dequeue(locker))
		{
		  break;
		}
	    }
	  if (_lock_owner == locker)
	    {
	      goto out;
	    }

	  current()->switch_to(_lock_owner);
	}
    }
 out:

  assert (_lock_owner == current());
}

/** Acquire the lock with priority inheritance.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC
bool
Switch_lock::test_and_set()
{
  if (_lock_owner == current ())
    return true;

  lock();

  return false;
}

/** Free the lock.
    Return the CPU to helper or next lock owner, whoever has the higher 
    priority, given that thread's priority is higher that our's.
 */
PUBLIC
void
Switch_lock::clear()
{
  assert (current() == _lock_owner);

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  _lock_owner = _lock_stack.dequeue();

  if (_lock_owner)
    _lock_owner->inc_lock_cnt();

  current()->dec_lock_cnt();

  Context* next_to_run = current(); // Whom should we switch to?

  // If lock count drops to 0, see if someone has locked us.  (If that
  // is the case, he wants to delete us.)  Switch to our lock owner.
  if (current()->lock_cnt() == 0)
    {
      Context* owner = current()->donatee();
      if (owner && owner != current())
	next_to_run = owner;
    }

  // Is there a new lock owner?  Has he a higher priority?
  if (_lock_owner 
      && _lock_owner->sched()->prio() > next_to_run->sched()->prio())
    {
      next_to_run = _lock_owner;
    }

  if (next_to_run != current())
    current()->switch_to(next_to_run);
}

