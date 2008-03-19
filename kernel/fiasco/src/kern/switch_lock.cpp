INTERFACE:

#ifdef NO_INSTRUMENT
#undef NO_INSTRUMENT
#endif
#define NO_INSTRUMENT __attribute__((no_instrument_function))

class Context;

/**
 * @brief Default validity checker for Switch_lock.
 * 
 * This validity checker returns true for ever call. It is used as default 
 * for the Switch_lock<>.
 */
class Switch_lock_valid
{
};

/** A lock that implements priority inheritance.
 *
 * The lock uses a validity checker (VALID) for doing an existence check
 * before the lock is actually acquired. With this mechanism the lock itself
 * may disappear while it is locked (see clear_no_switch_dirty() and 
 * switch_dirty()), even if it is under contention. When the lock no longer
 * exists VALID::valid(void const *lock) must return false, true if it
 * exists (see Switch_lock_valid). This mechanism is used in Thread_lock
 * when thread control blocks are deallocated.
 *
 * The operations lock(), lock_dirty(), try_lock(), test(), test_and_set(),
 * and test_and_set_dirty() may return #Invalid if the lock does 
 * no longer exist.
 *
 * The operations initialize(), lock_owner(), clear(), clear_dirty(), and
 * clear_no_switch_dirty() must not being called on and invalid lock,
 * thus the lock itself must be held for using these operations. 
 * (Except initialize(), which is only useful for locks that are always 
 * valid.)
 * 
 * @param VALID must be set to a validity checker for the lock.
 * 
 * The validity checker is used while aquiring the lock to test
 * if the lock itself existis. We assume that a lock may disappear
 * while we are blocked on it.
 */
template< typename VALID = Switch_lock_valid >
class Switch_lock
{
  // Warning: This lock's member variables must not need a
  // constructor.  Switch_lock instances must assume
  // zero-initialization or be initialized using the initialize()
  // member function.
  // Reason: to avoid overwriting the lock in the thread-ctor
  Context *_lock_owner;

public:
  /**
   * @brief The result type of lock operations.
   */
  enum Lock_res 
  { 
    Not_locked, ///< The lock was formerly not aquired and -- we got it
    Locked,     ///< The lock was already aquired by ourselves
    Invalid     ///< The lock does not exist (is invalid)
  };

  /**
   * Stores the context of the lock for a later switch.
   * (see clear_no_switch_dirty(), switch_dirty())
   */
  struct Lock_context
  {
    Context *owner;
  };
};

#undef NO_INSTRUMENT
#define NO_INSTRUMENT 


IMPLEMENTATION:

#include <cassert>

#include "cpu.h"
#include "atomic.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "context.h"
#include "globals.h"
#include "processor.h"


PUBLIC
static inline bool NO_INSTRUMENT 
Switch_lock_valid::valid(void const *) 
{ return true; }

// 
// Switch_lock inlines 
//

/**
 * Test if the lock is valid (uses the validity checker).
 * @return true if the lock really exists, false if not.
 */
PUBLIC template<typename VALID> 
inline
bool NO_INSTRUMENT
Switch_lock<VALID>::valid() const
{
  return VALID::valid(this);
}

/** Initialize Switch_lock.  Call this function if you cannot
    guarantee that your Switch_lock instance is allocated from
    zero-initialized memory. */
PUBLIC template<typename VALID> 
inline
void NO_INSTRUMENT
Switch_lock<VALID>::initialize()
{
  _lock_owner = 0;
}

/** Lock owner. 
 *  @pre The lock must be valid (see valid()).
 *  @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC template<typename VALID> 
inline 
Context * NO_INSTRUMENT
Switch_lock<VALID>::lock_owner() const
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  return (EXPECT_TRUE(valid())) ? _lock_owner : 0;
}

/** Is lock set?.
    @return #Locked if lock is set, #Not_locked if not locked, and #Invalid if 
            the lock does not exist (see valid()).
 */
PUBLIC template<typename VALID> 
inline 
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::test() const
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  if (EXPECT_FALSE(!valid()))
    return Invalid;
  return _lock_owner ? Locked : Not_locked;
}

/** Try to acquire the lock.
    @return #Locked if successful: current context is now the owner of the lock.
            #Not_locked if lock has previously been set.  Returns Not_locked
	    even if the current context is already the lock owner.
	    The result is #Invalid if the lock does not exist (see valid()).
 */
template<typename VALID> 
inline NEEDS["atomic.h"]
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::try_lock()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (EXPECT_FALSE(!valid()))
    return Invalid;

  bool ret = cas (&_lock_owner, static_cast<Context*>(0), current());

  if (ret)
    current()->inc_lock_cnt();	// Do not lose this lock if current is deleted

  return ret ? Locked : Not_locked;
}

/** Acquire the lock with priority inheritance.
 *  If the lock is occupied, enqueue in list of helpers and lend CPU 
 *  to current lock owner until we are the lock owner.
 *  @return #Locked if the lock was already locked by the current context.
 *          #Not_locked if the current context got the lock (the usual case).
 *          #Invalid if the lock does not exist (see valid()).
 */
