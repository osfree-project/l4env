INTERFACE:

class Context;

/** A lock that implements priority inheritance.
 */
class Switch_lock
{
  // Warning: This lock's member variables must not need a
  // constructor.  Switch_lock instances must assume
  // zero-initialization or be initialized using the initialize()
  // member function.
  // Reason: to avoid overwriting the lock in the thread-ctor
  Context *_lock_owner;
};


IMPLEMENTATION:

#include <cassert>

#include "cpu.h"
#include "atomic.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "context.h"
#include "globals.h"
#include "processor.h"

// 
// Switch_lock inlines 
// 

/** Initialize Switch_lock.  Call this function if you cannot
    guarantee that your Switch_lock instance is allocated from
    zero-initialized memory. */
PUBLIC inline
void
Switch_lock::initialize()
{
  _lock_owner = 0;
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

  bool ret = cas (&_lock_owner, static_cast<Context*>(0), current());

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
  while (!try_lock())
    {
      // do all the comparisons sequentially, the current owner
      // might interfere in clear otherwise
      //
      // XXX which kind of interference, what about SMP (cpu local lock
      // or global kernel lock ?)

      Lock_guard <Cpu_lock> guard (&cpu_lock);

      // Help lock owner until lock becomes free
      while (test())
	current()->switch_exec_locked (_lock_owner, Context::Helping);	
    }

  assert (_lock_owner == current());
}

/** Acquire the lock with priority inheritance.
    If the lock is occupied, enqueue in list of helpers and lend CPU 
    to current lock owner until we are the lock owner.
    @pre caller holds cpu lock
 */
PUBLIC inline NEEDS["cpu.h","context.h", "processor.h"]
void
Switch_lock::lock_dirty()
{
  assert(cpu_lock.test());

  // have we already the lock?
  if(_lock_owner == current())
    return;


  while (test())
    {
      assert(cpu_lock.test());

      // Help lock owner until lock becomes free
      //      while (test())
       current()->switch_exec_locked (_lock_owner, Context::Helping);

      Proc::irq_chance();
    }

  _lock_owner = current();

  current()->inc_lock_cnt();   // Do not lose this lock if current is deleted
}


/** Acquire the lock with priority inheritance.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC inline NEEDS["globals.h"]
bool
Switch_lock::test_and_set()
{
  if (_lock_owner == current())
    return true;

  lock();

  return false;
}

/** Acquire the lock with priority inheritance
    @return true if we owned the lock already.  false otherwise.
    @pre caller holds cpu lock
 */
PUBLIC inline NEEDS["globals.h"]
bool
Switch_lock::test_and_set_dirty()
{
  if (_lock_owner == current())
    return true;

  lock_dirty();

  return false;
}


/** Free the lock.  
    Return the CPU to helper if there is one, since it had to have a
    higher priority to be able to help (priority may be its own, it
    may run on a donated timeslice or round robin scheduling may have
    selected a thread on the same priority level as me)
 */
PUBLIC
void
Switch_lock::clear()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert (current() == _lock_owner);

  Context * h = _lock_owner->helper();
  Context * owner = _lock_owner;

  _lock_owner = 0;
  owner->dec_lock_cnt();

  /*
   * If someone helped us by lending its time slice to us.
   * Just switch back to the helper without changing its helping state.
   */
  if (h != owner)
    owner->switch_exec_locked (h, Context::Ignore_Helping);

  /*
   * Someone apparently tries to delete us. Therefore we aren't
   * allowed to continue to run and therefore let the scheduler
   * pick the next thread to execute. 
   */
  if (owner->lock_cnt() == 0 && owner->donatee()) 
    owner->schedule ();
}

/** Free the lock.  
    Return the CPU to helper if there is one, since it had to have a
    higher priority to be able to help (priority may be its own, it
    may run on a donated timeslice or round robin scheduling may have
    selected a thread on the same priority level as me).
    If _lock_owner is 0, then this is a no op
    @pre caller holds cpu lock
 */
PUBLIC inline
void
Switch_lock::clear_dirty()
{
  assert(cpu_lock.test());

  assert (current() == _lock_owner);

  Context * h = _lock_owner->helper();
  Context * owner = _lock_owner;

  _lock_owner = 0;
  owner->dec_lock_cnt();

  if (h != owner)
      owner->switch_exec_locked (h, Context::Ignore_Helping);

  if (owner->lock_cnt() == 0 && owner->donatee())
      owner->schedule ();

}
