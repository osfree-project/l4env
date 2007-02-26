INTERFACE:

class Sys_ipc_frame;

EXTENSION class Thread
{
public:
  inline bool ipc_short_cut();

private:
  /**
   * IPC rendezvous. 
   * This method sets up an IPC.  It also finishes the IPC if it was a
   * short IPC.
   * @param receiver IPC receiver
   * @param sender_regs sender's IPC registers
   * @return IPC error code that should be returned to the sender.
   *         If the IPC could not be set up because of a transient error
   *         (e.g., IPC partner not ready to receive), returns 0x80000000.
   */
  Mword ipc_send_regs (Thread* receiver,
		       Sys_ipc_frame const *sender_regs);

  /** 
   * Set up a page fault message.
   *
   * @param r the stack frame
   * @param fault_address the address where the PF occured
   * @param fault_ip EIP value on page fault
   * @param err error code
   */
  void page_fault_msg (Sys_ipc_frame &r, Address fault_address, 
		       Mword fault_ip, Mword err);

  /**
   * Write IPC error code into stack frame and/or UTCB in case of
   * successful IPC.
   *
   * @param regs the stack frame
   * @param err the error code
   * @see commit_ipc_failure
   */
  void commit_ipc_success (Sys_ipc_frame *regs, Ipc_err err);

  /**
   * Write IPC error code into stack frame and/or UTCB in case of
   * failed IPC.
   *
   * @param regs the stack frame
   * @param err the error code
   * @see commit_ipc_success
   */
  void commit_ipc_failure (Sys_ipc_frame *regs, Ipc_err err);

  /**
   * Get the current IPC error code out of stack frame and/or UTCB
   *
   * @param *regs the stack frame where to read out the error code
   * @return the error code found
   */
  Ipc_err get_ipc_err (Sys_ipc_frame *regs);

  /**
   * Set the source UTCB pointer in the IPC reg's, if this was
   * an intra task IPC. Necessary for efficient LIPC.
   * It's a dummy on non LIPC kernels.
   *
   * @param receiver the receiver
   * @return dest_regs the receivers IPC registers
   */
  void set_source_local_id(Thread *receiver,  Sys_ipc_frame *dest_regs);
};

/**
 * Dummy class, needed to hold code in Thread generic
 */
class Pf_msg_utcb_saver
{
public:
  Pf_msg_utcb_saver (const Utcb *u);
  void restore (Utcb *u);
};

//---------------------------------------------------------------------------
INTERFACE [v2-lipc]:

EXTENSION class Pf_msg_utcb_saver
{
private:
  Mword ipc_state;
};


IMPLEMENTATION:

// IPC setup, and handling of ``short IPC'' and page-fault IPC

// IDEAS for enhancing this implementation: 

// Volkmar has suggested a possible optimization for
// short-flexpage-to-long-message-buffer transfers: Currently, we have
// to resort to long IPC in that case because the message buffer might
// contain a receive-flexpage option.  An easy optimization would be
// to cache the receive-flexpage option in the TCB for that case.
// This would save us the long-IPC setup because we wouldn't have to
// touch the receiver's user memory in that case.  Volkmar argues that
// cases like that are quite common -- for example, imagine a pager
// which at the same time is also a server for ``normal'' requests.

// The handling of cancel and timeout conditions could be improved as
// follows: Cancel and Timeout should not reset the ipc_in_progress
// flag.  Instead, they should just set and/or reset a flag of their
// own that is checked every time an (IPC) system call wants to go to
// sleep.  That would mean that IPCs that do not block are not
// cancelled or aborted.
//-

#include <cstdlib>		// panic()

#include "l4_types.h"

#include "config.h"
#include "cpu_lock.h"
#include "dirq.h"
#include "ipc_timeout.h"
#include "lock_guard.h"
#include "logdefs.h"
#include "map_util.h"
#include "processor.h"
#include "kdb_ke.h"

/** Receiver-ready callback.  
    Receivers make sure to call this function on waiting senders when
    they get ready to receive a message from that sender. Senders need
    to overwrite this interface.

    Class Thread's implementation wakes up the sender if it is still in
    sender-wait state.
 */
PUBLIC virtual 
void Thread::ipc_receiver_ready()
{
  Lock_guard <Thread_lock> guard (thread_lock());

  if ((state_change (~Thread_polling, 0) & Thread_polling) == 0)
    return;

  state_add (Thread_ready);

  _thread_lock.set_switch_hint (SWITCH_ACTIVATE_LOCKEE);
}

/**
 * Compute thread's send timeout
 * @param t timeout descriptor
 * @param regs ipc registers
 * @return 0 if timeout expired, absolute timeout value otherwise
 */
PRIVATE inline
Unsigned64
Thread::snd_timeout (L4_timeout t, Sys_ipc_frame const * regs)
{
  // absolute timeout
  if (EXPECT_FALSE (regs->has_abs_snd_timeout()))
    {
      Unsigned64 sc = Timer::system_clock();
      Unsigned64 tval = t.snd_microsecs_abs (sc, regs->abs_snd_clock());

      // check if timeout already expired
      return tval <= sc ? 0 : tval;
    }

  // zero timeout
  if (t.snd_man() == 0)
    return 0;

  // relative timeout
  return t.snd_microsecs_rel (Timer::system_clock());
}

