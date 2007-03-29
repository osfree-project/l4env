INTERFACE: 

#include "lock_guard.h"
#include "switch_lock.h"

#ifdef NO_INSTRUMENT
#undef NO_INSTRUMENT
#endif
#define NO_INSTRUMENT __attribute__((no_instrument_function))

/** A wrapper for Switch_lock that works even when the threading system
    has not been intialized yet.
    This wrapper is necessary because most lock-protected objects are 
    initialized before the threading system has been fired up.
 */
class Helping_lock  
{
  Switch_lock<Switch_lock_valid> _switch_lock;

public:
  static bool threading_system_active;
};

typedef Lock_guard<Helping_lock> Helping_lock_guard;

#undef NO_INSTRUMENT
#define NO_INSTRUMENT

IMPLEMENTATION:

#include "globals.h"
#include "panic.h"
#include "std_macros.h"


/** Threading system activated. */
bool Helping_lock::threading_system_active = false;

/** Constructor. */
PUBLIC inline
Helping_lock::Helping_lock ()
{
  _switch_lock.initialize();
}

/** Acquire the lock with priority inheritance.
    @return true if we owned the lock already.  false otherwise.
 */
PUBLIC 
bool NO_INSTRUMENT
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
PUBLIC inline NEEDS ["panic.h"]
void NO_INSTRUMENT
Helping_lock::lock ()
{
  check ( test_and_set() == false );
}

/** Is lock set?.
    @return true if lock is set.
 */
PUBLIC inline NEEDS["std_macros.h"]
bool NO_INSTRUMENT
Helping_lock::test ()
{
  if (EXPECT_FALSE( ! threading_system_active) ) // still initializing?
    return false;

  return _switch_lock.test();
}

/** Free the lock.
    Return the CPU to helper or next lock owner, whoever has the higher 
    priority, given that thread's priority is higher that our's.
 */
PUBLIC inline NEEDS["std_macros.h"]
void NO_INSTRUMENT
Helping_lock::clear()
{
  if (EXPECT_FALSE( ! threading_system_active) ) // still initializing?
    return;

  _switch_lock.clear();
}

/** Lock owner. 
    @return current owner of the lock.  0 if there is no owner.
 */
PUBLIC inline NEEDS["std_macros.h", "globals.h"]
Context* NO_INSTRUMENT
Helping_lock::lock_owner ()
{
  if (EXPECT_FALSE( ! threading_system_active) ) // still initializing?
    return current();

  return _switch_lock.lock_owner();
}
