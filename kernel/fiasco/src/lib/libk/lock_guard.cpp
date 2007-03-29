INTERFACE:

// 
// Lock_guard: a guard object using a lock such as helping_lock_t
// 
template<typename LOCK>
class Lock_guard
{
  LOCK *_lock;
};

IMPLEMENTATION:

PUBLIC template<typename LOCK>
inline
Lock_guard<LOCK>::Lock_guard()
  : _lock(0)
{}

PUBLIC template<typename LOCK>
inline
Lock_guard<LOCK>::Lock_guard(LOCK *l)
  : _lock(l)
{
  if (_lock->test_and_set())
    _lock = 0;			// Was locked already -- avoid unlock.
}

PUBLIC template<typename LOCK>
inline
bool
Lock_guard<LOCK>::lock(LOCK *l)
{
  switch (l->test_and_set())
    {
    case LOCK::Locked:
      return true;
    case LOCK::Not_locked:
      _lock = l;			// Was not locked -- unlock.
      return true;
    default:
      return false; // Error case -- lock not existent
    }
}

PUBLIC template<typename LOCK>
inline
Lock_guard<LOCK>::~Lock_guard()
{
  if (_lock)
    _lock->clear();
}

PUBLIC template<typename LOCK>
inline
bool
Lock_guard<LOCK>::was_set(void)
{
  return !_lock;
}