/**
 * Compute thread's receive timeout
 * @param t timeout descriptor
 * @param regs ipc registers
 * @return 0 if timeout expired, absolute timeout value otherwise
 */
PRIVATE inline
Unsigned64
Thread::rcv_timeout (L4_timeout t, Sys_ipc_frame const *regs)
{
  // absolute timeout
  if (EXPECT_FALSE (regs->has_abs_rcv_timeout()))
    {
      Unsigned64 sc = Timer::system_clock();
      Unsigned64 tval = t.rcv_microsecs_abs (sc, regs->abs_rcv_clock());

      // check if timeout already expired
      return tval <= sc ? 0 : tval;
    }

  // zero timeout
  if (t.rcv_man() == 0)
    return 0;

  // relative timeout
  return t.rcv_microsecs_rel (Timer::system_clock());
}

/**
 * Send an IPC message.
 *        Block until we can send the message or the timeout hits.
 * @param partner the receiver of our message
 * @param t a timeout specifier
 * @param regs sender's IPC registers
 * @return sender's IPC error code
 */
PRIVATE
Ipc_err Thread::do_send (Thread *partner, L4_timeout t, Sys_ipc_frame *regs)
{

  if (EXPECT_FALSE (partner == 0 
      || partner->state() == Thread_invalid     // partner nonexistent?
      || partner == nil_thread))                // must not send to L4_NIL_ID
    return Ipc_err (Ipc_err::Enot_existent);
      
  Mword error_ret;

  // register partner so that we can be dequeued by kill()
  set_receiver (partner);

  state_add (Thread_polling |
	     Thread_ipc_in_progress | Thread_send_in_progress);

  if (EXPECT_FALSE (state() & Thread_cancel))
    {
      state_del (Thread_ipc_sending_mask | Thread_ipc_in_progress);
      return Ipc_err (Ipc_err::Secanceled);
    }

  sender_enqueue (partner->sender_list());

  // try a rendezvous with the partner
  error_ret = ipc_send_regs (partner, regs);

  if (EXPECT_FALSE (error_ret & 0x80000000))		// transient error
    {
      // in case of a transient error, try sleeping
      IPC_timeout timeout;

      // we're not sleeping forever
      if (t.snd_exp())
	{
          Unsigned64 tval = snd_timeout (t, regs);

          // Zero timeout or timeout expired already -- give up
          if (tval == 0)
	    {
	      state_del (Thread_polling | Thread_ipc_in_progress |
                         Thread_send_in_progress);
	      sender_dequeue (partner->sender_list());
	      return Ipc_err (Ipc_err::Setimeout);	// give up
	    }

          // enqueue timeout
          set_timeout (&timeout);
          timeout.set (tval);
	}
      
      do 
	{
	  // check if thread has been reset
	  if (state() & Thread_cancel)
	    {
	      sender_dequeue (partner->sender_list());
	      error_ret = Ipc_err::Secanceled;
	      break;
	    }

	  // go to sleep
	  if (state_change_safely (~(Thread_ipc_in_progress | Thread_polling |
                                     Thread_ready),
				     Thread_ipc_in_progress | Thread_polling))
	    schedule();
	  
	  state_add (Thread_polling);
	  
	  // check if thread has been reset
          if (state() & Thread_cancel)
	    {
	      sender_dequeue (partner->sender_list());
	      error_ret = Ipc_err::Secanceled;
	      break;
	    }
	  
	  // detect if we have timed out
	  if (timeout.has_hit())
	    {
	      sender_dequeue (partner->sender_list());
	      error_ret = Ipc_err::Setimeout;
	      break;
	    }
	  
	  // OK, it's our turn to try again to send the message
	  error_ret = ipc_send_regs (partner, regs);
	}
      while ((error_ret & 0x80000000)); // transient error
				// re-enter loop even if !(state&polling)
				// for error handling
  
      if (timeout.is_set())
	timeout.reset();
      // set _timeout to 0 to avoid a dangling pointer
      set_timeout(0);
      
    }
  
  if (in_sender_list())
    {
      assert (error_ret != 0);
      sender_dequeue (partner->sender_list());
    }

  assert (!in_sender_list());
  
  Ipc_err ret (error_ret);

  // are we in the middle of a long IPC?
  if (EXPECT_FALSE (error_ret == 0 && (state() & Thread_send_in_progress)))
    {
      CNT_IPC_LONG;
      ret = do_send_long (partner, regs);
    }

  // Do not delete ipc_in_progress flag if we're still receiving.
  state_del (Thread_ipc_sending_mask | ((state() & Thread_ipc_receiving_mask)
				       ? 0 
				       : Thread_ipc_in_progress));

  return ret;      
}

/** Prepare an IPC-receive operation.
    This method must be called before do_receive() and, when carrying out
    a combined snd-and-receive operation, also before do_send().
    @param partner IPC partner we want to receive a message from.  0 if
                   we accept IPC from any partner (``open wait'').
    @param regs receiver's IPC registers
 */
