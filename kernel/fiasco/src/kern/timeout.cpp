INTERFACE:

#include "l4_types.h"

class Timeout
{
  friend class Jdb_timeout_list;
  friend class Jdb_list_timeouts;

public:
  /**
   * Timeout constructor.
   */
  Timeout();

  static bool do_timeouts();

  void reset();

  /**
   * Check if timeout is set.
   */
  bool is_set();

  /**
   * Check if timeout has hit.
   */
  bool has_hit();

  void set(Unsigned64 clock);

  void set_again();

  static Timeout* Timeout::get_first_timeout();

  /**
   * Return remaining time of timeout.
   */
  Signed64 get_timeout();

protected:
  /**
   * Absolute system time we want to be woken up at.
   */
  Unsigned64 _wakeup;

private:
  /**
   * Default copy constructor (is undefined).
   */
  Timeout(const Timeout&);

  /**
   * Enqueue a new timeout.
   */
  void enqueue();

  /**
   * Dequeue an expired timeout.
   * @return true if a reschedule is necessary, false otherwise.
   */
  bool dequeue();

  virtual bool expired() = 0;

  /**
   * Next/Previous Timeout in timer list
   */
  Timeout *_next, *_prev;

  struct {
    bool     set  : 1;
    bool     hit  : 1;
    unsigned res  : 6; // performance optimization
  } _flags;

  static Timeout *first_timeout;
};

IMPLEMENTATION:

#include <cassert>
#include "cpu_lock.h"
#include "kip.h"
#include "lock_guard.h"
#include "timer.h"

Timeout * Timeout::first_timeout = 0;

IMPLEMENT inline
Timeout::Timeout()
{
  _flags.set  = _flags.hit = 0;
  _flags.res  = 0;
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
Timeout*
Timeout::get_first_timeout()
{
  return first_timeout;
}

IMPLEMENT inline NEEDS["timer.h"]
void
Timeout::enqueue()
{
  _flags.set = 1;

  if (!first_timeout)			// insert as first
    {
      first_timeout = this;
      _prev = _next = 0;
      Timer::update_timer(_wakeup);
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
	    {
	      first_timeout = this;
	      Timer::update_timer(_wakeup);
	    }
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
			Timeout::enqueue, Timeout::is_set]
void
Timeout::set (Unsigned64 clock)
{
  // XXX uses global kernel lock
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert (!is_set());

  _wakeup = clock;
  enqueue();
}

IMPLEMENT inline NEEDS ["timer.h"]
Signed64
Timeout::get_timeout()
{
  return _wakeup - Timer::system_clock();
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

IMPLEMENT inline NEEDS ["cpu_lock.h", "lock_guard.h", "timer.h", 
			Timeout::is_set]
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
	  {
	    first_timeout = _next;
	    if (_next)
	      Timer::update_timer(_next->_wakeup);
	  }
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

IMPLEMENT inline NEEDS ["kip.h", "timer.h", Timeout::dequeue]
bool 
Timeout::do_timeouts()
{
  bool reschedule = false;

  // timer interrupt handler synchronized clock before calling 
  // Thread::handle_timer_interrupt which in turn called us
  while (first_timeout && (Kip::k()->clock >= first_timeout->_wakeup))
    if (first_timeout->dequeue())
      reschedule = true;

  // After dequeueing all expired timeouts, program next event, if any
  if (first_timeout)
    Timer::update_timer (first_timeout->_wakeup);

  return reschedule;
}
