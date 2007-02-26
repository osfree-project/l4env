
INTERFACE:

#include "timeout.h"

class Timeslice_timeout : public Timeout
{
};

IMPLEMENTATION:

#include <cassert>
#include "context.h"
#include "globals.h"
#include "sched_context.h"
#include "static_init.h"
#include "std_macros.h"
#include "thread.h"

/* Initialize global valiable timeslice_timeout */
static void timeslice_timeout_init () FIASCO_INIT;

static 
void 
timeslice_timeout_init ()
{ 
  static Timeslice_timeout the_timeslice_timeout;

  timeslice_timeout = &the_timeslice_timeout;
}

STATIC_INITIALIZER (timeslice_timeout_init);

/**
 * Timeout expiration callback function
 * @return true to force a reschedule
 */
PRIVATE
bool
Timeslice_timeout::expired()
{
  Sched_context *sched = Context::current_sched();

  if (sched)
    {
      Thread *owner = Thread::lookup (sched->owner());

      // Ensure sched is owner's current timeslice
      assert (owner->sched() == sched);

      if (sched->id() == 0)		// Timesharing timeslice
        owner->switch_sched (sched);

      else				// Real-time timeslice
        {
          owner->preemption()->queue (Sched_context::Timeslice_overrun,
                                      _wakeup, sched);
          owner->switch_sched (sched->next());
        }
    }

  return true;				// Force reschedule
}