PRIVATE inline
void Thread::prepare_receive(Sender *partner,
			     Sys_ipc_frame *regs)
{
#if 0
  if (partner && partner->state() == Thread_invalid) // partner nonexistent?
    return;			// just return
  // don't need to signal error here -- it will be detected later
#endif

  setup_receiver (partner, regs);

  // if we don't have a send, get us going here; otherwise, the send
  // operation will set the Thread_ipc_in_progress flag after it has
  // initialized
  if (!regs->has_snd())
    {
      state_add (Thread_ipc_in_progress);
      if (state() & Thread_cancel)
	{
	  state_del (Thread_ipc_in_progress);
	}
    }
}

/** Receive an IPC message.  Block until we can receive a message or the 
    timeout hits.  Before calling this function, the thread needs to call
    prepare_receive().
    The sender and regs arguments must correspond to those supplied to 
    prepare_receive().
    @param sender IPC partner we want to receive a message from.  0 if
                  we accept IPC from any partner (``open wait'').
    @param t a timeout specifier
    @param regs receiver's IPC registers
 */
PRIVATE 
Ipc_err
Thread::do_receive (Sender *sender, L4_timeout t, Sys_ipc_frame *regs)
{
  assert(! (state() & Thread_ipc_sending_mask));

  IPC_timeout timeout;

  while ((state() & (Thread_receiving | Thread_ipc_in_progress | Thread_cancel))
	 == (Thread_receiving | Thread_ipc_in_progress))
    {

      deny_lipc();
      // LIPC: updating of the sender is superfluous, because every Thread
      // uses kernel IPC if someone is waiting for this receiver

      // Add "busy" flag for detection of race with interrupts that
      // occur after our sender-has-been-queued check.
      if (!state_change_safely(~(Thread_receiving | Thread_ipc_in_progress), 
			       Thread_busy | Thread_receiving 
			       | Thread_ipc_in_progress))
	{
	  // we're not in "Thread_receiving | ipc_in_progress" state anymore
	  // -- initial handshake completed or IPC aborted.
	  break;
	}

      // If a sender has been queued, wake it up
      Sender *first = * sender_list(); // find a sender
      if (first)
	{
	  if (!sender || sender == first) // first is acceptible sender
	    first->ipc_receiver_ready(); // wake it up

	  else if (sender->in_sender_list() // sender in list, but not first?
		   && this == sender->receiver())
	    sender->ipc_receiver_ready(); // wake it up
	}
      
      // Program timeout
      if (!_timeout)
        {
          // we're not sleeping forever
          if (t.rcv_exp())
            {
              Unsigned64 tval = rcv_timeout (t, regs);

              // Zero timeout or timeout expired already -- give up
              if (tval == 0)
	        {
                  state_del (Thread_ipc_in_progress | Thread_busy);
                  break;
                }

              // enqueue timeout
              set_timeout (&timeout);
              timeout.set (tval);
            }
        }

      if (! state_change_safely (~(Thread_ipc_in_progress | Thread_receiving 
				   | Thread_ready | Thread_busy), 
				 Thread_ipc_in_progress | Thread_receiving))
	{
	  state_del (Thread_busy);
	  continue;
	}

      maybe_enable_lipc();
      // Go to sleep
      schedule();
    }

  if (timeout.is_set())
    timeout.reset();
  // set _timeout to 0 to avoid a dangling pointer
   set_timeout(0);

  while ((state() & (Thread_receiving | Thread_rcvlong_in_progress 
		     | Thread_ipc_in_progress))
	 == (Thread_rcvlong_in_progress | Thread_ipc_in_progress)) // long IPC?
    {
      // XXX handle cancel: should notify sender -- even if we're killed!

      assert (pagein_addr() != (Address) -1);

      Ipc_err ipc_code = handle_page_fault_pager (pagein_addr(),
						  pagein_error_code());

      clear_pagein_request();
      
      state_add (Thread_busy_long);

      // resume IPC by waking up sender
      if (pagein_applicant()->ipc_continue(ipc_code).has_error())
	break;

      // XXX ipc_continue could already put us back to sleep...  This
      // would save us another context switch and running this code:

      if (state_change_safely(~(  Thread_ipc_in_progress 
				| Thread_rcvlong_in_progress 
				| Thread_busy_long | Thread_ready),
			          Thread_ipc_in_progress
				| Thread_rcvlong_in_progress))
	{
	  schedule();
	}
    }

  assert(! (state() & (Thread_ipc_in_progress | Thread_ipc_sending_mask)));

  // abnormal termination?
  if (EXPECT_FALSE (state() & Thread_ipc_receiving_mask))
    {
      // the IPC has not been finished.  could be timeout or cancel
      // XXX should only modify the error-code part of the status code

      if (state() & Thread_busy)	// we've presumably been reset!
        commit_ipc_success (regs, Ipc_err::Reaborted);

      else if (state() & Thread_cancel)	// we've presumably been reset!
        commit_ipc_success (regs, Ipc_err::Recanceled);

      else
	commit_ipc_success (regs, Ipc_err::Retimeout);

      state_del (Thread_ipc_receiving_mask);
    }

  return get_ipc_err (regs);		// sender puts return code here
}

/** Page fault handler.
    This handler suspends any ongoing IPC, then sets up page-fault IPC.
    Finally, the ongoing IPC's state (if any) is restored.
    @param pfa page-fault virtual address
    @param error_code page-fault error code.
 */
