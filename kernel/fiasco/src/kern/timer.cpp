INTERFACE:

#include "initcalls.h"

class Context;

class Timer
{
public:
  /**
   * @brief Static constructor for the interval timer.
   *
   * The implementation is platform specific. Two x86 implementations
   * are timer-pit and timer-rtc.
   */
  static void init() FIASCO_INIT;
  
  /**
   * @brief must be set to true in init().
   */
  static bool init_done;

  /**
   * @brief Acknowledges a timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void acknowledge();

  /**
   * @brief Enables the intervall timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void enable();

  /**
   * @brief Disabled the timer IRQ.
   */
  static void disable();
};

class timeout_t
{
  friend class timer;

  timeout_t(const timeout_t&);	// default copy constructor is undefined

  // DATA
  unsigned long long _wakeup;	// absolute time we want to be woken up
  timeout_t *_next, *_prev;	// next/prev in timer queue

  union {
    struct {
      bool set : 1;
      bool hit : 1;
    } _flags;
    unsigned short _flagword;
  };
};

class timer
{
  friend class timeout_t;	// timeouts can add themselves to the timeout
				// queue

  timer();			// default constructors are undefined
  timer(const timer&);

  // implementation details follow
  static timeout_t *first_timeout;
};

IMPLEMENTATION:

#include <cassert>

#include "config.h"
#include "context.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kip.h"
#include "kmem.h"
#include "cpu.h"
#include "lock_guard.h"
#include "thread_state.h"
//#include "static_init.h"

//STATIC_INITIALIZE(Timer);

bool Timer::init_done = false;

// 
// timeout_t
// 

PUBLIC inline 
timeout_t::timeout_t()
{
  _flagword = 0;
}

PUBLIC inline 
bool 
timeout_t::is_set()
{
  return _flags.set;
}

PUBLIC inline 
bool 
timeout_t::has_hit()
{
  return _flags.hit;
}

PUBLIC inline NEEDS ["globals.h"]
Context *
timeout_t::owner()
{
  return context_of (this);
}

inline 
void
timeout_t::enqueue()
{
  _flags.set = 1;
  if (!timer::first_timeout)
    {
      timer::first_timeout = this;
      _prev = _next = 0;
    }
  else
    {
      timeout_t *ti = timer::first_timeout;

      for (;;)
	{
	  if (ti->_wakeup >= _wakeup)
	    {
	      // insert before ti
	      _next = ti;
	      _prev = ti->_prev;
	      ti->_prev = this;
	      if (_prev)
		_prev->_next = this;
	      else
		timer::first_timeout = this;
	      
	      return;
	    }

	  if (! ti->_next)
	    break;

	  ti = ti->_next;
	}

      // insert as last item after ti
      ti->_next = this;
      _prev = ti;
      _next = 0;
    }
}

PUBLIC inline NEEDS [timeout_t::is_set, timeout_t::enqueue,
                     "cpu_lock.h", "lock_guard.h", <cassert>]
void
timeout_t::set(unsigned long long abs_microsec)
{
  // XXX uses global kernel lock
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert(! is_set());
  _wakeup = abs_microsec;
  enqueue();
}

PUBLIC inline NEEDS [timeout_t::is_set, timeout_t::enqueue, timeout_t::has_hit,
		     "cpu_lock.h", "lock_guard.h", <cassert>]
void 
timeout_t::set_again()
{
  // XXX uses global kernel lock
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  assert(! is_set());
  if (! has_hit())
    {
      enqueue();
    }
}

PUBLIC inline NEEDS [timeout_t::is_set, "cpu_lock.h", "lock_guard.h"] 
void 
timeout_t::reset()
{
  // XXX uses global kernel lock

  if (! is_set())
    return;			// avoid lock overhead if not set

  {
    Lock_guard<Cpu_lock> guard (&cpu_lock);

    if (is_set())
      {
        if (_prev)
  	_prev->_next = _next;
        else
  	timer::first_timeout = _next;
        if (_next)
  	_next->_prev = _prev;
      }

    _flags.set = 0;

  }
}

inline NEEDS ["kip.h", "context.h", "thread_state.h"]
bool 
timeout_t::check_hit()
{
  // XXX assume we run kernel-locked

  if (Kernel_info::kip()->clock < _wakeup)
    return false;

  timer::first_timeout = _next;		// dequeue
  if (_next) _next->_prev = 0;
  _flags.hit = 1;
  _flags.set = 0;
  owner()->state_change(~Thread_ipc_in_progress, 
			Thread_running); // set thread running

  // XXX don't we want to switch immediately if the newly woken-up
  // thread has a higher prio?

  owner()->ready_enqueue();

  return true;
}

PUBLIC inline
long long
timeout_t::get_timeout()
{
  return _wakeup - Kernel_info::kip()->clock;
}

// 
// timer
// 

// timer class variables
timeout_t *timer::first_timeout = 0;

PROTECTED
static inline NEEDS [timeout_t::check_hit]
void 
timer::do_timeouts()
{
  while (first_timeout)
    {
      if (! first_timeout->check_hit())
	break;
    }
}


//
// helpers
//
#if 0
inline 
unsigned long long
timeout_to_microsec(unsigned man, unsigned exp)
{
  return static_cast<unsigned long long>(man) << ((15 - exp) << 1);
}
#endif
