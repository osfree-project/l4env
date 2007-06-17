
INTERFACE:

#include "timeout.h"

class Context;
class Preemption;

class Deadline_timeout : public Timeout
{
  friend class Jdb_list_timeouts;

private:
  Preemption * const _preemption;	///< Preemption role notified for P-IPC
};

IMPLEMENTATION:

#include "context.h"
#include "globals.h"
#include "preemption.h"
#include "sched_context.h"
#include "std_macros.h"
#include "thread_state.h"

/**
 * Deadline_timeout constructor
 * @param preemption Preemption role used for sending preemption IPC
 */
PUBLIC inline
Deadline_timeout::Deadline_timeout (Preemption * const preemption)
                : _preemption (preemption)
{}

PRIVATE inline NEEDS ["globals.h"]
Context *
Deadline_timeout::owner() const
{
  // We could have saved our context in our constructor, but computing
  // it this way is easier and saves space. We can do this as we know
  // that Deadline_timeouts are always aggregated in their owner context.

  return context_of (this);
}

/**
 * Timeout expiration callback function
 * @return true if reschedule is necessary, false otherwise
 */
PRIVATE
bool
Deadline_timeout::expired()
{
  Context *_owner = owner();
  Context::Sched_mode _mode = _owner->mode();

  // Owner waiting for next period
  if (EXPECT_TRUE (_owner->state() & Thread_delayed_deadline))
    {
      _owner->set_mode (Context::Sched_mode (_mode | Context::Periodic));

      // Next period deferred until periodic event
      if (_owner->state() & Thread_delayed_ipc)
        {
          _owner->state_del (Thread_delayed_deadline);
          return false;
        }

      // Next period begins now
      set (_wakeup + _owner->period(), _owner->cpu());
      _owner->switch_sched (_owner->sched_context()->next());
      _owner->state_change_dirty (~Thread_delayed_deadline, Thread_ready);
      _owner->ready_enqueue();
    }
  else
    {
      // Deadline Miss
      _preemption->queue (Sched_context::Deadline_miss,
                          _wakeup,
                          _owner->sched());

      // If in periodic mode, enforce next period
      if (EXPECT_TRUE (_mode & Context::Periodic))
        {
          set (_wakeup + _owner->period(), _owner->cpu());
          _owner->switch_sched (_owner->sched_context()->next());
        }
    }

  // Flag reschedule if owner's priority is higher than the current
  // thread's (own or timeslice-donated) priority.
  return Context::can_preempt_current (_owner->sched());
}
