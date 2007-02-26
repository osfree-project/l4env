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
Lock_guard<LOCK>::Lock_guard(LOCK *l)
  : _lock(l)
{
  if (_lock->test_and_set())
    _lock = 0;			// Was locked already -- avoid unlock.
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
