INTERFACE:
class Sys_ipc_frame;

EXTENSION class Thread
{
private:
  bool ipc_short_cut( Sys_ipc_frame*) asm ("call_ipc_short_cut");
};


IMPLEMENTATION [ipc]:

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
#include "timer.h"
#include "cpu_lock.h"
#include "dirq.h"
#include "lock_guard.h"
#include "map_util.h"
#include "logdefs.h"
#include "processor.h"
//#include "regdefs.h"

// OSKIT crap
#include "undef_oskit.h"


/** IPC rendezvous. 
    This method sets up an IPC.  It also finishes the IPC if it was a short
    IPC.
    @param receiver IPC receiver
    @param sender_regs sender's IPC registers
    @return IPC error code that should be returned to the sender.
            If the IPC could not be set up because of a transient error
	    (e.g., IPC partner not ready to receive), returns 0x80000000.
 */
PRIVATE 
Mword Thread::ipc_send_regs(Thread* receiver,
			    Sys_ipc_frame const *sender_regs)
{
  Mword ret;

  ret = receiver->ipc_try_lock (nonull_static_cast<Sender*>(current_thread()));
 
  if (EXPECT_FALSE (ret) )
    {
      if ((Smword)ret < 0)
	return 0x80000000;	// transient error -- partner not ready

      return ret;
    }

  if (! Config::deceit_bit_disables_switch 
      && (sender_regs->snd_desc().deceite()))
    {
      panic("deceiving ipc");	// XXX unimplemented
    }

  Sys_ipc_frame* dest_regs = receiver->receive_regs();
  const L4_msgdope ret_dope(Sys_ipc_frame::num_reg_words(), 0);

  ret = 0;				// status code: IPC successful
  dest_regs->msg_dope (ret_dope);	// status code: rcv'd 2 dw

  // dequeue from sender queue if enqueued
  sender_dequeue(receiver->sender_list()); 

  // Reset sender's timeout, if any.  Once we're here, we don't want
  // the timeout to reset the sender's ipc_in_progress state bit
  // (which it still needs for a subsequent receive operation).
  timeout_t *t = _timeout;
  if (EXPECT_FALSE (t != 0) )
    {
      t->reset();
      if (t->has_hit())		// too late?
	{		
	  // Fix: re-set the Thread_ipc_in_progress flag.  The
	  // following algorithm makes sure we only set this flag if
	  // Thread_cancel has not been set in-between.
	  state_add(Thread_ipc_in_progress);
 	  if (state() & Thread_cancel)
	    {
	      state_del(Thread_ipc_in_progress);
	    }
	}
    }

  // copy message register contents
  sender_regs->copy_msg( dest_regs );

  // copy sender ID
  dest_regs->rcv_source( id() );

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
		       && (dest_regs->rcv_desc().is_register_ipc() 
		            // receiver wants register IPC
			   || dest_regs->rcv_desc().rmap()
			       // receiver wants short flexpage
			 )
	))
    {
      // short IPC!

      if (sender_regs->snd_desc().map()) // send short flexpage?
	{
	  if (EXPECT_FALSE (! dest_regs->rcv_desc().rmap()) ) 
	      // rcvr not expecting an fpage?
	    {
	      dest_regs->msg_dope_set_error(L4_msgdope::REMSGCUT);
	      ret = L4_msgdope::SEMSGCUT;
	    }
	  else
	    {
	      dest_regs->msg_dope_combine( 
  	        fpage_map(space(), 
			  L4_fpage(sender_regs->msg_word(1)),
			  receiver->space(), dest_regs->rcv_desc().fpage(),
			  sender_regs->msg_word(0) & Config::PAGE_MASK));
	  
	      if (dest_regs->msg_dope().rcv_map_failed())
		ret = L4_msgdope::SEMAPFAILED;
	    }
	}
 
      // (If we don't send a flexpage in our message, that's OK even
      // if the receiver expected one.  The receiver can tell from the
      // status code that he didn't receive one.)

      // IPC done -- reset states
      state_del(Thread_polling 
		| Thread_send_in_progress);
      
      receiver->state_change(~Thread_ipc_mask, Thread_running);
    }
  else
    {
      // prepare long IPC

      // XXX check for cancel -- sender?  receiver?

      _target_desc = dest_regs->rcv_desc(); //ebp & ~1;

      // If the receive timeout has hit, ignore it.  We do this by
      // overwriting the Thread_ipc_in_process flag which the timeout
      // may have deleted -- but only if the Thread_cancel flag has
      // not been set in-between.

      receiver->reset_receive_timeout();
      
      // set up page-fault timeouts
      L4_timeout t = sender_regs->timeout();
      if (t.snd_pfault() == 15) // snd pfault timeout == 0 ms?
	receiver->_pf_timeout = L4_timeout(0,1,0,1,0,0);
      else
	{
	  receiver->_pf_timeout = L4_timeout(1, t.snd_pfault(), 
					     1, t.snd_pfault(), 0, 0);
	  // XXX should normalize timeout spec, but do_send/do_receive
	  // can cope with non-normalized numbers.
	}

      t = dest_regs->timeout();
      if (t.rcv_pfault() == 15) // rcv pfault timeout == 0 ms?
	_pf_timeout = L4_timeout(0,1,0,1,0,0);
      else
	{
	  _pf_timeout = L4_timeout(1, t.rcv_pfault(), 1, t.rcv_pfault(), 0, 0);
	  // XXX should normalize timeout spec, but do_send/do_receive
	  // can cope with non-normalized numbers.
	}

      // set up return code in case we're aborted
      dest_regs->msg_dope_set_error(L4_msgdope::REABORTED);

      // switch receiver's state, and put it to sleep.
      // overwrite ipc_in_progress flag a timeout may have deleted --
      // see above
      receiver->state_change(~(Thread_waiting | Thread_receiving
			       | Thread_busy | Thread_running), 
			     Thread_rcvlong_in_progress | Thread_ipc_in_progress);
      if (receiver->state() & Thread_cancel)
	{
	  receiver->state_change(~Thread_ipc_in_progress, Thread_running);
	}

      // in the sender, send_in_progress must remain set
      state_del(Thread_polling);
    }

  if (!Config::disable_automatic_switch 
      && (!Config::deceit_bit_disables_switch 
	  || !sender_regs->snd_desc().deceite()))
    {
      receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_RECEIVER);
    }

  receiver->ipc_unlock();
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
      - dest is ready to receive
      - dest is a valid, non-nil thread

    @param regs sender's IPC registers
    @return true if shortcut was successful, false if shortcut is not
            applicable and standard implementation IPC handling should be
	    used instead.
 */

