/*
 * Fiasco
 * Preemption IPC Handling
 */

INTERFACE:

#include "sched_context.h"
#include "sender.h"

class Receiver;
class Sched_context;

class Preemption : public Sender
{
  private:
    Sched_context * _pending;
};

IMPLEMENTATION:

#include <cassert>
#include "entry_frame.h"
#include "globals.h"
#include "kip.h"
#include "logdefs.h"
#include "lock_guard.h"
#include "receiver.h"
#include "thread_lock.h"
#include "thread_state.h"

/**
 * Preemption sender role constructor
 */
PUBLIC
Preemption::Preemption (Global_id const &id)
          : Sender     (id/*.preemption_id() XXX: enable preeption again*/),
            _pending   (0)
{}

/**
 * Set receiver (preempter) which receives preemption IPC
 */
PUBLIC inline NEEDS ["receiver.h"]
void
Preemption::set_receiver (Receiver *receiver)
{
  // Dequeue from old preempter's sender list
  if (_receiver)
    sender_dequeue (_receiver->sender_list());

  // Set new preempter
  _receiver = receiver;

  // If there is already a pending event, enqueue in preempter's sender list
  if (pending())
    preempter_enqueue();
}

/*
 * Enqueue in the preempter's sender list in order to send a Preemption IPC
 */
PRIVATE
void
Preemption::preempter_enqueue()
{
  // If there is no preempter, we're done
  if (EXPECT_FALSE (!_receiver))
    return;

  // Enqueue in preempter's sender list
  sender_enqueue (_receiver->sender_list(), _receiver->sched()->prio());

  // If the preempter is already waiting, make it ready; this will cause
  // it to run Preemption::ipc_receiver_ready(), which handles the rest
  if (_receiver->sender_ok (this))
    {
      _receiver->state_change_dirty (~Thread_busy, Thread_ready);
      _receiver->ready_enqueue();
    }
}

/**
 * Return scheduling context for which this role is going to send PIPC
 */
PUBLIC inline
Sched_context *
Preemption::pending() const
{
  return _pending;
}

/**
 * Set scheduling context for which this role is going to send PIPC
 */
PUBLIC inline
void
Preemption::set_pending (Sched_context *const sched)
{
  _pending = sched;
}

PUBLIC
virtual void
Preemption::ipc_receiver_aborted()
{
  assert(receiver());

  sender_dequeue(receiver()->sender_list());
  set_receiver(0);
}

/**
 * Callback function for when the receiver of a preemption IPC
 * becomes ready to receive. Send the IPC and submit the next one.
 */
PRIVATE
bool
Preemption::ipc_receiver_ready(Receiver *recv)
{

  assert (receiver());
  assert (receiver() == recv); 
  (void) recv;
  
  Sched_context::Preemption_type type;
  Cpu_time time;
  unsigned short id;
  bool lost;

  {
    // Lock down sender's list of scheduling contexts
    Lock_guard <Thread_lock> guard ((context_of (this))->thread_lock());

    // Dequeue even if more preemption IPCs are pending to ensure fairness
    sender_dequeue (receiver()->sender_list());

    if(!pending())
        return false;

    // Extract preemption info
    id   = pending()->id();
    time = pending()->preemption_time();
    type = pending()->preemption_type();
    lost = pending()->preemption_count() > 1;
  }

  {

    Lock_guard <Thread_lock> guard(receiver()->thread_lock());


    // canceled
    if(!receiver()->sender_ok(this))
      return false;

    receiver()->ipc_init (this);

    // Send message
    setup_msg (L4_pipc (type, lost, id, time));

    receiver()->state_change(~(Thread_receiving | Thread_busy
                               | Thread_transfer_in_progress
                               | Thread_ipc_in_progress),
                             Thread_ready);
  }

  // Lock down sender's list of scheduling contexts again
  Lock_guard <Thread_lock> guard ((context_of (this))->thread_lock());

  // Check if Sched_contexts have been deallocated before we got the lock
  if (EXPECT_FALSE (!pending()))
    return true;

  // Find the next Sched_context with a pending preemption event, if any
  set_pending (pending()->find_next_preemption());

  // Enqueue again in sender list if there are more preemption IPCs pending
  if (pending())
    sender_enqueue (receiver()->sender_list(), 255);

  return true;
}

/**
 * Queue a pending preemption event on a Sched_context. If there is already a
 * pending event, this new event is resubmitted by find_next_preemption() later.
 * @param type Type of preemption event
 * @param time Time at which preemption event occured
 * @param sched Sched_context this preemption event occured on
 */
PUBLIC
void
Preemption::queue (Sched_context::Preemption_type type, Cpu_time time,
                   Sched_context *sched)
{
  // We're invoked from *_timeout::expired() with interrupts disabled.

  // Set preemption time and type for this Sched_context
  sched->set_preemption_event (type, time);

  LOG_SEND_PREEMPTION;

  // If there is already a pending preemption event, we're done
  if (pending())
    return;

  // Mark this preemption event pending
  set_pending (sched);

  preempter_enqueue();
}

PRIVATE inline
void
Preemption::setup_msg (L4_pipc const pipc) const
{
  receiver()->rcv_regs()->msg_dope (0);		// state = OK
  receiver()->rcv_regs()->set_msg_word (0, pipc.low());
  receiver()->rcv_regs()->set_msg_word (1, pipc.high());
}