PRIVATE 
Ipc_err
Thread::handle_page_fault_pager(Address pfa, Mword error_code)
{
  assert(!(state() & Thread_lipc_ready));
  
  // do not handle user space page faults from kernel mode if we're
  // already handling a request
  if (EXPECT_FALSE(!PF::is_usermode_error(error_code)
		   && thread_lock()->test()))
    {
      panic("page fault in locked operation");
    }

  // set up a register block used as an IPC parameter block for the
  // page fault IPC
  Sys_ipc_frame r;

  // save the UTCB fields affected by PF IPC
  Pf_msg_utcb_saver saved_utcb_fields(utcb());

  // create the page-fault message
  page_fault_msg(r, pfa, regs()->ip(), error_code);

  L4_timeout timeout(L4_timeout::Never);
  
  // This might be a page fault in midst of a long-message IPC operation.
  // Save the current IPC state and restore it later.
  Sender *orig_partner;
  Sys_ipc_frame *orig_rcv_regs;
  save_receiver_state (&orig_partner, &orig_rcv_regs);

  Receiver *orig_snd_partner = receiver();
  Timeout *orig_timeout = _timeout;
  if (orig_timeout)
    orig_timeout->reset();
  unsigned orig_ipc_state = state() & Thread_ipc_mask;
  state_del (orig_ipc_state);
  if (orig_ipc_state)
    timeout = _pf_timeout;	// in long IPC -- use pagefault timeout

  prepare_receive(_pager, &r);

  Ipc_err err = do_send(_pager, timeout, &r);
  Ipc_err ret(0);

  if (EXPECT_FALSE(err.has_error()))
    {
      if (Config::conservative)
	{
	  printf(" page fault send error = 0x%lx\n", err.raw());
	  kdb_ke("snd to pager failed");
	}

      // skipped receive operation
      state_del(Thread_ipc_receiving_mask);

      // PF from kernel mode? -> always abort
      if (!PF::is_usermode_error(error_code))
	{
	  // Translate a number of error codes so that they make sense
	  // in the long-IPC page-in case.
	  switch (err.error())
	    {
	    case Ipc_err::Setimeout:
	      ret.error(Ipc_err::Resndpfto);  break;
	    case Ipc_err::Enot_existent:
	      ret.error(Ipc_err::Reaborted);  break;
	    default:
	      ret.error(err.error());
	    }
	}
      else if (err.error() == Ipc_err::Enot_existent)
	ret = (state() & Thread_cancel)
	  ? Ipc_err (0) // retry user insn after thread_ex_regs
	  : err;
      // else ret = 0 -- that's the default, and it means: retry
    }
  else
    {
      err = do_receive(_pager, timeout, &r);

      if (EXPECT_FALSE(err.has_error()))
	{
	  if (Config::conservative)
	    {
	      printf("page fault rcv error = 0x%lx\n", err.raw());
	      kdb_ke("rcv from pager failed");
	    }
	  // PF from kernel mode? -> always abort
	  if (!PF::is_usermode_error(error_code))
	    {
	      // Translate a number of error codes so that they make sense
	      // in the long-IPC page-in case.
	      switch (err.error())
		{
		case Ipc_err::Retimeout:
		  ret = Ipc_err(Ipc_err::Rercvpfto);  break;
		case Ipc_err::Enot_existent:
		  ret = Ipc_err(Ipc_err::Reaborted);  break;
		default:
		  ret = err;
		}
	    }
	}

      // If the pager rejects the mapping, it replies -1 in msg.w0
      else if (EXPECT_FALSE (r.msg_word (utcb(), 0) == (Mword) -1))
        ret = Ipc_err(Ipc_err::Reaborted);

      // else ret = 0 -- that's the default, and it means: retry
    }

  // restore previous IPC state

  saved_utcb_fields.restore(utcb());
  
  set_receiver(orig_snd_partner);
  restore_receiver_state(orig_partner, orig_rcv_regs);
  state_add(orig_ipc_state);
  if (orig_timeout)
    orig_timeout->set_again();

  LOG_PF_RES_USER;

  return ret;
}

IMPLEMENT inline NEEDS ["map_util.h", Thread::wake_receiver,
			Thread::copy_utcb_to, Thread::unlock_receiver]
