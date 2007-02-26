/*
 * Fiasco
 * Preemption IPC Handling
 */

INTERFACE:

#include "sender.h"

class Receiver;
class Sched_context;

class Preemption : public Sender
{
private:
  Preemption (Preemption&);
  
  /**
   * @brief Return the Sched_context for which this Preemption sender is
   *        currently waiting to send a preemption IPC.
   */
  Sched_context *pending_preemption();

  /**
   * @brief Set the Sched_context for which this Preemption sender is
   *        currently waiting to send a preemption IPC.
   */
  void set_pending_preemption (Sched_context *sched);

  /**
   * @brief Callback function for when the receiver of a preemption IPC
   *        becomes ready to receive. Send the IPC and submit the next one.
   */
  void ipc_receiver_ready();

  // DATA
  Sched_context *_pending_preemption;

public:
  /**
   * @brief Submit a Sched_context for sending a preemption IPC. If there is
   *        already a pending Sched_context, this Sched_context is resubmitted
   *        by find_next_preemption() later.
   */
  void send (Receiver *preempter, Sched_context *sched);
};

IMPLEMENTATION:

#include <cassert>
#include "entry_frame.h"
#include "receiver.h"
#include "sched_context.h"
#include "thread_state.h"

PUBLIC
Preemption::Preemption (L4_uid id)
          : Sender (id),
            _pending_preemption (0)
{}

IMPLEMENT inline
Sched_context *
Preemption::pending_preemption()
{
  return _pending_preemption;
}

IMPLEMENT inline
void
Preemption::set_pending_preemption (Sched_context *sched)
{
  _pending_preemption = sched;
}

IMPLEMENT
void
Preemption::ipc_receiver_ready()
{
  assert (receiver());
  assert (pending_preemption());

  if (receiver()->ipc_try_lock (this) == 0)
    {
      Cpu_time t = pending_preemption()->preemption_time();

      // Send preemption IPC
      receiver()->receive_regs()->msg_dope(0);		// state = OK

      receiver()->receive_regs()->set_msg_word (0, t >> 32 & (1 << 24) - 1 |
                                  pending_preemption()->id() << 24);
      receiver()->receive_regs()->set_msg_word (1, t);

      receiver()->state_change(~(Thread_waiting | Thread_receiving |
                                 Thread_busy | Thread_ipc_in_progress),
                                 Thread_running);

      // Find the next Sched_context with a pending preemption event, if any
      set_pending_preemption (pending_preemption()->find_next_preemption());
      
      // If there aren't any left, stop sending
      if (!pending_preemption())
        sender_dequeue (receiver()->sender_list());

      receiver()->ipc_unlock();
    }
}

IMPLEMENT
void
Preemption::send (Receiver *preempter, Sched_context *sched)
{
  // We're invoked from Context::schedule() with interrupts disabled.

  // No preempter to send to or already handling another preemption event
  if (!preempter || pending_preemption())
    return;

  // Now handling this Sched_context's preemption event
  set_pending_preemption (sched);

  // Preempter is now receiver of this preemption event
  set_receiver (preempter);

  // Enqueue in preempter's sender list
  sender_enqueue (preempter->sender_list());

  // If the preempter is already waiting, make it running; this will cause
  // it to run Preemption::ipc_receiver_ready(), which handles the rest
  if (preempter->sender_ok (this))
    {
      preempter->state_change_dirty (~Thread_busy, Thread_running);
      preempter->ready_enqueue();
    }
}
