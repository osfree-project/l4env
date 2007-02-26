
INTERFACE:

#include "timeout.h"

class Receiver;

class IPC_timeout : public Timeout
{
public:
  /**
   * @brief IPC_timeout constructor
   * @param Receiver which is notified when timeout expires
   */
  IPC_timeout (Receiver *owner);

  /**
   * @brief IPC_timeout destructor
   */
  virtual ~IPC_timeout();

private:
  /**
   * @brief Timeout expiration callback function
   * @return true if reschedule is necessary, false otherwise
   */
  bool expired();

  /**
   * @brief Owner of this IPC_timeout
   */
  Receiver * const _owner;
};

IMPLEMENTATION:

#include "context.h"
#include "receiver.h"
#include "thread_state.h"

IMPLEMENT inline
IPC_timeout::IPC_timeout (Receiver * const owner)
           : _owner (owner)
{}

IMPLEMENT inline NEEDS ["receiver.h"]
IPC_timeout::~IPC_timeout()
{
  _owner->set_timeout (0);	// reset owner's timeout field
}

IMPLEMENT
bool
IPC_timeout::expired()
{
  // Set thread running
  _owner->state_change (~Thread_ipc_in_progress, Thread_running);
  _owner->ready_enqueue();
     
  // Flag reschedule if the global timeslice has been invalidated or the newly
  // woken-up thread's prio is higher than that of global timeslice.
  return !Context::current_sched() ||
         _owner->sched()->prio() > Context::current_sched()->prio();
}