Mword
Thread::ipc_send_regs (Thread* receiver, Sys_ipc_frame const *sender_regs)
{
  Mword ret;

  ret = receiver->ipc_try_lock (nonull_static_cast<Sender*>(current_thread()));

  if (EXPECT_FALSE (ret))
    {
      if ((Smword) ret < 0)
	return 0x80000000;	// transient error -- partner not ready

      return ret;
    }

  if (EXPECT_FALSE (state() & Thread_cancel))
    {
      receiver->ipc_unlock();
      return Ipc_err::Secanceled;
    }

  receiver->ipc_init(nonull_static_cast<Sender*>(current_thread()));

  if (!Config::deceit_bit_disables_switch && sender_regs->snd_desc().deceite())
    panic ("deceiving ipc");	// XXX unimplemented

  Sys_ipc_frame* dst_regs = receiver->rcv_regs();
  const L4_msgdope ret_dope(Sys_ipc_frame::num_reg_words(), 0);

  ret = 0;				// status code: IPC successful
  dst_regs->msg_dope (ret_dope);	// status code: rcv'd 2 dw

  // dequeue from sender queue if enqueued
  sender_dequeue (receiver->sender_list());

  // Reset sender's timeout, if any.  Once we're here, we don't want
  // the timeout to reset the sender's ipc_in_progress state bit
  // (which it still needs for a subsequent receive operation).
  Timeout *t = _timeout;
  if (EXPECT_FALSE (t != 0))
    {
      t->reset();
      if (t->has_hit())		// too late?
	{
	  // Fix: re-set the Thread_ipc_in_progress flag.  The
	  // following algorithm makes sure we only set this flag if
	  // Thread_cancel has not been set in-between.
	  state_add (Thread_ipc_in_progress);
	  if (state() & Thread_cancel)
	    state_del (Thread_ipc_in_progress);
	}
    }

  // copy message register contents
  sender_regs->copy_msg (dst_regs);

  copy_utcb_to(receiver);

  // copy sender ID
  dst_regs->rcv_src (id());

  // disallow lipc of the receiver
  receiver->deny_lipc();

  // is this a fast (registers-only) receive operation?
  // the following operations can be short-cut here:
  // - short message sender to short message receiver
  // - (short) register message sender to long message receiver
  // the following operations can not be short-cut as we're not allowed
  // to touch user memory:
  // - long message sender to long message receiver (obviously)
  // - long message sender to short message receiver
  //   (the long message might actually fit into a short message)
  // - short-flexpage message sender to long message receiver
  //   (because the rcvr's msg buffer may contain a flexpage option)

  // There's a possible optimization for
  // short-flexpage-to-long-message-buffer transfers; see the top of
  // this file.

  if (EXPECT_TRUE (!sender_regs->snd_desc().is_long_ipc())
        // sender wants short register IPC
      || EXPECT_TRUE (sender_regs->snd_desc().msg() == 0
		       // sender wants short IPC
		       && (dst_regs->rcv_desc().is_register_ipc()
		            // receiver wants register IPC
			   || dst_regs->rcv_desc().rmap()
			       // receiver wants short flexpage
			 )
	))
    {
      // short IPC!

      if (sender_regs->snd_desc().map()) // send short flexpage?
	{
	  if (EXPECT_FALSE (! dst_regs->rcv_desc().rmap()) )
	      // rcvr not expecting an fpage?
	    {
	      dst_regs->msg_dope_set_error(Ipc_err::Remsgcut);
	      ret = Ipc_err::Semsgcut;
	    }
	  else
	    {
	      dst_regs->msg_dope_combine(
	        fpage_map(space(),
			  L4_fpage(sender_regs->msg_word(1)),
			  receiver->space(), dst_regs->rcv_desc().fpage(),
			  sender_regs->msg_word(0) & Config::PAGE_MASK,
			  L4_fpage(sender_regs->msg_word(1)).grant()));

	      if (dst_regs->msg_dope().rcv_map_failed())
		ret = Ipc_err::Semapfailed;
	    }
	}

      /* set the source local-id */
      set_source_local_id(receiver, dst_regs);

      // (If we don't send a flexpage in our message, that's OK even
      // if the receiver expected one.  The receiver can tell from the
      // status code that he didn't receive one.)

      // IPC done -- reset states

      // After this point we are able to receive!
      // Make sure that we are not if there was a send error.
      state_del ((ret != 0 ? Thread_ipc_in_progress : 0) |
		  Thread_polling | Thread_send_in_progress);

      wake_receiver (receiver);

      if (!Config::deceit_bit_disables_switch ||
	  !sender_regs->snd_desc().deceite())
	  receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_LOCKEE);
    }
  else
    {
      // prepare long IPC

      // XXX check for cancel -- sender?  receiver?

      _target_desc = dst_regs->rcv_desc(); //ebp & ~1;

      // If the receive timeout has hit, ignore it.  We do this by
      // overwriting the Thread_ipc_in_process flag which the timeout
      // may have deleted -- but only if the Thread_cancel flag has
      // not been set in-between.

      receiver->reset_timeout();

      // set up page-fault timeouts
      L4_timeout t = sender_regs->timeout();
      if (t.snd_pfault() == 15) // send pfault timeout == 0 ms?
	receiver->_pf_timeout = L4_timeout(0,1,0,1,0,0);
      else
	{
	  receiver->_pf_timeout = L4_timeout(1, t.snd_pfault(),
					     1, t.snd_pfault(), 0, 0);
	  // XXX should normalize timeout spec, but do_send/do_receive
	  // can cope with non-normalized numbers.
	}

      t = dst_regs->timeout();
      if (t.rcv_pfault() == 15) // rcv pfault timeout == 0 ms?
	_pf_timeout = L4_timeout(0,1,0,1,0,0);
      else
	{
	  _pf_timeout = L4_timeout(1, t.rcv_pfault(), 1, t.rcv_pfault(), 0, 0);
	  // XXX should normalize timeout spec, but do_send/do_receive
	  // can cope with non-normalized numbers.
	}

      // set up return code in case we're aborted
      dst_regs->msg_dope_set_error(Ipc_err::Reaborted);

      // switch receiver's state, and put it to sleep.
      // overwrite ipc_in_progress flag a timeout may have deleted --
      // see above
      receiver->state_change(~(Thread_receiving | Thread_busy | Thread_ready),
			     Thread_rcvlong_in_progress
			     | Thread_ipc_in_progress);
      if (receiver->state() & Thread_cancel)
	  receiver->state_change(~Thread_ipc_in_progress, Thread_ready);

      // in the sender, send_in_progress must remain set
      state_del(Thread_polling);
    }

  unlock_receiver(receiver, sender_regs);
  return ret;
}

