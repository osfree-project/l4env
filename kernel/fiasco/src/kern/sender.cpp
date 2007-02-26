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

protected:
  Receiver *  _receiver;

private:
  Global_id   _id;
  Sender *sender_next, *sender_prev;

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

/** Optimized constructor.  This constructor assumes that the object storage
    is zero-initialized.
    @param id user-visible thread ID of the sender
    @param ignored an integer argument.  The value doesn't matter and 
                   is used just to distinguish this constructor from the 
		   default one.
 */
PROTECTED inline
explicit
Sender::Sender (const Global_id& id, int /*ignored*/)
      : _receiver (0),
        _id (id)
{}

/** Sender ID.
    @return user-visible thread ID of the sender
 */
PUBLIC inline 
Global_id
Sender::id() const
{
  return _id;
}

/** Current receiver.
    @return receiver this sender is currently trying to send a message to.
 */
PUBLIC inline 
Receiver *
Sender::receiver() const
{
  return _receiver;
}

/** Set current receiver.
    @param receiver the receiver we're going to send a message to
 */
PROTECTED inline
void
Sender::set_receiver (Receiver* receiver)
{
  _receiver = receiver;
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
PUBLIC inline NEEDS ["cpu_lock.h", "lock_guard.h", <cassert>]
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

//---------------------------------------------------------------------------
IMPLEMENTATION[!lipc]:

/** Constructor.
    @param id user-visible thread ID of the sender
 */
PROTECTED inline
Sender::Sender(const Global_id& id)
  : _id (id), sender_next (0)
{}



INTERFACE[v2-lipc]:

#include "utcb.h"

class Space;

// MAYBE V4 needs this too, because in V4 you can wait
// for any local Thread

EXTENSION class Sender
{

private:

  // Optimization, with some Stack-magic
  // we can return the values from the Context or Zero

  Space* _snd_space;
  Local_id	 _snd_local_id;
};


//---------------------------------------------------------------------------
IMPLEMENTATION[v2-lipc]:

/** Constructor.
    @param id user-visible thread ID of the sender
 */
PROTECTED inline
Sender::Sender(const Global_id& id)
    : _id (id),
      sender_next (0),
      _snd_space(0),
      _snd_local_id(0)
{}


PROTECTED inline
void
Sender::set_snd_local_id(Local_id lid)
{
  _snd_local_id = lid;
}

PROTECTED inline
void
Sender::set_snd_space(Space *space)
{
  _snd_space = space;
}

PUBLIC inline
Local_id
Sender::snd_local_id() const
{
  return _snd_local_id;
}

PUBLIC inline
Space *
Sender::snd_space() const
{
  return _snd_space;
}