PUBLIC template<typename VALID> 
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::lock()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);
  return lock_dirty();
}

/** Acquire the lock with priority inheritance.
 *  If the lock is occupied, enqueue in list of helpers and lend CPU 
 *  to current lock owner until we are the lock owner (see lock()).
 *  @pre caller holds cpu lock
 *  @return #Locked if the lock was already locked by the current context.
 *          #Not_locked if the current context got the lock (the usual case).
 *          #Invalid if the lock does not exist (see valid()).
 */
PUBLIC template<typename VALID> 
inline NEEDS["cpu.h","context.h", "processor.h"]
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::lock_dirty()
{
  assert(cpu_lock.test());

  if (!valid())
    return Invalid;

  // have we already the lock?
  if(_lock_owner == current())
    return Locked;


  while (test())
    {
      assert(cpu_lock.test());

      // Help lock owner until lock becomes free
      //      while (test())
      current()->switch_exec_locked (_lock_owner, Context::Helping);

      Proc::irq_chance();

      if (!valid())
	return Invalid;
    }

  _lock_owner = current();

  current()->inc_lock_cnt();   // Do not lose this lock if current is deleted
  return Not_locked;
}


/** Acquire the lock with priority inheritance.
 *  @return #Locked if we owned the lock already.  #Not_locked otherwise.
 *          #Invalid is returned if the lock does not exist (see valid()).
 */
PUBLIC template<typename VALID> 
inline NEEDS["globals.h"]
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::test_and_set()
{
  return lock();
}

/** Acquire the lock with priority inheritance (see test_and_set()).
 *  @return #Locked if we owned the lock already.  #Not_locked otherwise.
 *          #Invalid is returned if the lock does not exist (see valid()).
 *          @pre caller holds cpu lock
 */
PUBLIC template<typename VALID> 
inline NEEDS["globals.h"]
typename Switch_lock<VALID>::Lock_res NO_INSTRUMENT
Switch_lock<VALID>::test_and_set_dirty()
{
  return lock_dirty();
}

/**
 * Clear the lock, however do not switch to a potential helper yet.
 * This function is used when the lock must be cleared and the object
 * containing the lock will be deallocated atomically. We have to do the
 * switch later using switch_dirty(Lock_context).
 * @return the context for a later switch_dirty()
 * @pre The lock must be valid (see valid()).
 * @pre caller hold cpu lock
 * @post switch_dirty() must be called in the same atomical section
 */
PROTECTED template<typename VALID>
inline
typename Switch_lock<VALID>::Lock_context NO_INSTRUMENT
Switch_lock<VALID>::clear_no_switch_dirty()
{
  Lock_context c;
  c.owner = _lock_owner;
  assert (current() == _lock_owner);
  _lock_owner = 0;
  c.owner->dec_lock_cnt();
  return c;
}

/**
 * Do the switch part of clear() after a clear_no_switch_dirty().
 * This function does not touch the lock itself (may be called on 
 * an invalid lock).
 * @param c the context returned by a former clear_no_switch_dirty().
 * @pre must be called atomically with clear_no_switch_dirty(),
 *      under the same cpu lock
 */
PROTECTED template<typename VALID>
static inline
void NO_INSTRUMENT
Switch_lock<VALID>::switch_dirty(Lock_context const &c) 
{
  Context *h = c.owner->helper();
  /*
   * If someone helped us by lending its time slice to us.
   * Just switch back to the helper without changing its helping state.
   */
  if (h != c.owner)
    c.owner->switch_exec_locked (h, Context::Ignore_Helping);

  /*
   * Someone apparently tries to delete us. Therefore we aren't
   * allowed to continue to run and therefore let the scheduler
   * pick the next thread to execute. 
   */
  if (c.owner->lock_cnt() == 0 && c.owner->donatee()) 
    c.owner->schedule ();
}

/** Free the lock.  
    Return the CPU to helper if there is one, since it had to have a
    higher priority to be able to help (priority may be its own, it
    may run on a donated timeslice or round robin scheduling may have
    selected a thread on the same priority level as me)

    @pre The lock must be valid (see valid()).
 */
PUBLIC template<typename VALID> 
void NO_INSTRUMENT
Switch_lock<VALID>::clear()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  switch_dirty(clear_no_switch_dirty());
}

/** Free the lock.  
    Return the CPU to helper if there is one, since it had to have a
    higher priority to be able to help (priority may be its own, it
    may run on a donated timeslice or round robin scheduling may have
    selected a thread on the same priority level as me).
    If _lock_owner is 0, then this is a no op

    @pre The lock must be valid (see valid())
    @pre caller holds cpu lock
 */
PUBLIC template<typename VALID> 
inline
void NO_INSTRUMENT
Switch_lock<VALID>::clear_dirty()
{
  assert(cpu_lock.test());

  switch_dirty(clear_no_switch_dirty());
}

