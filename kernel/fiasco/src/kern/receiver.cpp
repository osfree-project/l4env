INTERFACE:

#include "context.h"
#include "timer.h"

class Sys_ipc_frame;
class Sender;
class Thread;

/** A receiver.  This is a role class, and real receiver's must inherit from 
    it.  The protected interface is intended for the receiver, and the public
    interface is intended for the sender.

    The only reason this class inherits from Context is to force a specific 
    layout for Thread.  Otherwise, Receiver could just embed or reference
    a Context.
 */
class Receiver : public Context
{
  // DATA
  Sender*           _partner;	// IPC partner I'm waiting for/involved with
  Sys_ipc_frame    *_receive_regs; // registers used for receive
  vm_offset_t       _pagein_request; // sender requests page in of this page
  Thread*           _pagein_applicant;
  Sender*           _sender_first;

protected:
  // XXX Timeout for both, sender and receiver! In normal case we would have
  // to define own timeouts in Receiver and Sender but because only one
  // timeout can be set at one timeout we use the same timeout. The timeout
  // has to be defined here because dirq_t::hit has to be able to reset the
  // timeout (irq_t::_irq_thread is of type Receiver).
  timeout_t*          _timeout;
};

IMPLEMENTATION:

#include "l4_types.h"
#include <cassert>

#include "globals.h"
#include "lock_guard.h"
#include "sender.h"
#include "thread_lock.h"
#include "entry_frame.h"
#include "std_macros.h"
#include "thread_state.h"

// Interface for receivers

// CREATORS

/** Constructor.
    @param thread_lock the lock used for synchronizing access to this receiver
    @param space_context the space context 
 */
PROTECTED
inline
Receiver::Receiver(Thread_lock *thread_lock, 
		   Space_context* space_context)
  : Context (thread_lock, space_context)
{}

/** IPC partner (sender).
    @return sender of ongoing or previous IPC operation
 */
PROTECTED
inline
Sender*
Receiver::partner() const
{
  return _partner;
}

/** Restore a saved IPC state to restart a suspended IPC.
    @param partner sender of suspended receive operation
    @param regs registers of suspended receive operation
 */
PROTECTED
inline NEEDS[Receiver::set_partner, Receiver::set_receive_regs]
void
Receiver::restore_receiver_state (Sender* partner, 
				  Sys_ipc_frame* regs)
{
  set_partner (partner);
  set_receive_regs (regs);
}

/** Save IPC state to allow later restart a suspended IPC.
    @param out_partner returns sender of suspended receive operation
    @param out_regs returns pointer to IPC regs of suspended receive operation
 */
PROTECTED
inline
void
Receiver::save_receiver_state (Sender** out_partner,
			       Sys_ipc_frame** out_regs)
{
  *out_partner = _partner;
  *out_regs = _receive_regs;
}

/** Reset the current page-in request.
 */
PROTECTED inline
void
Receiver::clear_pagein_request()
{
  _pagein_request = 0xffffffff;
}

/** Return current page-in request.  Returns 0 if there is no page-in request.
 */
PROTECTED inline
vm_offset_t
Receiver::pagein_request() const
{
  return _pagein_request;
}

/** Return current requestor of a page-in. 
    @return owner of current page-in request
*/
PROTECTED inline
Thread*
Receiver::pagein_applicant() 
{
  return _pagein_applicant;
}

// Interface for senders

/** Return a reference to receiver's IPC registers.
    Senders call this function to poke values into the receiver's register
    set.
    @pre state() & Thread_ipc_receiving_mask
    @return pointer to receiver's IPC registers.
 */
PUBLIC 
inline NEEDS[<cassert>]
Sys_ipc_frame*
Receiver::receive_regs() const
{
  assert (state () & Thread_ipc_receiving_mask);

  return _receive_regs;
}

/** Send page-in request.
    Senders call this function to request that the receiver pages in memory 
    at a specific address.  Afterwards, senders should go to sleep.  The 
    receiver will wake them up later.
    @param address virtual address that should be paged in
    @param notify the sender thread that wants to be notified after the 
                  page has been paged in
    @post state() & Thread_running
 */
PUBLIC inline
void
Receiver::set_pagein_request(vm_offset_t address, Thread *notify)
{
  assert (address != 0xffffffff);

  _pagein_request = address;
  _pagein_applicant = notify;

  state_change (~Thread_busy_long, Thread_running);
}

/** Set up a receiving IPC.  
    @param sender 0 means any sender OK, otherwise only specified sender
                  may send
    @param regs   register set that should be used as IPC registers
    @return state bits that should be added to receiver's state word.
    @post (sender == 0 || partner() == sender) && (receive_regs() == regs)
          && (retval == (sender ? Thread_receiving : Thread_waiting)
 */
