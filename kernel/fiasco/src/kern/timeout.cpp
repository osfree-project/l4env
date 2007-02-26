INTERFACE:

#include "l4_types.h"

/** A timeout basic object. It contains the necessary queues and handles
    enqueuing, dequeuing and handling of timeouts. Real timeout classes
    should overwrite expired(), which will do the real work, if an
    timeout hits.
 */
class Timeout
{
  friend class Jdb_timeout_list;
  friend class Jdb_list_timeouts;

public:
  /**
   * Timeout constructor.
   */
  Timeout();

  /**
   * Handles the timeouts, i.e. call expired() for the expired timeouts
   * and programs the "oneshot timer" to the next timeout.
   * @return true if a reschedule is necessary, false otherwise.
   */
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

  static Timeout* get_first_timeout(int index = 0);

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

  /**
   * Overwritten timeout handler function.
   * @return true if a reschedule is necessary, false otherwise.
   */
  virtual bool expired();

  struct {
    bool     set  : 1;
    bool     hit  : 1;
    unsigned res  : 6; // performance optimization
  } _flags;

  /**
   * Next/Previous Timeout in timer list
   */
  Timeout *_next, *_prev;

  /**
   * Timeout queue count (2^n) and  distance between two queues in 2^n.
   */  
  enum {
    Wakeup_queue_count	  = 8,
    Wakeup_queue_distance = 12 // i.e. (1<<12)us
  };

  /**
   * The timeout queues.
   */
  static Timeout first_timeout[Wakeup_queue_count];

  /**
   * The current programmed timeout.
   */
  static Unsigned64 current_set;
};

IMPLEMENTATION:

#include <cassert>
#include "cpu_lock.h"
#include "kip.h"
#include "lock_guard.h"
#include "timer.h"
#include <cstdio>
#include "static_init.h"
#include <climits>
#include "config.h"
#include "kdb_ke.h"


Timeout Timeout::first_timeout[Wakeup_queue_count];

Unsigned64 Timeout::current_set = (Unsigned64) ULONG_LONG_MAX;


IMPLEMENT inline
Timeout::Timeout()
{
  _flags.set  = _flags.hit = 0;
  _flags.res  = 0;
}


/**
 * Initializes an timeout object.
 */
PUBLIC inline  NEEDS[<climits>]
void
Timeout::init()
{
  _next = _prev = this;
  _wakeup = ULONG_LONG_MAX;
}

/* Yeah, i know, an derived and specialized timeout class for
   the root node would be nicer. I already had done this, but
   it was significantly slower than this solution */
