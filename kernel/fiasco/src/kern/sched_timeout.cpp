
INTERFACE:

#include "timeout.h"

class Context;

class Sched_timeout : public Timeout
{
public:

  /**
   * @brief IPC timeout constructor
   * @param Context which is notified when timeout expires
   */
  Sched_timeout (Context *owner);

private:

  /**
   * @brief Timeout expiration callback function
   * @return true if reschedule is necessary, false otherwise
   */
  bool expired();

  /**
   * @brief Owner of this timeout
   */
  Context * const _owner;
};

IMPLEMENTATION:

#include "context.h"

IMPLEMENT inline
Sched_timeout::Sched_timeout (Context * const owner)
             : _owner (owner)
{}

IMPLEMENT
bool
Sched_timeout::expired()
{
  _owner->set_sched (_owner->timesharing_sched()->next());

  // Flag reschedule if the global timeslice has been invalidated or the newly
  // activated timeslice's prio is higher than that of global timeslice.
  return !Context::current_sched() ||
         _owner->sched()->prio() > Context::current_sched()->prio();
}