IMPLEMENT
bool Thread::ipc_short_cut(Sys_ipc_frame *regs)
{
  L4_timeout t;

  state_change_dirty(~Thread_cancel, 0);
  
  // Locate the receiver of the send part
  threadid_t dst_id(regs->snd_dest());

  if (EXPECT_FALSE (
      // Check send options:
         (!Config::deceit_bit_disables_switch && 
	  (regs->snd_desc().deceite() | regs->snd_desc().is_long_ipc()))
      				// long or deceiving send op or no send at all
      || ( Config::deceit_bit_disables_switch && regs->snd_desc().is_long_ipc())
      				// long send op or no send at all
      || (! dst_id.is_valid())	// L4_INVALID_ID
      || dst_id.is_nil()	// L4_NIL_ID or interrupt ID
      // Check receive options:
      || (regs->rcv_desc().has_receive() // have receive part
	  && (!regs->rcv_desc().is_register_ipc() // recv more than short ipc
	      || (Config::deceit_bit_disables_switch
		  && regs->snd_desc().deceite())  // only recv + don't switch
	      || Config::disable_automatic_switch // only recv+don't switch
	          // don't switch to receiver on reply
	      || (t = regs->timeout(), // timeout != 0, != infin.
		  t.rcv_exp() && t.rcv_man())
	      // We never call sender_list()->ipc_receiver_ready(), so
	      // we have to rule out cases where we would normally call it.
	      || (regs->rcv_desc().open_wait() // open wait and...
		  && (_irq || *sender_list())) // sender queued or irq attached
	      ))) )
    {
      LOG_SHORTCUT_FAILED_1;
      return false;
    }

  Thread* dest  = dst_id.lookup();

#if defined(CONFIG_IA32) || defined(CONFIG_UX)
  // Touch the state to page in the TCB. If we get a pagefault here,
  // the handler doesn't handle it but returns immediatly after
  // setting eax to 0xffffffff
  Mword pagefault_if_0xffffffff;
  asm volatile (".globl c_short_cut		\n\t"
	        "c_short_cut:			\n\t"
	        ".globl pagein_tcb_request3	\n\t"
		"pagein_tcb_request3:		\n\t"
		"movl (%1),%%eax		\n\t"
		: "=a" (pagefault_if_0xffffffff) : "r" (&dest->_state));


  // From now on, we need a consistent receiver state.  Lock it down.
  // We don't need the cpu_lock because we are still cli'd.

  if (EXPECT_FALSE(    (pagefault_if_0xffffffff == 0xffffffff)
		    || (! dest->sender_ok (this))
		    || dest->thread_lock()->test()) )  // destination locked?
    {
      // no short cut possible, fall back to the conservative implementation
      LOG_SHORTCUT_FAILED_2;
      return false;
    }
#endif

  // Carry out the send operation

  // If the dest thread is in the correct state, then receive_regs()
  // is supposed to point to its (user) register set
  Sys_ipc_frame* dest_regs = dest->receive_regs();
  const L4_msgdope ret_dope(Sys_ipc_frame::num_reg_words(), 0);

  // copy the short msg directly into the registers of the receiver
  dest_regs->msg_dope( ret_dope );	// status code: rcv'd 2 dw
  regs->copy_msg( dest_regs );

  // copy sender ID
  dest_regs->rcv_source( current_thread()->id() );

  // wake up receiver
  dest->state_change_dirty (~(Thread_ipc_receiving_mask 
			      | Thread_ipc_in_progress),
			    Thread_running);

  // Prepare a receive if we have one.

  unsigned deceit_bit_set = regs->snd_desc().deceite();

  if (EXPECT_TRUE( regs->rcv_desc().has_receive()) ) // receive part ?
    {
      unsigned ipc_state =
	setup_receiver_state (regs->rcv_desc().open_wait() ? 0 : dest, regs);

      // change our state to waiting / receiving
      state_change_dirty (~0, Thread_ipc_in_progress | ipc_state);

      if (t.rcv_exp() == 0)	// timeout == infin.?
	{
	  regs->msg_dope(L4_msgdope::RECANCELED);
	  state_change_dirty (~Thread_running, 0); // Need wakeup.
	}
      else			// timeout == 0
	{
	  regs->msg_dope(L4_msgdope::RETIMEOUT);
	}
    }
  else
    regs->msg_dope(0);		// No receive part -> status = OK

  LOG_SHORTCUT_SUCCESS;

  if (EXPECT_TRUE(  (!Config::disable_automatic_switch
                    && (!Config::deceit_bit_disables_switch || !deceit_bit_set))
  	          || (dest->sched()->prio() > sched()->prio())) )
    switch_to (dest);
  else
    // don't switch to partner if deceiting bit is set and priority 
    // of partner is less than or equal to current priority
    dest->ready_enqueue();

  return true;
}


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

  if (! state_del(Thread_polling))
    return;

  state_add(Thread_running);

  _thread_lock.set_switch_hint(SWITCH_ACTIVATE_RECEIVER);
}

