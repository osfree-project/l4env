IMPLEMENTATION:

#include "entry_frame.h"
#include "globalconfig.h"
#include "l4_types.h"
#include "receiver.h"
#include "thread_state.h"
#include "thread_lock.h"
#include "lock_guard.h"

PUBLIC
virtual void
Irq::ipc_receiver_aborted()
{
  assert(receiver());

  sender_dequeue(receiver()->sender_list());
  set_receiver(0);
}

/** Sender-activation function called when receiver gets ready.
    Irq::hit() actually ensures that this method is always called
    when an interrupt occurs, even when the receiver was already
    waiting.
 */
PUBLIC
virtual bool
Irq::ipc_receiver_ready(Receiver *)
{
  // we are running with ints off
  assert(current() == _irq_thread);
  assert(_queued);
  assert(current()->state() & Thread_ready);

  Lock_guard <Thread_lock> guard;

  // possible preemption point
  if (guard.lock(_irq_thread->thread_lock()) == Thread_lock::Invalid)
    return false;

  if(!_irq_thread->sender_ok(this))
    return true;

  _irq_thread->ipc_init(this);

  assert(_irq_thread->state() & Thread_ready);

  _irq_thread->state_change(~(Thread_receiving | Thread_busy
                              | Thread_transfer_in_progress
                              | Thread_ipc_in_progress),
                            Thread_ready);

  // here we can optimize coz we are running with ints off
      // XXX receiver should also get a fresh timeslice
  if (consume() < 1)    // last interrupt in queue?
    sender_dequeue(_irq_thread->sender_list());

  // else remain queued if more interrupts are left
  return true;
}
