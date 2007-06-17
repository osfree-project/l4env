
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
#include "std_macros.h"
#include "thread.h"

/* Initialize global valiable timeslice_timeout */
static Per_cpu<Timeslice_timeout> DEFINE_PER_CPU the_timeslice_timeout(true);

PUBLIC
Timeslice_timeout::Timeslice_timeout(unsigned cpu)
{
  timeslice_timeout.cpu(cpu) = this;
}


/**
 * Timeout expiration callback function
 * @return true to force a reschedule
 */
PRIVATE
bool
Timeslice_timeout::expired()
{
  Sched_context *sched = Context::current_sched(current()->cpu());

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