/** Send an IPC message.  Block until we can send the message or the timeout
    hits.
    @param partner the receiver of our message
    @param t a timeout specifier
    @param regs sender's IPC registers
    @return sender's IPC error code
 */
PRIVATE 
L4_msgdope Thread::do_send(Thread *partner, L4_timeout t, Sys_ipc_frame *regs)
{
  if (!partner || partner->state() == Thread_invalid // partner nonexistent?
      || partner == nil_thread) // special case: must not send to L4_NIL_ID
    return L4_msgdope(L4_msgdope::ENOT_EXISTENT);
      
  Mword error_ret;

  // register partner so that we can be dequeued by kill()
  set_receiver (partner);

  state_add(Thread_polling | Thread_ipc_in_progress 
	    | Thread_send_in_progress);

  if (EXPECT_FALSE (state() & Thread_cancel) )
    {
      state_del(Thread_ipc_sending_mask | Thread_ipc_in_progress);
      return L4_msgdope(L4_msgdope::SECANCELED);
    }

  sender_enqueue(partner->sender_list());

  // try a rendezvous with the partner
  error_ret = ipc_send_regs(partner, regs);

  if (EXPECT_FALSE (error_ret & 0x80000000) )	// transient error
    {
      // in case of a transient error, try sleeping
      
      timeout_t timeout;
      
      if (t.snd_exp() != 0) // not sleep forever?
	{
	  if (t.snd_man() == 0) // timeout = zero?
	    {
	      state_del(Thread_polling | Thread_ipc_in_progress
			| Thread_send_in_progress);
	      sender_dequeue(partner->sender_list());
	      return L4_msgdope(L4_msgdope::SETIMEOUT); // give up
	    }      
	  
	  // enqueue a timeout
	  _timeout = &timeout;
	  timeout.set(Kernel_info::kip()->clock + t.snd_microsecs());
	}
      
      do 
	{
	  // detect a reset thread here (before scheduling)
	  if (state() & Thread_cancel)
	    {
	      // we've presumably been reset!
	      sender_dequeue(partner->sender_list());
	      error_ret = L4_msgdope::SECANCELED;
	      break;
	    }

	  // go to sleep
	  if (state_change_safely(~(Thread_ipc_in_progress
				    | Thread_polling | Thread_running), 
				  Thread_ipc_in_progress | Thread_polling))
	    schedule();
	  
	  state_add(Thread_polling);
	  
	  // detect if we have been reset meanwhile
	  if (state() & Thread_cancel)
	    {
	      // we've presumably been reset!
	      sender_dequeue(partner->sender_list());
	      error_ret = L4_msgdope::SECANCELED;
	      break;
	    }
	  
	  // detect if we have timed out
	  if (timeout.has_hit())
	    {
	      sender_dequeue(partner->sender_list());
	      error_ret = L4_msgdope::SETIMEOUT;
	      break;
	    }
	  
	  // OK, it's our turn to try again to send the message
	  error_ret = ipc_send_regs(partner, regs);	      
	}
      while ((error_ret & 0x80000000)); // transient error
				// re-enter loop even if !(state&polling)
				// for error handling
      assert(! in_sender_list());
  
      if (timeout.is_set())
	timeout.reset();

      _timeout = 0;
    }

  assert(! in_sender_list());
  
  L4_msgdope ret(error_ret);

  // are we in the middle of a long IPC?
  if (EXPECT_FALSE( error_ret == 0 && (state() & Thread_send_in_progress)) )
    {
      // long IPC
      ret = do_send_long(partner, regs);
    }

  // Do not delete ipc_in_progress flag if we're still receiving.
  state_del(Thread_ipc_sending_mask | ((state() & Thread_ipc_receiving_mask)
				       ? 0 
				       : Thread_ipc_in_progress));

  return ret;      
}

