INTERFACE:

#include "l4_types.h"

class Receiver;

/** A sender.  This is a role class, so real senders need to inherit from it.
 */
class Sender
{
public:
  /** Receiver-ready callback.  Receivers make sure to call this
      function on waiting senders when they get ready to receive a
      message from that sender.  Senders need to overwrite this interface. */
  virtual void ipc_receiver_ready() = 0;

private:
  // DATA

  L4_uid _id;
  Receiver*   _send_partner;
  Sender*     sender_next, * sender_prev;

  friend class Jdb;
  friend class Jdb_thread_list;
};

IMPLEMENTATION:

#include <cassert>

#include "cpu_lock.h"
#include "lock_guard.h"

//
// state requests/manipulation
//

/** Constructor.
    @param id user-visible thread ID of the sender
 */
PROTECTED inline
Sender::Sender(const L4_uid& id)
  : _id (id), sender_next (0)
{}

/** Optimized constructor.  This constructor assumes that the object storage
    is zero-initialized.
    @param id user-visible thread ID of the sender
    @param ignored an integer argument.  The value doesn't matter and 
                   is used just to distinguish this constructor from the 
		   default one.
 */
PROTECTED inline
explicit
Sender::Sender(const L4_uid& id, int /*ignored*/)
  : _id (id)
{}

/** Sender ID.
    @return user-visible thread ID of the sender
 */
PUBLIC inline 
L4_uid Sender::id() const
{ return _id; }

/** Current receiver.
    @return receiver this sender is currently trying to send a message to.
 */
PUBLIC inline 
Receiver* Sender::receiver() const
{ return _send_partner; }

/** Set current receiver.
    @param receiver the receiver we're going to send a message to
 */
PROTECTED inline
void Sender::set_receiver(Receiver* receiver)
{
  _send_partner = receiver;
}

// 
// queueing functions
// 

/** Sender in a queue of senders?.  
    @return true if sender has enqueued in a receiver's list of waiting 
            senders
 */
PUBLIC inline 
bool 
Sender::in_sender_list()
{
  return sender_next;
}

/** Enqueue in a sender list.
    @param r pointer to sender-list head.
 */
PROTECTED inline NEEDS ["cpu_lock.h", "lock_guard.h"]
void
Sender::sender_enqueue(Sender **r)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (! in_sender_list())
    {
      if (*r)
	{
	  sender_next = *r;
	  sender_prev = sender_next->sender_prev;
	  sender_prev->sender_next = this;
	  sender_next->sender_prev = this;
	}
      else
	{
	  *r = sender_prev = sender_next = this;
	}
    }
}

/** Dequeue from a sender list.
    @param r pointer to sender-list head.
 */
PROTECTED inline NEEDS ["cpu_lock.h", "lock_guard.h", <cassert>]
void
Sender::sender_dequeue(Sender **r)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);
  
  if (in_sender_list())
    {
      if (sender_next == this)	// are we alone in the list?
	{
	  assert(*r == this);

	  *r = 0;
	}
      else
	{
	  if (*r == this) // are we first in the list?
	    *r = sender_next;
	  
	  sender_prev->sender_next = sender_next;
	  sender_next->sender_prev = sender_prev;
	}

      sender_next = 0;
    }
}