PROTECTED 
inline NEEDS["thread_state.h", Receiver::set_partner, 
	     Receiver::set_receive_regs]
unsigned
Receiver::setup_receiver_state (Sender* sender, Sys_ipc_frame* regs)
{
  set_receive_regs (regs);	// message should be poked in here

  if (sender)
    {
      set_partner (sender);
      return Thread_receiving;
    }
  else
    {
      return Thread_waiting;
    }
}

/** Convenience function: Set up a receiving IPC and add the
    corresponding state.
    @param sender 0 means any sender OK, otherwise only specified sender
                  may send
    @param regs   register set that should be used as IPC registers
    @return state bits that should be added to receiver's state word.
    @post (sender == 0 || partner() == sender) && (receive_regs() == regs)
          && (state & (sender ? Thread_receiving : Thread_waiting)
 */
PROTECTED inline NEEDS[Receiver::setup_receiver_state]
void
Receiver::setup_receiver (Sender* sender, Sys_ipc_frame* regs)
{
  state_add (setup_receiver_state(sender, regs));
}

/** Return whether the receiver is ready to accept a message from the
    given sender.
    @param sender thread that wants to send a message to this receiver
    @return true if receiver is in correct state to accept a message 
                 right now (open wait, or closed wait and waiting for sender).
 */
PUBLIC 
inline NEEDS["std_macros.h", "thread_state.h", Receiver::partner]
bool
Receiver::sender_ok (Sender* sender) const
{
  unsigned ipc_state = state() & (Thread_receiving|Thread_waiting
				  | Thread_send_in_progress
				  | Thread_ipc_in_progress);
  
  // Thread_send_in_progress must not be set
  if (   (EXPECT_TRUE  (ipc_state == (Thread_waiting  |Thread_ipc_in_progress)))
      || (EXPECT_TRUE ((ipc_state == (Thread_receiving|Thread_ipc_in_progress))
		       && partner() == sender))
      )
    {
      return true;
    }
  
  return false;
}

/** Return whether the receiver is still engaged in a long IPC with a
    sender.
    @param sender a sender
    @return true if receiver is in long IPC with sender
 */
PUBLIC 
inline NEEDS["thread_state.h", Receiver::partner]
bool
Receiver::in_long_ipc (Sender* sender) const
{
  if (((state() & (Thread_rcvlong_in_progress|Thread_ipc_in_progress)) 
       != (Thread_rcvlong_in_progress|Thread_ipc_in_progress))
      || partner() != sender)
    return false;

  return true;
}


/** Try to start an IPC handshake with this receiver.
    Check the receiver's state, checks if the receiver is acceptable at
    this time, and if OK, /lock/ the receiver and copy the sender's ID
    to the receiver.  
    @param sender the sender that wants to establish an IPC handshake
    @return 0 for success, -1 in case of a transient failure, an IPC
            error code if an error occurs. 
 */
PUBLIC inline NEEDS [Receiver::sender_ok, Receiver::set_partner, 
		     Receiver::receive_regs, 
		     Receiver::clear_pagein_request,
		     "std_macros.h", "thread_lock.h", "entry_frame.h",
                     "sender.h", "l4_types.h"]
int
Receiver::ipc_try_lock (Sender* sender)
{
  if (EXPECT_FALSE( state () == Thread_invalid) )
    return L4_msgdope::ENOT_EXISTENT;

  thread_lock()->lock();

  if (EXPECT_FALSE(! sender_ok (sender)) )
    {
      thread_lock()->clear();
      return -1;
    }

  set_partner (sender);
  receive_regs()->rcv_source( sender->id() );
  clear_pagein_request ();

  return 0;			// OK
}

/** Unlock a receiver locked with ipc_try_lock(). */
PUBLIC inline NEEDS ["thread_lock.h", "globals.h"]
void
Receiver::ipc_unlock ()
{
  assert (thread_lock()->lock_owner() == current());

  thread_lock()->clear();
}

/** Head of sender list.
    @return a reference to the receiver's list of senders
 */
PUBLIC inline
Sender**
Receiver::sender_list ()
{
  return &_sender_first;
}

// MANIPULATORS

inline 
void
Receiver::set_receive_regs(Sys_ipc_frame* regs)
{
  _receive_regs = regs;
}

inline 
void
Receiver::set_partner(Sender* partner)
{
  _partner = partner;
}

PROTECTED inline
void
Receiver::set_receive_timeout(timeout_t *t)
{
  _timeout = t;
}

PUBLIC inline
void
Receiver::reset_receive_timeout()
{
  if (_timeout)
    {
      _timeout->reset();
      _timeout = 0;
    }
}