/** Short cut for L4 IPC system call (short IPC).
    For the special case of short IPC we try to avoid
    the full blown overhead of the interruptible ipc scheme.
    It's only applicable if the following requirements are all met:
      - short IPC send    by the sender
      - short IPC receive by the sender (optional, no receive also possible)
        except open wait in threads that can receive msgs from IRQs
      - no receive timeout in case of receive part
      - dst is ready to receive
      - dst is a valid, non-nil thread

    @return true if shortcut was successful, false if shortcut is not
            applicable and standard implementation IPC handling should be
	    used instead.
 */

IMPLEMENT inline NOEXPORT ALWAYS_INLINE
bool
Thread::ipc_short_cut()
{
  Sys_ipc_frame *regs = sys_frame_cast<Sys_ipc_frame>(this->regs());
  L4_timeout t;

  // Locate the receiver of the send part
  L4_uid dst_id(regs->snd_dst());

  if (EXPECT_FALSE
      (// Check send options:
       (!Config::deceit_bit_disables_switch &&
	(regs->snd_desc().deceite() | regs->snd_desc().is_long_ipc()))
				// long or deceiving send op or no send at all
       || (Config::deceit_bit_disables_switch && regs->snd_desc().is_long_ipc())
				// long send op or no send at all
       || dst_id.is_invalid()	// L4_INVALID_ID
       || dst_id.is_nil()	// L4_NIL_ID
       || dst_id.is_irq()	// Interrupt ID
       || dst_id.next_period()  // next period IPC
       // Check receive options:
       || (regs->rcv_desc().has_receive() // have receive part
	  && (!regs->rcv_desc().is_register_ipc() // rcv more than short ipc
	      || (Config::deceit_bit_disables_switch
		  && regs->snd_desc().deceite())  // only rcv + don't switch
	      || regs->has_abs_rcv_timeout()
	      || (t = regs->timeout(), // timeout != 0, != infin.
		  t.rcv_exp() && t.rcv_man())
	      // We never call sender_list()->ipc_receiver_ready(), so
	      // we have to rule out cases where we would normally call it.
	      || (regs->rcv_desc().open_wait() // open wait and...
		  && (*sender_list() || _irq)) // ... sender queued or irq atch
	    ))))
    {
      CNT_SHORTCUT_FAILED;
      LOG_SHORTCUT_FAILED_1;
      return false;
    }

  Thread *dst = lookup (dst_id);

  // From now on, we need a consistent receiver state.  Lock it down.
  // We don't need the cpu_lock because we are still cli'd.

  register bool can_preempt = false;
  if (EXPECT_FALSE(    !dst->is_tcb_mapped()
			// don't take the shortcut if
		    || (   Config::deceit_bit_disables_switch
		        && regs->snd_desc().deceite()
			&& (can_preempt = can_preempt_current(dst->sched())))
		    || !dst->sender_ok(this)
		    ||  dst->thread_lock()->test()  // destination locked?
		    || (dst->state() & (Thread_delayed_deadline |
			 Thread_delayed_ipc))))
    {
      // no short cut possible, fall back to the conservative implementation
      CNT_SHORTCUT_FAILED;
      LOG_SHORTCUT_FAILED_2;
      return false;
    }

  // Carry out the send operation

  // If the dst thread is in the correct state, then rcv_regs()
  // is supposed to point to its (user) register set
  Sys_ipc_frame* dst_regs = dst->rcv_regs();
  const L4_msgdope ret_dope(Sys_ipc_frame::num_reg_words(), 0);

  // copy the short msg directly into the registers of the receiver
  dst_regs->msg_dope (ret_dope);	// status code: rcv'd 2 dw
  regs->copy_msg (dst_regs);

  copy_utcb_to(dst);

  // copy sender ID
  dst_regs->rcv_src( current_thread()->id() );

  dst->deny_lipc();
  // set the sender local id, necessary for efficient LIPC.
  set_source_local_id(dst, dst_regs);

  // wake up receiver
  dst->state_change_dirty (~(Thread_ipc_receiving_mask
			      | Thread_ipc_in_progress),
			    Thread_ready);

  // Prepare a receive if we have one.

  unsigned deceit_bit_set = regs->snd_desc().deceite();

  if (EXPECT_TRUE( regs->rcv_desc().has_receive()) ) // receive part ?
    {
      register unsigned ipc_state =
	setup_receiver_state (regs->rcv_desc().open_wait() ? 0 : dst, regs);

      // change our state to waiting / receiving
      state_change_dirty (~0U, Thread_ipc_in_progress | ipc_state);

      if (t.rcv_exp() == 0)	// timeout == infin.?
	{
	  regs->msg_dope(Ipc_err::Recanceled);
	  state_change_dirty (~Thread_ready, 0); // Need wakeup.
	  allow_lipc();
	}
      else			// timeout == 0
	regs->msg_dope(Ipc_err::Retimeout);
    }
  else
    regs->msg_dope(0);		// No receive part -> status = OK

  CNT_SHORTCUT_SUCCESS;
  LOG_SHORTCUT_SUCCESS;

  // we don't get here if (Config::deceit_bit_disables_switch &&
  // deceit_bit_set && can_preempt_current (dst->sched)
  if (!Config::deceit_bit_disables_switch || !deceit_bit_set)
    switch_exec_locked (dst, Not_Helping);
  else
    dst->ready_enqueue ();

  state_change_dirty (~Thread_ipc_mask, 0);
  return true;
}