/** Prepare an IPC-receive operation.
    This method must be called before do_receive() and, when carrying out
    a combined send-and-receive operation, also before do_send().
    @param partner IPC partner we want to receive a message from.  0 if
                   we accept IPC from any partner (``open wait'').
    @param regs receiver's IPC registers
 */
PRIVATE 
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
  if (!regs->snd_desc().has_send())
    {
      state_add(Thread_ipc_in_progress);
      if (state() & Thread_cancel)
	{
	  state_del(Thread_ipc_in_progress);
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
L4_msgdope Thread::do_receive(Sender *sender, L4_timeout t, 
			 Sys_ipc_frame *regs)
{
  assert(! (state() & Thread_ipc_sending_mask));

  unsigned ipc_state = sender ? Thread_receiving : Thread_waiting;

  timeout_t timeout;

  while ((state() & (ipc_state | Thread_ipc_in_progress | Thread_cancel))
	 == (ipc_state | Thread_ipc_in_progress))
    {
      // Add "busy" flag for detection of race with interrupts that
      // occur after our sender-has-been-queued check.
      if (!state_change_safely(~(ipc_state | Thread_ipc_in_progress), 
			       Thread_busy | ipc_state 
			       | Thread_ipc_in_progress))
	{
	  // we're not in "ipc_state | ipc_in_progress" state anymore
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
      
      // Program timeout.
      if (! _timeout && t.rcv_exp() != 0)
	{
	  if (t.rcv_man() == 0) // zero timeout
	    {
	      state_del (Thread_ipc_in_progress|Thread_busy);
	      break;
	    }

	  set_receive_timeout(&timeout);
      
	  timeout.set(Kernel_info::kip()->clock + t.rcv_microsecs());
	  
	}

      if (! state_change_safely(~(Thread_ipc_in_progress | ipc_state 
				  | Thread_running | Thread_busy), 
				Thread_ipc_in_progress | ipc_state))
	{
	  state_del(Thread_busy);
	  continue;
	}

      // Go to sleep
      schedule();
    }

  if (timeout.is_set())
    timeout.reset();

  set_receive_timeout(0);
  
  while ((state() & (ipc_state | Thread_rcvlong_in_progress 
			       | Thread_ipc_in_progress))
	 == (Thread_rcvlong_in_progress | Thread_ipc_in_progress)) // long IPC?
    {
      // XXX handle cancel: should notify sender -- even if we're killed!

      assert (pagein_request () != 0xffffffff);

      L4_msgdope ipc_code = handle_page_fault_pager(pagein_request(), 2);

      clear_pagein_request ();
      
      state_add (Thread_busy_long);

      // resume IPC by waking up sender
      if (pagein_applicant()->ipc_continue(ipc_code).has_error())
	break;

      // XXX ipc_continue could already put us back to sleep...  This
      // would save us another context switch and running this code:

      if (state_change_safely(~(  Thread_ipc_in_progress 
				| Thread_rcvlong_in_progress 
				| Thread_busy_long | Thread_running),
			          Thread_ipc_in_progress
				| Thread_rcvlong_in_progress))
	{
	  schedule();
	}
    }

  assert(! (state() & (Thread_ipc_in_progress | Thread_ipc_sending_mask)));

  if (state() & Thread_ipc_receiving_mask) // abnormal termination
    {
      // the IPC has not been finished.  could be timeout or cancel
      // XXX should only modify the error-code part of the status code
      if (state() & Thread_busy)
	{
	  // we've presumably been reset!
	  regs->msg_dope( L4_msgdope::REABORTED );
	}
      else if (state() & Thread_cancel)
	{
	  // we've presumably been reset!
	  regs->msg_dope( L4_msgdope::RECANCELED );
	}
      else
	regs->msg_dope( L4_msgdope::RETIMEOUT );

      state_del(Thread_ipc_receiving_mask);
    }

  return regs->msg_dope();		// sender puts return code here
}

/** Page fault handler.
    This handler suspends any ongoing IPC, then sets up page-fault IPC.
    Finally, the ongoing IPC's state (if any) is restored.
    @param pfa page-fault virtual address
    @param error_code page-fault error code.
 */
PRIVATE 
L4_msgdope Thread::handle_page_fault_pager(vm_offset_t pfa, unsigned error_code)
{
  // do not handle user space page faults from kernel mode if we're
  // already handling a request
#warning X86 Stuff in arch independent code
#if defined(CONFIG_IA32) || defined(CONFIG_UX)
  if (EXPECT_FALSE( !(error_code & PF_ERR_USERMODE) && thread_lock()->test()) )
    {
      panic("page fault in locked operation");
    }
#endif

  // set up a register block used as an IPC parameter block for the
  // page fault IPC

  Sys_ipc_frame r;
  
  r.set_msg_word(2, 0); // nop in V2
#warning X86 Stuff in arch independent code
#if defined(CONFIG_IA32) || defined(CONFIG_UX)
  r.set_msg_word(0, (pfa & ~3) | (error_code & (PF_ERR_PRESENT | PF_ERR_WRITE)));
  r.set_msg_word(1, (error_code & PF_ERR_USERMODE) ? regs()->pc() : (Mword)-1);
#endif
  r.snd_desc(0);			// snd descriptor = short msg
  // rcv descriptor = map msg
  r.rcv_desc(L4_rcv_desc::short_fpage(L4_fpage(0,0,L4_fpage::WHOLE_SPACE,0))); 

  L4_timeout timeout( L4_timeout::NEVER );
  
  // This might be a page fault in midst of a long-message IPC operation.
  // Save the current IPC state and restore it later.
  Sender *orig_partner;
  Sys_ipc_frame *orig_receive_regs;
  save_receiver_state (&orig_partner, &orig_receive_regs);

  Receiver *orig_send_partner = receiver();
  timeout_t *orig_timeout = _timeout;
  if (orig_timeout)
    orig_timeout->reset();
  unsigned orig_ipc_state = state() & Thread_ipc_mask;
  state_del(orig_ipc_state);
  if (orig_ipc_state)
    timeout = _pf_timeout;		// in long IPC -- use pagefault timeout

  prepare_receive(_pager, &r);

  L4_msgdope err = do_send(_pager, timeout, &r);
  L4_msgdope ret(0);

  if (EXPECT_FALSE( err.has_error()) )
    {
      if (Config::conservative)
	{
	  printf("page fault send error = 0x%x\n", err.raw());
	  kdb_ke("send to pager failed");
	}

      // skipped receive operation
      state_del(Thread_ipc_receiving_mask);

#warning X86 Stuff in arch independent code
#if defined(CONFIG_IA32) || defined(CONFIG_UX)
      // PF from kernel mode? -> always abort
      if (! (error_code & PF_ERR_USERMODE))
	{
	  // Translate a number of error codes so that they make sense
	  // in the long-IPC page-in case.
	  switch (err.error())
	    {
	    case L4_msgdope::SETIMEOUT:
	      ret.error(L4_msgdope::RESNDPFTO);  break;
	    case L4_msgdope::ENOT_EXISTENT:
	      ret.error(L4_msgdope::REABORTED);  break;
	    default:
	      ret.error(err.error());
	    }
	}
      else if (err.error() == L4_msgdope::ENOT_EXISTENT)
	ret = (state() & Thread_cancel)
	  ? L4_msgdope(0) // retry user insn after thread_ex_regs
	  : err;
#endif
      // else ret = 0 -- that's the default, and it means: retry
    }
  else
    {
      err = do_receive(_pager, timeout, &r);

      if (EXPECT_FALSE( err.has_error()) )
	{
	  if (Config::conservative)
	    {
	      printf("page fault rcv error = 0x%x\n", err.raw());
	      kdb_ke("rcv from pager failed");
	    }
#warning X86 Stuff in arch independent code
#if defined(CONFIG_IA32) || defined(CONFIG_UX)
	  // PF from kernel mode? -> always abort
	  if (! (error_code & PF_ERR_USERMODE))
	    {
	      // Translate a number of error codes so that they make sense
	      // in the long-IPC page-in case.
	      switch (err.error())
		{
		case L4_msgdope::RETIMEOUT:
		  ret = L4_msgdope(L4_msgdope::RERCVPFTO);  break;
		case L4_msgdope::ENOT_EXISTENT:
		  ret = L4_msgdope(L4_msgdope::REABORTED);  break;
		default:
		  ret = err;
		}
	    }
#endif
	  // XXX currently, we (Jochen-compatibly) ignore receive errors here
	  // and just retry the page fault.  However, maybe it would make
	  // sense to start exception handling (return false) when the pager
	  // returns something which is not a mapping?
	}
      // else ret = 0 -- that's the default, and it means: retry
    }

  // restore previous IPC state
  set_receiver (orig_send_partner);
  restore_receiver_state (orig_partner, orig_receive_regs);
  state_add(orig_ipc_state);
  if (orig_timeout)
    orig_timeout->set_again();

  LOG_PF_RES_USER;

  return ret;
}


extern "C" void here_a_big_cast_problem_occurs(void);
/** L4 IPC system call.
    This is the `normal'' version of the IPC system call.  It usually only
    gets called if ipc_short_cut() has failed.
    @param regs system-call arguments.
    @return value to be returned in %eax register.
 */     
IMPLEMENT
Mword Thread::sys_ipc(Syscall_frame *i_regs)
{
  Sys_ipc_frame *regs = static_cast<Sys_ipc_frame*>(i_regs);
  
  if((unsigned)i_regs!=(unsigned)regs)
    here_a_big_cast_problem_occurs();

  L4_msgdope ret(0);
  L4_timeout t = regs->timeout();

  // find the ipc partner thread belonging to the destination thread
  threadid_t dst_id(regs->snd_dest());
  Thread *partner = dst_id.lookup(); // XXX no clans & chiefs for now!

  // first, before starting a send, we must prepare a pending receive
  // so that we can atomically switch from send mode to receive mode
  
  bool have_receive_part = regs->rcv_desc().has_receive();
  bool have_send_part = regs->snd_desc().has_send();
  bool have_sender = false;
  Sender *sender = 0;
  Irq_alloc *interrupt = 0;

  if (have_receive_part)
    {
      if (regs->rcv_desc().open_wait())        // open wait ?
        {
          sender = 0;
          have_sender = true;
        }

      // not an open wait
      else if (! dst_id.is_nil()) // not nil thread, and not irq id?
        {
          if (! partner)
            return L4_msgdope(L4_msgdope::ENOT_EXISTENT).raw();

          sender = partner;
          have_sender = true;
        }

      // nil thread or interrupt thread
      else if (!regs->has_snd_dest())  // nil thread?
        {
          // only prepare IPC for timeout != 0
          if (t.rcv_exp() == 0 || t.rcv_man() != 0)
            {
              sender = partner;
              have_sender = true;
            }
        }
      // must be interrupt thread
      else 
        {  
          interrupt = Irq_alloc::lookup(regs->irq());

	  if (!interrupt)
	    return L4_msgdope(L4_msgdope::ENOT_EXISTENT).raw(); 


          if (_irq != interrupt)
            {
              // a receive with a timeout != 0 from a non-associated
              // interrupt is illegal
              if (t.rcv_exp() == 0 || t.rcv_man() != 0) // t/o != 0
                return L4_msgdope(L4_msgdope::ENOT_EXISTENT).raw();
            }

          if (_irq)
            {
	      // we always try to receive from the
      	      // assoc'd irq first, not from the spec. one
              sender = nonull_static_cast<irq_t*>(_irq);    
              have_sender = true;
            }
        }

      if (have_sender)
        prepare_receive(sender, regs);
    }

  // The send part.  Be careful here: We must not test the parameter
  // in the %eax register here because %eax may already have been   
  // overwritten by a sender if this IPC operation has a receive part
  // but no send part (because in this case, prepare_receive() already
  // activates the receive part).  That's why, we save that parameter 
  // early (have_send_part).

  if (have_send_part)           // do we do a send operation?
    {
      ret = do_send(partner, t, regs);
    }

  // send done, receive operation follows

  if (have_receive_part         // do we do a receive operation?
      && !ret.has_error()) // no send error
    {
      if (have_sender)
        {
          // do the receive operation
          ret = do_receive(sender, t, regs);

          // if this was a receive from an interrupt and the timeout was 0,
          // this is a re-associate operation
          if (ret.error() != L4_msgdope::RETIMEOUT
              || ! interrupt
              || (t.rcv_exp() == 0 || t.rcv_man() != 0)) // t/o != 0
            {
              return ret.raw();
            }
        }

      else if (! interrupt)     // rcv from nil; this also means t/o == 0
        {
          //
          // disassociate from irq
          //
          if (_irq)
            {
              _irq->free(this);
              _irq = 0;
            }

          return L4_msgdope(L4_msgdope::RETIMEOUT).raw();
        }

      //
      // associate with an interrupt
      //
      if (interrupt->alloc(this, Config::irq_ack_in_kernel))
        {
          // succeeded -- free previously allocated irq
          if (_irq)
            _irq->free(this);

          _irq = interrupt;

	  // success
          return L4_msgdope(L4_msgdope::RETIMEOUT).raw();
        }

      // failed -- could not associate new irq
      return L4_msgdope(L4_msgdope::ENOT_EXISTENT).raw();
    }

  // skipped receive operation
  state_del(Thread_ipc_receiving_mask);

  return ret.raw();
}

