INTERFACE:

#include "l4_types.h"

class Timeout
{
public:
  /**
   * @brief Timeout constructor.
   */
  Timeout();

  static bool do_timeouts();

  void reset();

  /**
   * @brief Check if timeout is set.
   */
  bool is_set();

  /**
   * @brief Check if timeout has hit.
   */
  bool has_hit();

  void set(Unsigned64 clock);

  void set_again();

  /**
   * Return remaining time of timeout.
   */
  Signed64 get_timeout();

private:

  /**
   * @brief Default copy constructor (is undefined).
   */
  Timeout(const Timeout&);

  /**
   * @brief Enqueue a new timeout.
   */
  void enqueue();

  /**
   * @brief Dequeue an expired timeout.
   * @return true if a reschedule is necessary, false otherwise.
   */
  bool dequeue();

  virtual bool expired() = 0;

  /**
   * @brief Absolute system time we want to be woken up at.
   */
  Unsigned64 _wakeup;

  /**
   * @brief Next/Previous Timeout in timer list
   */
  Timeout *_next, *_prev;

  struct {
    bool set : 1;
    bool hit : 1;
  } _flags;

  static Timeout *first_timeout;
};

IMPLEMENTATION:

#include <cassert>
#include "cpu_lock.h"
#include "kip.h"
#include "lock_guard.h"

Timeout * Timeout::first_timeout = 0;

IMPLEMENT inline
Timeout::Timeout()
{
  _flags.set = _flags.hit = 0;
}

IMPLEMENT inline 
bool 
Timeout::is_set()
{
  return _flags.set;
}

IMPLEMENT inline 
bool 
Timeout::has_hit()
{
  return _flags.hit;
}

IMPLEMENT inline
void
Timeout::enqueue()
{
  _flags.set = 1;

  if (!first_timeout)			// insert as first
    {
      first_timeout = this;
      _prev = _next = 0;
      return;
    }

  Timeout *t;

  for (t = first_timeout;; t = t->_next)
    {
      if (t->_wakeup >= _wakeup)	// insert before t
        {
          _next = t;
          _prev = t->_prev;
          t->_prev = this;
    
          if (_prev)
            _prev->_next = this;
          else
            first_timeout = this;
          
          return;
        }

      if (!t->_next)
        break;
    }

  t->_next = this;			// insert as last after t
  _prev = t;
  _next = 0;
}

IMPLEMENT inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
                        Timeout::is_set, Timeout::enqueue]
void
Timeout::set (Unsigned64 clock)
{
  // XXX uses global kernel lock
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert (!is_set());

  _wakeup = clock;
  enqueue();
}

IMPLEMENT inline NEEDS ["kip.h"]
Signed64
Timeout::get_timeout()
{
  return _wakeup - Kernel_info::kip()->clock;
}

IMPLEMENT inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
                        Timeout::is_set, Timeout::enqueue, Timeout::has_hit]
void 
Timeout::set_again()
{
  // XXX uses global kernel lock
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert(! is_set());
  if (! has_hit())
    {
      enqueue();
    }
}

IMPLEMENT inline NEEDS ["cpu_lock.h", "lock_guard.h", Timeout::is_set]
void 
Timeout::reset()
{
  // XXX uses global kernel lock

  if (!is_set())
    return;			// avoid lock overhead if not set

  {
    Lock_guard<Cpu_lock> guard (&cpu_lock);

    if (is_set())
      {
        if (_prev)
  	  _prev->_next = _next;
        else
  	  first_timeout = _next;
        if (_next)
  	  _next->_prev = _prev;
      }

    _flags.set = 0;
  }
}

IMPLEMENT inline
bool 
Timeout::dequeue()
{
  // XXX assume we run kernel-locked

  first_timeout = _next;		// dequeue
  if (_next)
    _next->_prev = 0;

  _flags.hit = 1;
  _flags.set = 0;

  return expired();
}

IMPLEMENT inline NEEDS ["kip.h", Timeout::dequeue]
bool 
Timeout::do_timeouts()
{
  bool reschedule = false;

  while (first_timeout && Kernel_info::kip()->clock >= first_timeout->_wakeup)
    if (first_timeout->dequeue())
      reschedule = true;

  return reschedule;
}