/** L4 IPC system call.
    This is the `normal'' version of the IPC system call.  It usually only
    gets called if ipc_short_cut() has failed.
    @param regs system-call arguments.
    @return value to be returned in %eax register.
 */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Thread::sys_ipc()
{
  Sys_ipc_frame *regs = sys_frame_cast<Sys_ipc_frame>(this->regs());

  Ipc_err ret(0);
  L4_timeout t = regs->timeout();

  // find the ipc partner thread belonging to the destination id
  L4_uid id              = regs->snd_dst();
  Thread *partner        = lookup (id);
  bool have_receive_part = regs->rcv_desc().has_receive();
  bool have_send_part    = regs->snd_desc().has_snd();
  bool have_sender       = false;
  Sender *sender         = 0;
  Irq_alloc *interrupt   = 0;

  // Add Thread_delayed_* flags if this a "next-period" IPC.
  // The flags must be cleared again on all exit paths from this function.
  if (EXPECT_FALSE (id.next_period()))
    {
      state_add (Thread_delayed_deadline);

      if (id.is_nil() && t.rcv_exp() && !t.rcv_man() &&
          !regs->has_abs_rcv_timeout())		// next period, 0 timeout
        goto success;

      if (mode() & Nonstrict)
        state_add (Thread_delayed_ipc);
    }

  // first, before starting a snd, we must prepare a pending receive
  // so that we can atomically switch from send mode to receive mode

  if (EXPECT_TRUE (have_receive_part))
    {
      if (regs->rcv_desc().open_wait())		// open wait
        {
	  // sender already 0
          have_sender = true;
        }

      else if (EXPECT_FALSE (id.is_nil()))	// nil id
        {
          // only prepare IPC for timeout != 0
          if (!t.rcv_exp() || t.rcv_man() || regs->has_abs_rcv_timeout())
            {
              sender = partner;
              have_sender = true;
            }
        }

      else if (EXPECT_FALSE (id.is_irq()))	// interrupt id
        {
          interrupt = Irq_alloc::lookup (regs->irq());

	  if (EXPECT_FALSE (!interrupt))
	    {
	      commit_ipc_failure (regs, Ipc_err::Enot_existent);
	      return;
	    }

	  if (interrupt->owner() != this)
	    {
	      // a receive with a timeout != 0 from a non-associated
	      // interrupt is illegal
	      if (!t.rcv_exp() || t.rcv_man()
		  || regs->has_abs_rcv_timeout())
		{
		  commit_ipc_failure (regs, Ipc_err::Enot_existent);
		  return;
		}
	    }

	  if (interrupt->owner() == this)
	    {
	      // we always try to receive from the
	      // assoc'd irq first, not from the spec. one
	      sender = nonull_static_cast<Irq*>(interrupt);
	      have_sender = true;
	    }
        }

      else					// regular thread id
        {
          if (EXPECT_FALSE (!partner || partner->state() == Thread_invalid))
            {
              commit_ipc_failure (regs, Ipc_err::Enot_existent);
              return;
            }

          if (EXPECT_FALSE (id.is_preemption()))
            sender = partner->preemption();
          else
            sender = partner;

          have_sender = true;
        }

      if (have_sender)
        prepare_receive (sender, regs);
    } //  if (EXPECT_TRUE (have_receive_part))

  // The send part.  Be careful here: We must not test the parameter
  // in the %eax register here because %eax may already have been
  // overwritten by a sender if this IPC operation has a receive part
  // but no send part (because in this case, prepare_receive() already
  // activates the receive part).  That's why, we save that parameter
  // early (have_send_part).

  if (EXPECT_TRUE (have_send_part))		// do we do a send operation?
    {
      if (EXPECT_FALSE(Config::Multi_irq_attach && id.is_irq()))
	{
	  Irq_alloc *irq = Irq_alloc::lookup(regs->irq());
	  if (EXPECT_FALSE(!irq || irq->owner() != this))
	    {
	      // failed -- could not associate new irq
	      commit_ipc_failure (regs, Ipc_err::Enot_existent);
	      return;
	    }

	  if (regs->msg_word(0) == 0) // ack IRQ
	    irq->acknowledge();
	  else if (regs->msg_word(0) == 1) // disassociate IRQ
	    disassociate_irq(irq);
	  else
	    {
	      // failed -- could not associate new irq
	      commit_ipc_failure (regs, Ipc_err::Enot_existent);
	      return;
	    }

	  if (have_sender && have_receive_part)
	    {
	      state_add (Thread_ipc_in_progress);
	      if (state() & Thread_cancel)
		state_del (Thread_ipc_in_progress);
	    }
	}
      else
        ret = do_send (partner, t, regs);
    }
  else
    // make sure that there are no reamining parts of the send descriptor
    regs->msg_dope(0);

  // send done, receive operation follows

  if (EXPECT_TRUE (have_receive_part         // do we do a receive operation?
		   && !ret.has_error()) )    // no send error
    {
      if (have_sender)
        {
          // do the receive operation
          ret = do_receive (sender, t, regs);

          // if this was a receive from an interrupt and the timeout was 0,
          // this is a re-associate operation
          if (EXPECT_TRUE (ret.error() != Ipc_err::Retimeout ||
                           !interrupt ||
                           !t.rcv_exp() || t.rcv_man() ||
			   regs->has_abs_rcv_timeout()))
            goto success;
        }

      else if (! interrupt)     // rcv from nil; this also means t/o == 0
        {
          //
          // disassociate from irq
	  // ! With Multi_irq_attach disassociate from all IRQs !
          //
          if (_irq)
            {
	      Irq_alloc::free_all(this);
              _irq = 0;
            }

	  commit_ipc_failure (regs, Ipc_err::Retimeout);
          return;
        }

      //
      // associate with an interrupt
      //
      if (associate_irq(interrupt))
	{
	  // success
	  commit_ipc_failure (regs, Ipc_err::Retimeout);
          return;
        }

      // failed -- could not associate new irq
      commit_ipc_failure (regs, Ipc_err::Enot_existent);
      return;
    }

  // skipped receive operation
  state_del (Thread_ipc_mask);

success:
  commit_ipc_success (regs, ret);

  // New period doesn't begin as long as Thread_delayed_deadline is still set
  while (state_change_safely (~(Thread_delayed_deadline | Thread_ready),
                                Thread_delayed_deadline))
    schedule();
}

