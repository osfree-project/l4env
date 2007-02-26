INTERFACE:

#include "context.h"
#include "timeout.h"

class Sys_ipc_frame;
class Sender;
class Thread;
class Space;

/** A receiver.  This is a role class, and real receiver's must inherit from 
    it.  The protected interface is intended for the receiver, and the public
    interface is intended for the sender.

    The only reason this class inherits from Context is to force a specific 
    layout for Thread.  Otherwise, Receiver could just embed or reference
    a Context.
 */
class Receiver : public Context
{
public:
  bool sender_ok (const Sender* sender) const;

private:
  // DATA
  Sender*           _partner;	// IPC partner I'm waiting for/involved with
  Sys_ipc_frame    *_rcv_regs; // registers used for receive
  Address           _pagein_addr; // sender requests page in of this page
  Mword             _pagein_error_code;
  Thread*           _pagein_applicant;
  Sender*           _sender_first;

protected:
  // XXX Timeout for both, sender and receiver! In normal case we would have
  // to define own timeouts in Receiver and Sender but because only one
  // timeout can be set at one time we use the same timeout. The timeout
  // has to be defined here because Dirq::hit has to be able to reset the
  // timeout (Irq::_irq_thread is of type Receiver).
  Timeout *          _timeout;

};

IMPLEMENTATION:

#include "l4_types.h"
#include <cassert>

#include "cpu_lock.h"
#include "globals.h"
#include "lock_guard.h"
#include "sender.h"
#include "thread_lock.h"
#include "entry_frame.h"
#include "std_macros.h"
#include "thread_state.h"

// Interface for receivers

/** Constructor.
    @param thread_lock the lock used for synchronizing access to this receiver
    @param space_context the space context 
 */
PROTECTED inline
Receiver::Receiver(Thread_lock *thread_lock, 
		   Space* space,
		   unsigned short prio,
		   unsigned short mcp,
		   Unsigned64 quantum)
  : Context (thread_lock, space, prio, mcp, quantum)
{}

/** IPC partner (sender).
    @return sender of ongoing or previous IPC operation
 */
PROTECTED inline
Sender*
Receiver::partner() const
{
  return _partner;
}

/** Restore a saved IPC state to restart a suspended IPC.
    @param partner sender of suspended receive operation
    @param regs registers of suspended receive operation
 */
PROTECTED inline NEEDS[Receiver::set_partner, Receiver::set_rcv_regs]
void
Receiver::restore_receiver_state (Sender* partner, 
				  Sys_ipc_frame* regs)
{
  set_partner (partner);
  set_rcv_regs (regs);
}

/** Save IPC state to allow later restart a suspended IPC.
    @param out_partner returns sender of suspended receive operation
    @param out_regs returns pointer to IPC regs of suspended receive operation
 */
PROTECTED inline
void
Receiver::save_receiver_state (Sender** out_partner,
			       Sys_ipc_frame** out_regs)
{
  *out_partner = _partner;
  *out_regs = _rcv_regs;
}

/** Reset the current page-in request.
 */
PROTECTED inline
void
Receiver::clear_pagein_request()
{
  _pagein_addr       = (Address) -1;
  _pagein_error_code = 0;
}

/** Return current page-in address.  Returns -1 if there is no page-in request.
 */
PROTECTED inline
Address
Receiver::pagein_addr() const
{
  return _pagein_addr;
}

/** Return current page-in error code.  Returns 0 if there is no page-in
 *  request.
 */
