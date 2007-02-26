INTERFACE: 

#include "lock_guard.h"
#include "switch_lock.h"

/** A wrapper for Switch_lock that works even when the threading system
    has not been intialized yet.
    This wrapper is necessary because most lock-protected objects are 
    initialized before the threading system has been fired up.
 */
class Helping_lock  
{
  Switch_lock _switch_lock;

public:
  static bool threading_system_active;
};

typedef Lock_guard<Helping_lock> Helping_lock_guard;

IMPLEMENTATION:

#include "globals.h"
#include "panic.h"

/** Threading system activated. */
bool Helping_lock::threading_system_active = false;

/** Acquire the lock with priority inheritance.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC 
bool 
Helping_lock::test_and_set ()
{
  if (! threading_system_active) // still initializing?
    return false;
  
  if (_switch_lock.test_and_set())
    return true;

  return false;
}

/** Acquire the lock with priority inheritance.
    If the lock is occupied, enqueue in list of helpers and lend CPU 
    to current lock owner until we are the lock owner.
 */
PUBLIC 
void 
Helping_lock::lock ()
{
  check ( test_and_set() == false );
}

/** Is lock set?.
    @return true if lock is set.
 */
PUBLIC inline
bool
Helping_lock::test ()
{
  return _switch_lock.test();
}

/** Free the lock.
    Return the CPU to helper or next lock owner, whoever has the higher 
    priority, given that thread's priority is higher that our's.
 */
PUBLIC 
void
Helping_lock::clear()
{
  if (! threading_system_active) // still initializing?
    return;

  _switch_lock.clear();
}

/** Lock owner. 
    @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC inline
Context*
Helping_lock::lock_owner ()
{
  return _switch_lock.lock_owner();
}
