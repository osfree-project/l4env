INTERFACE [act_ipc]:

#include "sender.h"

class Receiver;

class Activation: public Sender
{
private:
  Receiver *	_dispatcher;
  unsigned	_pending;	// 0=none, 1=block, 2=unblock
};

//----------------------------------------------------------------------------
IMPLEMENTATION [act_ipc]:

#include <cassert>
#include "entry_frame.h"
#include "globals.h"
#include "logdefs.h"
#include "lock_guard.h"
#include "receiver.h"
#include "thread_lock.h"
#include "thread_state.h"
#include "warn.h"

/** Activation sender role constructor */
PUBLIC
Activation::Activation	(Global_id const id)
          : Sender	(id),
            _dispatcher	(0),
            _pending	(0)
{}

/** Get receiver which receives activation IPC */
PUBLIC inline Receiver * Activation::dispatcher() const
{ return _dispatcher; }

/** Set receiver which receives activation IPC */
PUBLIC inline void Activation::dispatcher (Receiver * const a)
{ _dispatcher = a; }

/**
 * Callback function for when the receiver of an activation IPC
 * becomes ready to receive. Send the IPC and reset the pending flag.
 */
PRIVATE
void
Activation::ipc_receiver_ready()
{
  assert (receiver());

  unsigned type;

  {
    Lock_guard <Thread_lock> guard ((context_of (this))->thread_lock());

    if (EXPECT_FALSE (!_pending))
      return;

    type = _pending;
    _pending = 0;
    sender_dequeue (receiver()->sender_list());
  }

  if (receiver()->ipc_try_lock (this) == 0)
    {
      receiver()->ipc_init (this);

      // Send message
      setup_msg (type);
      receiver()->deny_lipc();
      receiver()->state_change (~(Thread_receiving
				  | Thread_busy
				  | Thread_ipc_in_progress),
				Thread_ready);

      // Unlock causes receiver to be activated or ready-enqueued
      receiver()->ipc_unlock();
    }
}

/**
 * Submit an activation IPC to be sent.
 * @param type Type of activation event
 */
PUBLIC
void
Activation::send (unsigned type)
{
  // We're invoked from context::ready_{en,de}queue() with interrupts disabled.

  assert (type == 1 || type == 2);

  if (!_dispatcher)	// No dispatcher to send to?
    return;

  if (_pending)		// Already activation event pending?
    {
      assert (type != _pending); // Block and unblock must occur in turn
      _pending = 0;	// Two adjacent events cancel one another
      return;
    }

  _pending = type;

  // Dispatcher is receiver of this activation event
  set_receiver (_dispatcher);

  // Enqueue in dispatcher's sender list
  sender_enqueue (_dispatcher->sender_list());

  // If the dispatcher is already waiting, make it ready; this will cause
  // it to run Activation::ipc_receiver_ready(), which handles the rest
  if (_dispatcher->sender_ok (this))
    {
      _dispatcher->state_change_dirty (~Thread_busy, Thread_ready);
      _dispatcher->ready_enqueue();
    }
}

PRIVATE inline
void
Activation::setup_msg (unsigned const type) const
{
  receiver()->rcv_regs()->msg_dope (0);		// state = OK
  receiver()->rcv_regs()->set_msg_word (0, type);
}

//----------------------------------------------------------------------------
INTERFACE [!act_ipc]:

#include "l4_types.h"

class Receiver;
class Activation {};

IMPLEMENTATION [!act_ipc]:
PUBLIC Activation::Activation (Global_id) {}
PUBLIC void Activation::dispatcher (Receiver *) {}
PRIVATE void Activation::ipc_receiver_ready() {}