PROTECTED inline
Mword
Receiver::pagein_error_code() const
{
  return _pagein_error_code;
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
    Senders call this function to poke values into the receiver's register set.
    @pre state() & Thread_ipc_receiving_mask
    @return pointer to receiver's IPC registers.
 */
PUBLIC inline NEEDS[<cassert>]
Sys_ipc_frame*
Receiver::rcv_regs() const
{
  assert (state () & Thread_ipc_receiving_mask);

  return _rcv_regs;
}

/** Send page-in request.
    Senders call this function to request that the receiver pages in memory 
    at a specific address.  Afterwards, senders should go to sleep.  The 
    receiver will wake them up later.
    @param address virtual address that should be paged in
    @param notify the sender thread that wants to be notified after the 
                  page has been paged in
    @post state() & Thread_ready
 */
PUBLIC inline
void
Receiver::set_pagein_request(Address address, Mword error_code, Thread *notify)
{
  assert (address != (Address) -1);

  _pagein_applicant  = notify;
  _pagein_addr       = address;
  _pagein_error_code = error_code;

  state_change (~Thread_busy_long, Thread_ready);
}

/** Return whether the receiver is still engaged in a long IPC with a
    sender.
    @param sender a sender
    @return true if receiver is in long IPC with sender
 */
PUBLIC inline NEEDS["thread_state.h", Receiver::partner]
bool
Receiver::in_long_ipc (Sender* sender) const
{
  if (((state() & (Thread_rcvlong_in_progress|Thread_ipc_in_progress)) 
       != (Thread_rcvlong_in_progress|Thread_ipc_in_progress))
      || partner() != sender)
    return false;

  return true;
}


/** Head of sender list.
    @return a reference to the receiver's list of senders
 */
PUBLIC inline
Sender**
Receiver::sender_list()
{
  return &_sender_first;
}

// MANIPULATORS

PROTECTED inline 
void
Receiver::set_rcv_regs(Sys_ipc_frame* regs)
{
  _rcv_regs = regs;
}

PUBLIC inline
void
Receiver::set_timeout (Timeout *t)
{
  _timeout = t;
}

PUBLIC inline
void
Receiver::reset_timeout()
{
  if (EXPECT_TRUE(!_timeout))
    return;

  _timeout->reset();
  _timeout = 0;
}


/** Try to start an IPC handshake with this receiver.
    Check the receiver's state, checks if the receiver is acceptable at
    this time, and if OK, /lock/ the receiver and copy the sender's ID
    to the receiver.  
    @param sender the sender that wants to establish an IPC handshake
    @return 0 for success, -1 in case of a transient failure, an IPC
            error code if an error occurs. 
 */
PUBLIC inline NEEDS [Receiver::sender_ok,
		     Receiver::rcv_regs, 
		     Receiver::clear_pagein_request,
		     "std_macros.h", "thread_lock.h", "entry_frame.h",
                     "sender.h", "l4_types.h"]
int
Receiver::ipc_try_lock (const Sender* sender)
{
  if (EXPECT_FALSE (state() == Thread_invalid))
    return Ipc_err::Enot_existent;

  thread_lock()->lock();

  if (EXPECT_FALSE (!sender_ok (sender)))
    {
      /*
	Mark for the receiver that someone is waiting for him.
	If the receiver wants to do the next LIPC, 
	it will check, if someone is waiting for him.
	If yes, then abort LIPC and use normal kernel IPC.
      */
      thread_lock()->clear();
      return -1;
    }

  return 0;			// OK
}

/** Initiates a receiving IPC and updates the ipc partner.
    @param sender the sender that wants to establish an IPC handshake
 */
PUBLIC inline NEEDS [Receiver::set_partner, 
		     Receiver::rcv_regs, 
		     Receiver::clear_pagein_request,
		     "entry_frame.h", "sender.h", "l4_types.h"]
void
Receiver::ipc_init (Sender* sender)
{
  set_partner (sender);
  rcv_regs()->rcv_src (sender->id());
  state_add_dirty(Thread_transfer_in_progress);
}

PROTECTED inline
void Receiver::prepare_receive_dirty(Sender *partner,
                             Sys_ipc_frame *regs)
{
  // cpu lock required, or weird things will happen
  assert(cpu_lock.test());

  set_rcv_regs (regs);  // message should be poked in here
  set_partner (partner);

  state_change_dirty(~(Thread_ipc_sending_mask|Thread_transfer_in_progress),
                     Thread_receiving | Thread_ipc_in_progress);
}

PUBLIC inline
bool
Receiver::in_ipc(Sender *sender)
{
  Mword ipc_state = (state() & (Thread_ipc_in_progress
                               | Thread_cancel
                               | Thread_transfer_in_progress));

  if(EXPECT_TRUE
     ((ipc_state == (Thread_transfer_in_progress | Thread_ipc_in_progress))
      &&  _partner == sender))
    return true;

  return false;

}

/** Set the IPC partner (sender).
    @param partner IPC partner
 */
PROTECTED inline 
void
Receiver::set_partner(Sender* partner)
{
  _partner = partner;
}

/** Unlock a receiver locked with ipc_try_lock(). */
PUBLIC inline NEEDS ["thread_lock.h", "globals.h"]
void
Receiver::ipc_unlock()
{
  assert (thread_lock()->lock_owner() == current());

  thread_lock()->clear();
}


/** Return whether the receiver is ready to accept a message from the
    given sender.
    @param sender thread that wants to send a message to this receiver
    @return true if receiver is in correct state to accept a message 
                 right now (open wait, or closed wait and waiting for sender).
 */
IMPLEMENT inline NEEDS["std_macros.h", "thread_state.h", Receiver::partner]
bool
Receiver::sender_ok (const Sender *sender) const
{
  unsigned ipc_state = state() & (Thread_receiving |
                                  //                 Thread_send_in_progress |
                                  Thread_ipc_in_progress);

  // If Thread_send_in_progress is still set, we're still in the send phase
  if (EXPECT_FALSE (ipc_state != (Thread_receiving | Thread_ipc_in_progress)))
    return false;

  // Check open wait; test if this sender is really the first in queue
  if (EXPECT_TRUE(!partner()
                  && (!_sender_first || sender == _sender_first)))
    return true;

  // Check closed wait; test if this sender is really who we specified
  if (EXPECT_TRUE (sender == partner()))
    return true;

  return false;
}

/** Set up a receiving IPC.  
    @param sender 0 means any sender OK, otherwise only specified sender
                  may send
    @param regs   register set that should be used as IPC registers
    @return state bits that should be added to receiver's state word.
    @post (sender == 0 || partner() == sender) && (receive_regs() == regs)
          && (retval == Thread_receiving)
 */
PROTECTED inline NEEDS["thread_state.h", Receiver::set_partner, 
		       Receiver::set_rcv_regs]
unsigned
Receiver::setup_receiver_state (Sender* sender,
				Sys_ipc_frame* regs,
				bool = false)
{
  set_rcv_regs (regs);	// message should be poked in here
  set_partner (sender);
  return Thread_receiving;
}