extern "C"
void
sys_ipc_wrapper()
{
  // Don't allow interrupts before we've got a call frame with a return
  // address on our stack, so that we can tweak the return address for
  // sysenter + sys_ex_regs to the iret path.
  Proc::sti();

  current_thread()->sys_ipc();

  assert (!(current()->state() &
           (Thread_delayed_deadline | Thread_delayed_ipc)));

  // If we return with a modified return address, we must not be interrupted
  Proc::cli();
}

extern "C"
void
ipc_short_cut_wrapper()
{
  register Thread *const ct = current_thread();

  if (EXPECT_TRUE (ct->ipc_short_cut()))
    return;

  Proc::sti();

  ct->sys_ipc();

  // If we return with a modified return address, we must not be interrupted
  Proc::cli();
}

IMPLEMENT inline
void
Thread::page_fault_msg (Sys_ipc_frame &r, Address fault_address,
			Mword fault_eip, Mword error_code)
{
  r.set_msg_word (0, PF::addr_to_msgword0 (fault_address, error_code));
  r.set_msg_word (1, PF::pc_to_msgword1 (fault_eip, error_code));
  r.set_msg_word (2, 0); // nop in V2
  r.snd_desc (0);	// short msg
  r.rcv_desc(L4_rcv_desc::short_fpage(L4_fpage(0,0,L4_fpage::Whole_space,0)));
};

IMPLEMENT inline
void
Thread::commit_ipc_success (Sys_ipc_frame *regs, Ipc_err err)
{
  regs->msg_dope (regs->msg_dope().raw_dope() | err.raw());
}

IMPLEMENT inline
void
Thread::commit_ipc_failure (Sys_ipc_frame *regs, Ipc_err err)
{
  state_del (Thread_delayed_deadline | Thread_delayed_ipc);
  regs->msg_dope (0);
  commit_ipc_success (regs, err);
}

IMPLEMENT inline
Ipc_err
Thread::get_ipc_err (Sys_ipc_frame *regs)
{
  return Ipc_err (regs->msg_dope().raw());
}

//---------------------------------------------------------------------------
IMPLEMENTATION [v2-lipc]:

IMPLEMENT inline
Pf_msg_utcb_saver::Pf_msg_utcb_saver (const Utcb *u)
{
  ipc_state = u->state();
}

IMPLEMENT inline
void Pf_msg_utcb_saver::restore (Utcb *u)
{
  u->state(ipc_state);
}


//---------------------------------------------------------------------------
IMPLEMENTATION[!lipc]:
/** Dummy function, needed to hold code in Thread generic.
 */
IMPLEMENT inline
void
Thread::set_source_local_id (Thread *, Sys_ipc_frame *)
{}

IMPLEMENT inline
Pf_msg_utcb_saver::Pf_msg_utcb_saver (const Utcb *)
{}

IMPLEMENT inline
void
Pf_msg_utcb_saver::restore (Utcb *)
{}

/** Unlock the Receiver locked with ipc_try_lock().
    If the sender goes to wait for a registered message enable LIPC.
    @param receiver receiver to unlock
    @param sender_regs dummy
 */
PRIVATE inline NEEDS ["entry_frame.h"]
void
Thread::unlock_receiver (Receiver *receiver, const Sys_ipc_frame*)
{
  receiver->ipc_unlock();
}