IMPLEMENT virtual bool
Timeout::expired()
{
  kdb_ke("Wakeup List Terminator reached");
  return false;  
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
Timeout::get_first_timeout(int index)
{
  return &first_timeout[index & (Wakeup_queue_count-1)];
}


/* Hazelnut uses an unsortet queue, this is fast in enqueuing and dequeue,
   but slow in finding the next programmable timeout.
*/
IMPLEMENT inline NEEDS[<cassert>, "timer.h", "config.h", "kdb_ke.h"]
void
Timeout::enqueue()
{
  int queue = (_wakeup >> Wakeup_queue_distance) & (Wakeup_queue_count-1);

  _flags.set = 1;

  Timeout *tmp = get_first_timeout(queue);
  
  for(;tmp->_next != get_first_timeout(queue+1)
	&& tmp->_next->_wakeup < _wakeup
	; tmp = tmp->_next);

  _next = tmp->_next;
  tmp->_next = this;

  _prev = tmp;
  _next->_prev = this;


  if (Config::scheduler_one_shot && (_wakeup <= current_set))
    {
      current_set = _wakeup;
      Timer::update_timer(_wakeup);  
    }
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
  if (has_hit())
    return;
  
  enqueue();
}

IMPLEMENT inline NEEDS ["cpu_lock.h", "lock_guard.h", "timer.h", 
			Timeout::is_set]
void 
Timeout::reset()
{
  if (EXPECT_FALSE(!is_set()))
    return;			// avoid lock overhead if not set

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  _next->_prev = _prev;
  _prev->_next = _next;

  _flags.set = 0;

  // Normaly we should reprogramm the timer in one shot mode
  // But we let the timer interrupt handler to do this "lazily", to save cycles
}

IMPLEMENT inline
bool 
Timeout::dequeue()
{
  // XXX assume we run kernel-locked

  _next->_prev = _prev;
  _prev->_next = _next;

  _flags.hit = 1;
  _flags.set = 0;

  return expired();
}

IMPLEMENT inline NEEDS [<cassert>, <climits>, "kip.h", "timer.h", "config.h", 
			Timeout::dequeue]
bool 
Timeout::do_timeouts()
{
  
  static Unsigned64 old_clock = 0ULL;
  bool reschedule = false;

  // We test if the time between 2 activiations of this function is greater
  // than the distance between two timeout queues.
  // To avoid to lose timeouts, we calculating the missed queues and
  // scan them too.
  // This can only happen, if we dont enter the timer interrupt for a long
  // time, usual with one shot timer.
  // Because we initalize old_dequeue_time with zero,
  // we can get an "miss" on the first timerinterrupt.
  // But this is booting the system, which is uncritical.
  
  // Calculate which timeout queues needs to be checked.
  int start = (old_clock >> Wakeup_queue_distance);
  int diff  = (Kip::k()->clock >> Wakeup_queue_distance) - start;
  int end   = (start + diff + 1) & (Wakeup_queue_count -1);

  // wrap around 
  start = start &  (Wakeup_queue_count -1);

  // test if an complete miss
  if(diff >= Wakeup_queue_count)
    start = end = 0; // scan all queues

  // update old_clock for the next run
  old_clock = Kip::k()->clock;

  // ensure we always terminate
  assert((end >=0) && (end < Wakeup_queue_count));    
  
  for (;;)
    {
      
      Timeout *timeout = get_first_timeout(start)->_next;

      // now scan this queue for timeouts below current clock
      while ((timeout != get_first_timeout(start+1)))
	{
	  Timeout *tmp = timeout->_next;

	  if (!timeout->_wakeup || timeout->_wakeup > (Kip::k()->clock))
	    break;

	  
	  if (timeout->dequeue())
	    reschedule = true;

	  timeout = tmp;  
	}
      
      // next queue
      start = (start + 1) & (Wakeup_queue_count - 1);

      if (start == end)
	break;
    }

  if (Config::scheduler_one_shot)
    {
      // scan all queues for the next minimum
      current_set = (Unsigned64) ULONG_LONG_MAX;
      bool update_timer = false;
      
      for(int i=0; i< Wakeup_queue_count; i++)
	{
	  // make sure that something enqueued other than the dummy element
	  if(get_first_timeout(i)->_next == get_first_timeout(i+1))
	    continue;
	  
	  update_timer = true;

	  if(get_first_timeout(i)->_next->_wakeup <current_set)
	    current_set =  get_first_timeout(i)->_next->_wakeup;
	}
      
      if(update_timer)
	Timer::update_timer (current_set);

    }
  return reschedule;
}


/**
 * Initialize the timeout queues, i.e. "enqueue" the terminator element.
 */
PUBLIC static inline
void Timeout::init_queues()
{
  for(int i=0; i< Timeout::Wakeup_queue_count; i++)
    {
      first_timeout[i]._next = get_first_timeout(i+1);
      first_timeout[i]._prev = get_first_timeout(i-1);
      first_timeout[i]._wakeup =  0;      
      //      Timeout::first_timeout[i].init();
    }
}

PUBLIC static inline
bool Timeout::is_root_node(Address addr)
{
  if((addr >= (Address) &first_timeout) 
      && (addr <= (Address)&first_timeout + sizeof(first_timeout)))
      return true;
  return false;
}

/* Initialize global valiable timeslice_timeout */
static void timeout_init () FIASCO_INIT;

/**
 * Wrapper function for Timeout::init_queues.
 */
static 
void 
timeout_init ()
{
  Timeout::init_queues();
}

STATIC_INITIALIZER_P (timeout_init, STARTUP_INIT_PRIO);
