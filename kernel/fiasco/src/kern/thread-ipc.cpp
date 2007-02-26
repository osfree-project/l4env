INTERFACE:

class Sys_ipc_frame;

EXTENSION class Thread
{
public:
  inline bool ipc_short_cut();

private:
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

};

/**
 * Save critical contents of UTCB during nested IPC.
 */
class Pf_msg_utcb_saver
{
public:
  Pf_msg_utcb_saver (const Utcb *u);
  void restore (Utcb *u);
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
#include "warn.h"

/** Receiver-ready callback.  
    Receivers make sure to call this function on waiting senders when
    they get ready to receive a message from that sender. Senders need
    to overwrite this interface.

    Class Thread's implementation wakes up the sender if it is still in
    sender-wait state.
 */
PUBLIC virtual
bool
Thread::ipc_receiver_ready(Receiver *recv)
{
  assert(receiver());
  assert(receiver() == recv);
  assert(receiver() == current());

  if(!(state() & Thread_ipc_in_progress))
    return false;

  if(!recv->sender_ok(this))
    return false;
  
  recv->ipc_init(this);

  state_add_dirty (Thread_ready | Thread_transfer_in_progress);

  ready_enqueue();

  // put receiver into sleep
  receiver()->state_del_dirty(Thread_ready);

  return true;
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


/** Page fault handler.
    This handler suspends any ongoing IPC, then sets up page-fault IPC.
    Finally, the ongoing IPC's state (if any) is restored.
    @param pfa page-fault virtual address
    @param error_code page-fault error code.
 */
PRIVATE 
Ipc_err
Thread::handle_page_fault_pager(Thread* pager, Address pfa, Mword error_code)
{
  // do not handle user space page faults from kernel mode if we're
  // already handling a request
  if (EXPECT_FALSE(!PF::is_usermode_error(error_code)
		   && thread_lock()->test()))
    {
      panic("page fault in locked operation");
    }

  if (EXPECT_FALSE((state() & Thread_alien)
                   && !(state() & (Thread_rcvlong_in_progress
                                   | Thread_ipc_in_progress))))
    return Ipc_err(Ipc_err::Reaborted);

  if (! revalidate (pager))
    {
      WARN ("Denying %x.%x to send fault message (pfa=" L4_PTR_FMT
	    ", code=" L4_PTR_FMT ") to %x.%x",
	    id().task(), id().lthread(), pfa, error_code, 
	    pager ? pager->id().task() : L4_uid (L4_uid::Invalid).task(),
	    pager ? pager->id().lthread() : L4_uid (L4_uid::Invalid).lthread());
      return Ipc_err(Ipc_err::Enot_existent);
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

  Proc::cli();
  Ipc_err err = do_ipc(true, pager,
                       true, pager,
                       timeout, &r);
  Proc::sti();
  
  Ipc_err ret(0);

  if (EXPECT_FALSE(err.has_error()))
    {
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
            case Ipc_err::Retimeout:
              ret.error(Ipc_err::Rercvpfto);  break;
              //            case Ipc_err::Fnot_existent:
              //              ret.error(Ipc_err::Reaborted);  break;
            default:
              ret.error(err.error());
            }
        }
      if (Config::conservative)
        {
          printf(" page fault %s error = 0x%lx\n",
                 err.snd_error() ? "send" : "rcv",
                 err.raw());
          if(err.snd_error())
            {
              kdb_ke("snd to pager failed");
            }
          else
            {
              kdb_ke("rcv from pager failed");
            }
        }

      if(err.snd_error()
         && (err.error() == Ipc_err::Enot_existent)
         && PF::is_usermode_error(error_code))
        {
          ret = (state() & Thread_cancel)
            ? Ipc_err (0) // retry user insn after thread_ex_regs
            : err;
        }
    }
  else // no error
    {
      // If the pager rejects the mapping, it replies -1 in msg.w0
      if (EXPECT_FALSE (r.msg_word (utcb(), 0) == (Mword) -1))
        ret = Ipc_err(Ipc_err::Reaborted);
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

  Thread *dst = lookup (dst_id, space());

  if (! dst)
    return false;

  // From now on, we need a consistent receiver state.  Lock it down.
  // We don't need the cpu_lock because we are still cli'd.

  register bool can_preempt = false;
  if (EXPECT_FALSE(    !dst->is_tcb_mapped()
			// don't take the shortcut if
		    || !dst->sender_ok(this)
		    || (   Config::deceit_bit_disables_switch
		        && regs->snd_desc().deceite()
			&& (can_preempt = can_preempt_current(dst->sched())))
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
  const L4_msgdope ret_dope(regs->snd_desc(), 
      Sys_ipc_frame::num_reg_words(), 0);

  // copy the short msg directly into the registers of the receiver
  dst_regs->msg_dope (ret_dope);	// status code: rcv'd 2 dw
  regs->copy_msg (dst_regs);

  copy_utcb_to(dst);

  // copy sender ID
  dst_regs->rcv_src( current_thread()->id() );

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
  bool have_receive_part = regs->rcv_desc().has_receive();
  bool have_send_part    = regs->snd_desc().has_snd();
  bool have_sender       = false;
  bool lookup_done       = false;
  Thread *partner        = 0;
  Sender *sender         = 0;
  Irq_alloc *interrupt 	 = 0;
  
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
  
  // do we do a closed receive operation?
  if (EXPECT_TRUE(have_receive_part))
    {
      if (regs->rcv_desc().open_wait())
	have_sender = true;
      else if (EXPECT_FALSE(id.is_nil()))
	{
          // only prepare IPC for timeout != 0
          if (!t.rcv_exp() || t.rcv_man() || regs->has_abs_rcv_timeout())
            {
              sender = nil_thread;
              have_sender = true;
            }
        }
      else if (EXPECT_FALSE(id.is_irq()))
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
	  partner = lookup (id, space());
	  lookup_done = true;

          if (EXPECT_FALSE (!partner || !partner->is_tcb_mapped() || partner->state() == Thread_invalid))
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
    }

  if (EXPECT_TRUE(have_send_part))
    {
      // irq quirks
      if (EXPECT_FALSE(id.is_irq()))
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
	  have_send_part = false;
	}
      else if (! lookup_done)
	partner = lookup (id, space());
    }
  
  if (EXPECT_FALSE(have_receive_part && !have_sender))
    {
      if (!interrupt) // is disassoc from IRQ
	{
	  assert(t.rcv_exp() && !t.rcv_man()); // timeout 0
	  //
	  // disassociate from all IRQs 
	  //
	  if (_irq)
	    {
	      Irq_alloc::free_all(this);
	      _irq = 0;
	    }
	  commit_ipc_failure (regs, Ipc_err::Retimeout);
	  return;
	}
      else
	{
	  //
	  // associate with an interrupt
	  //
	  if (associate_irq(interrupt))
	    {
	      // success
	      commit_ipc_failure (regs, Ipc_err::Retimeout);
	      return;
	    }
	}
      have_receive_part = false;
    }

  if (EXPECT_TRUE(have_send_part || have_receive_part))
    ret = do_ipc(have_send_part, partner,
	         have_receive_part, sender,
	         t, regs);

success:
  if (EXPECT_TRUE(!ret.has_error()))
    commit_ipc_success (regs, ret);
  else
    commit_ipc_failure (regs, ret);

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

  current_thread()->sys_ipc();

  assert (!(current()->state() &
           (Thread_delayed_deadline | Thread_delayed_ipc)));

  // If we return with a modified return address, we must not be interrupted
  //  Proc::cli();
}

extern "C"
void
ipc_short_cut_wrapper()
{
  register Thread *const ct = current_thread();

  if (EXPECT_TRUE (ct->ipc_short_cut()))
    return;

  ct->sys_ipc();

  // If we return with a modified return address, we must not be interrupted
  //  Proc::cli();
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
  L4_msgdope m = regs->msg_dope();
  m.error(err.raw());
  regs->msg_dope(m);
}

IMPLEMENT inline
void
Thread::commit_ipc_failure (Sys_ipc_frame *regs, Ipc_err err)
{
  state_del (Thread_delayed_deadline | Thread_delayed_ipc);
  L4_msgdope m = regs->msg_dope();
  m.strings(0);
  m.mwords(0);
  m.error(err.raw());
  regs->msg_dope(m);
}

IMPLEMENT inline
Ipc_err
Thread::get_ipc_err (Sys_ipc_frame *regs)
{
  return Ipc_err (regs->msg_dope().raw());
}

PRIVATE
Ipc_err Thread::do_send_wait (Thread *partner, L4_timeout t, Sys_ipc_frame *regs)
{

  // test if we have locked the partner 
  // in the best case the partner is unlocked
  assert(!partner->thread_lock()->test()
  	 || partner->thread_lock()->lock_owner() == this);

  state_add_dirty(Thread_polling| Thread_send_in_progress | Thread_ipc_in_progress);
  // register partner so that we can be dequeued by kill()
  set_receiver (partner);

  // lock here for the preemption points
  partner->thread_lock()->lock_dirty();
  Proc::preemption_point();

  if(state() & Thread_cancel)
    {
      partner->thread_lock()->clear_dirty();

      state_del (Thread_ipc_sending_mask
                 |Thread_transfer_in_progress
                 |Thread_ipc_in_progress);

      return Ipc_err(Ipc_err::Secanceled);
    }
  
  if(partner->sender_ok(this))
    return 0;
  
  sender_enqueue (partner->sender_list(), sched_context()->prio());

  Proc::preemption_point();

  IPC_timeout timeout;
  
  if (EXPECT_FALSE(t.snd_exp()))
    {
      Unsigned64 tval = snd_timeout (t, regs);
      // Zero timeout or timeout expired already -- give up
      if (tval == 0)
	  return abort_send(Ipc_err::Setimeout, partner);
      
      // enqueue timeout
      Proc::preemption_point();
      
      set_timeout (&timeout);
      timeout.set (tval);
    }

  do
    {

      partner->thread_lock()->clear_dirty();
      // possible PREEMPTION POINT

      Proc::preemption_point();

      if((state() & (Thread_ipc_in_progress | Thread_polling
		     | Thread_cancel | Thread_transfer_in_progress))
	 == (Thread_ipc_in_progress|Thread_polling))
	{
	  state_del_dirty(Thread_ready);
	  schedule();
	}

      Proc::preemption_point();

      // if we are here 5 cases are  possible
      // - timeout has hit
      // - ipc was canceled
      // - the receiver had awoken us
      // - the receiver has been killed
      // - or someone has simply awake us, then we go to sleep again

      partner->thread_lock()->lock_dirty();
      
      if (EXPECT_FALSE(state() & Thread_cancel))
	break;

      // ipc handshake bit is set
      if(state() & Thread_transfer_in_progress)
	break;

      assert(!partner->in_ipc(this));
      assert(!(state() & Thread_transfer_in_progress));

      // detect if we have timed out
      if (timeout.has_hit())
	{
	  // ipc timeout always clear this flag
	  assert(!(state() & Thread_ipc_in_progress));
	  return abort_send(Ipc_err::Setimeout, partner);
	}

      if(partner->state() == Thread_dead)
	return abort_send(Ipc_err::Enot_existent, partner);

      // huh? someone has simply awake us, goto sleep again

      // Make sure we're really still in IPC
      assert(state() &  Thread_ipc_in_progress);

      // adding again, so that ipc_receiver_ready find the correct state
      state_add_dirty(Thread_polling);

    } while(true);
  
  // because the handshake has already taken place
  // an triggered timeout can be ignored
  if (timeout.has_hit())
    state_add_dirty(Thread_ipc_in_progress);
 
  // reset is only an simple dequeing operation from an double
  // linked list, so we dont need an extra preemption point for this
  timeout.reset();
  set_timeout(0);
  
  Proc::preemption_point();
  sender_dequeue (partner->sender_list());
  Proc::preemption_point();

  state_del_dirty(Thread_polling);

  if (EXPECT_FALSE(state() & Thread_cancel))
    {
      // this test catch partner-cancel and kill
      if(!partner->in_ipc(this))
	return abort_send(Ipc_err::Secanceled, partner);
      
      // the partner still waits for us, cancel them too
      partner->state_change(~Thread_ipc_in_progress,
			    Thread_cancel | Thread_ready);
      partner->ready_enqueue();
      Proc::preemption_point();
      return abort_send(Ipc_err::Seaborted, partner);
    }

  // partner canceled?, handles kill too
  if(EXPECT_FALSE(!partner->in_ipc(this))) 
    return abort_send(Ipc_err::Seaborted, partner);

  if(partner->state() == Thread_dead)
    return abort_send(Ipc_err::Enot_existent, partner);

  assert(state() & Thread_ipc_in_progress);
  assert (!in_sender_list());

  return 0;
}

PRIVATE
Ipc_err Thread::abort_send(Ipc_err err, Receiver *partner)
{
  state_del_dirty (Thread_send_in_progress | Thread_polling | Thread_ipc_in_progress
		   |Thread_transfer_in_progress);

  assert(partner);

  Proc::preemption_point();
  if(in_sender_list())
    sender_dequeue (partner->sender_list());

  Proc::preemption_point();
  partner->thread_lock()->clear_dirty();

  Proc::preemption_point();
  if (_timeout && _timeout->is_set())
    _timeout->reset();

  set_timeout(0);
	  
  return err;
}

PRIVATE
void
Thread::handle_long_ipc()
{
  
  while ((state() & (Thread_receiving | Thread_rcvlong_in_progress 
		     | Thread_ipc_in_progress))
	 == (Thread_rcvlong_in_progress | Thread_ipc_in_progress)) // long IPC? 
   {

      // XXX handle cancel: should notify sender -- even if we're killed!
     Proc::sti();

      assert (pagein_addr() != (Address) -1);

      Ipc_err ipc_code = handle_page_fault_pager (_pager,
						  pagein_addr(),
						  pagein_error_code());

      clear_pagein_request();     
      state_add (Thread_busy_long);

      // resume IPC by waking up sender
      if (pagein_applicant()->ipc_continue(ipc_code).has_error())
	break;

      // XXX ipc_continue could already put us back to sleep...  This
      // would save us another context switch and running this code:

      Proc::cli();

      assert(state() & Thread_ready);

      if((state() 
	  & (Thread_ipc_in_progress | Thread_rcvlong_in_progress | Thread_busy_long))
	 != (Thread_ipc_in_progress | Thread_rcvlong_in_progress | Thread_busy_long))
	continue;
      
      state_del_dirty(Thread_ready | Thread_busy_long);
      
      schedule();
    }

  assert(! (state() & (Thread_ipc_in_progress | Thread_ipc_sending_mask)));
  
}


PRIVATE inline
void Thread::goto_sleep(L4_timeout t, Sys_ipc_frame *regs)
{
  if(EXPECT_FALSE
     ((state() & (Thread_receiving | Thread_ipc_in_progress | Thread_cancel))
      != (Thread_receiving | Thread_ipc_in_progress)))
    return;

  IPC_timeout timeout;

  if(EXPECT_FALSE(t.rcv_exp() && t.rcv_man() && !_timeout))
    {

      state_del_dirty(Thread_ready);	  

      Unsigned64 tval = rcv_timeout (t, regs);
      
      if (EXPECT_TRUE((tval != 0)))
	{
	  set_timeout (&timeout);
	  timeout.set (tval);
	}
      else // timeout already hit
	state_change_dirty(~Thread_ipc_in_progress, Thread_ready);

    }
  else
    {
      
      if(EXPECT_TRUE(!t.rcv_exp()))
	state_del_dirty(Thread_ready);
      else
	state_change_dirty(~Thread_ipc_in_progress, Thread_ready);
    }

  Proc::preemption_point();
  schedule();

  if (EXPECT_FALSE((long)_timeout)) {
    timeout.reset();
    set_timeout(0);
  }

  assert(state() & Thread_ready);  
}





PRIVATE inline NEEDS["logdefs.h"]
Ipc_err Thread::try_handshake_receiver(Thread *partner, L4_timeout t, Sys_ipc_frame *regs)
{

  // By touching the partner tcb we can raise an pagefault.
  // The Pf handler might be enable the interrupts, if no mapping in
  // the master kernel directory exists.
  // Because the partner can created in between,
  // and partner->state() == Thread_invalid is insufficient
  // we need a cancel test.
  //

  if (EXPECT_FALSE (partner == 0 || partner == nil_thread
	|| !partner->is_tcb_mapped() // msut ensure a mapped TCB (cli'd)
	|| partner->state() == Thread_invalid))
    return Ipc_err (Ipc_err::Enot_existent);

  assert(cpu_lock.test());
      
  if(EXPECT_FALSE(partner->thread_lock()->test())) // lock is not free
    {
      Proc::preemption_point();
      partner->thread_lock()->lock_dirty();
    }

  if(EXPECT_FALSE(state() & Thread_cancel))
    {
      // clear_dirty() handle the not locked case too
      partner->thread_lock()->clear_dirty();
      return Ipc_err(Ipc_err::Secanceled);
    }

  Ipc_err err(0);
  
  if (EXPECT_FALSE (!partner->sender_ok
		    (nonull_static_cast<Sender*>(current_thread()))))
  
    {
      err = do_send_wait(partner, t, regs);

      // if an error occured, we should not hold the lock anymore
      assert(!err.has_error() || partner->thread_lock()->lock_owner() != this);
    }
  return err;
}


PRIVATE inline
void
Thread::wake_receiver (Thread *receiver)
{
  // If neither IPC partner is delayed, just update the receiver's state
  if (EXPECT_TRUE (!((state() | receiver->state()) & Thread_delayed_ipc)))
    {
      receiver->state_change_dirty(~(Thread_ipc_receiving_mask
				    | Thread_ipc_in_progress),
				  Thread_ready);
      return;
    }

  // Critical section if either IPC partner is delayed until its next period
  assert(cpu_lock.test());
  
  // Sender has no receive phase and deadline timeout already hit
  if ( (state() & (Thread_receiving |
		   Thread_delayed_deadline | Thread_delayed_ipc)) ==
      Thread_delayed_ipc)
    {
      state_change_dirty (~Thread_delayed_ipc, 0);
      switch_sched (sched_context()->next());
      _deadline_timeout.set (Timer::system_clock() + period());
    }

  // Receiver's deadline timeout already hit
  if ( (receiver->state() & (Thread_delayed_deadline |
                             Thread_delayed_ipc) ==   
	                     Thread_delayed_ipc))      
    {
      receiver->state_change_dirty (~Thread_delayed_ipc, 0);   
      receiver->switch_sched (receiver->sched_context()->next());
      receiver->_deadline_timeout.set (Timer::system_clock() +
                                       receiver->period());
    }

  receiver->state_change_dirty (~(Thread_ipc_mask|Thread_delayed_ipc),
				Thread_ready);
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
Ipc_err Thread::do_ipc (bool have_send, Thread *partner,
			bool have_receive, Sender *sender,
			L4_timeout t, Sys_ipc_frame *regs)
{

  bool dont_switch = false;
  
  if(have_send)
    {

      Ipc_err err = try_handshake_receiver(partner, t, regs);
     
      if(EXPECT_FALSE(err.has_error()))
	return err;

      assert(!(state() & Thread_polling));

      partner->ipc_init(this);      

      // mmh, we can reset the receivers timeout
      // ping pong with timeouts will profit from it, because
      // it will require much less sorting overhead
      // if we dont reset the timeout, the possibility is very high
      // that the receiver timeout is in the timeout queue
      if (partner->_timeout && partner->_timeout->is_set())
	{
	  partner->_timeout->reset();
	  partner->set_timeout(0);
	}

      Ipc_err ret = transfer_msg(partner, regs);

      if (Config::deceit_bit_disables_switch &&
          regs->snd_desc().deceite())
	dont_switch = true;

      // partner locked, i.e. lazy locking (not locked) or we own the lock
      assert(!partner->thread_lock()->test()
	     || partner->thread_lock()->lock_owner() == this);


      if(EXPECT_FALSE(ret.has_error() || !have_receive))
	{
	  // make the ipc partner ready if still engaged in ipc with us
	  if(partner->in_ipc(this))
	    {
	      wake_receiver(partner);
	      if(!dont_switch)
		partner->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_LOCKEE);
	    }

	  partner->thread_lock()->clear_dirty();

	  state_del (Thread_ipc_sending_mask
		     |Thread_transfer_in_progress
		     |Thread_ipc_in_progress);
	  
	  return ret;
	}

      partner->thread_lock()->clear_dirty_dont_switch();
      // possible preemption point

      if(EXPECT_TRUE(!partner->in_ipc(this)))
        {
	  state_del (Thread_ipc_sending_mask
		     |Thread_transfer_in_progress
		     |Thread_ipc_in_progress);
	
	  return Ipc_err::Secanceled;
        }

      wake_receiver(partner);

    }
  else
      regs->msg_dope(0);

  assert(have_receive);
  if (state() & Thread_cancel)
    {
      state_del(Thread_ipc_mask);
      return Ipc_err::Recanceled;
    }
  prepare_receive_dirty (sender, regs);

  while (EXPECT_TRUE
         ((state() & (Thread_receiving | Thread_ipc_in_progress | Thread_cancel))
          == (Thread_receiving | Thread_ipc_in_progress)) ) 
    {
    
      Sender *next = 0;

      if(EXPECT_FALSE((long)*sender_list()))
	{

	  if(sender) // closed wait
	    {
	      
	      if(sender->in_sender_list()
		 && this == sender->receiver()
		 && sender->ipc_receiver_ready(this))
		next = sender;
	    }
	  else // open wait
	    {
	      
	      next = *sender_list();
	      if(!next->ipc_receiver_ready(this)) 
		{
		  next->sender_dequeue_head(sender_list());
		  Proc::preemption_point();
		  continue;
		}
	    }
	}
      
      assert(cpu_lock.test());
    
      if(EXPECT_FALSE((long) next))
	{
	  
	  assert(!(state() & Thread_ipc_in_progress)
		 || !(state() & Thread_ready));
	  
	  // maybe switch_exec should return an bool to avoid testing the 
	  // state twice
	  if(have_send) {
	    
	    assert(partner);
	    assert(partner->sched());
	  }

	  if(EXPECT_TRUE(have_send && !dont_switch
			 && (partner->state() & Thread_ready)
			 && (next->sender_prio() <= partner->sched()->prio())))
	      switch_exec_locked(partner,  Context::Not_Helping);
	  else
	    {
	      Proc::preemption_point();
              assert(cpu_lock.test());
	      if(have_send && (partner->state() & Thread_ready))
		partner->ready_enqueue();
	      schedule();
	    }
	  
	  assert(state() & Thread_ready);
	}

      else if(EXPECT_TRUE(have_send && (partner->state() & Thread_ready)))
        {
          if(!dont_switch)
    	    switch_exec_locked(partner,  Context::Not_Helping);
          else
            partner->ready_enqueue();
        }
      else
	goto_sleep(t, regs);
      
      have_send = false;
    }
  
  assert(!(state() & Thread_ipc_sending_mask));  

  if (EXPECT_FALSE((long)_timeout)) {
    _timeout->reset();
    set_timeout(0);
  }

  // if the receive operation was canceled/finished before we 
  // switched to the old receiver, finish the send
  if(have_send && (partner->state() & Thread_ready))
    {
      if(!dont_switch)
        switch_exec_locked(partner,  Context::Not_Helping);
      else
        partner->ready_enqueue();
    }

  // fast out if ipc is already finished
  if(EXPECT_TRUE((state() & 
                  (~(Thread_fpu_owner|Thread_cancel))) == Thread_ready))
    return  get_ipc_err (regs);

  handle_long_ipc();

  // abnormal termination?
  if (EXPECT_FALSE (state() & Thread_ipc_receiving_mask))
    {
      // the IPC has not been finished.  could be timeout or cancel
      // XXX should only modify the error-code part of the status code

      if (state() & Thread_busy)        // we've presumably been reset!
        commit_ipc_success (regs, Ipc_err::Reaborted);

      //else 
      if (state() & Thread_cancel)      // we've presumably been reset!
        {
#if 0
          if (state() & Thread_transfer_in_progress)
	    {
	  LOG_MSG(this,"REAB2");
            commit_ipc_success (regs, Ipc_err::Reaborted);
	    }
          else
	    {
	  LOG_MSG(this,"RECA1");
#endif
            commit_ipc_success (regs, Ipc_err::Recanceled);
#if 0
	    }
#endif
        }

      else
        commit_ipc_success (regs, Ipc_err::Retimeout);
    }

  state_del (Thread_ipc_mask);

  return get_ipc_err (regs);            // sender puts return code here
}


PRIVATE inline NEEDS ["map_util.h", Thread::copy_utcb_to, 
		      Thread::unlock_receiver]
Ipc_err Thread::transfer_msg (Thread *receiver,
                              Sys_ipc_frame *sender_regs)
{
  
  if (!Config::deceit_bit_disables_switch && sender_regs->snd_desc().deceite())
    panic ("deceiving ipc");    // XXX unimplemented

  Sys_ipc_frame* dst_regs = receiver->rcv_regs();
  const L4_msgdope ret_dope(sender_regs->snd_desc(), 
      Sys_ipc_frame::num_reg_words(), 0);


  dst_regs->msg_dope (ret_dope);        // status code: rcv'd 2 dw

  // copy message register contents
  sender_regs->copy_msg (dst_regs);

  copy_utcb_to (receiver);

  // copy sender ID
  dst_regs->rcv_src (id());

  // fast out if only register msg
  if(EXPECT_TRUE(sender_regs->snd_desc().is_register_ipc()))
      return 0;

  // because we do a longer ipc with preemption points, we set a corrent ipc state
  state_add_dirty(Thread_send_in_progress |
                  Thread_ipc_in_progress| Thread_transfer_in_progress);

  Mword ret = 0;                                // status code: IPC successful
  // we need the lock here definitly
  receiver->thread_lock()->lock_dirty();

  // short flexpage mapping to register/rmap receiver
  if(EXPECT_TRUE (sender_regs->snd_desc().msg() == 0
                  && (dst_regs->rcv_desc().is_register_ipc()
                      || dst_regs->rcv_desc().rmap())))
    {
      assert(sender_regs->snd_desc().map());

      if (EXPECT_FALSE (! dst_regs->rcv_desc().rmap()) )
        // rcvr not expecting an fpage?
        {
          dst_regs->msg_dope_set_error(Ipc_err::Remsgcut);
          ret = Ipc_err::Semsgcut;
        }
      else
        {
          Proc::sti();

          dst_regs->msg_dope_combine
            (fpage_map(space(),
                       L4_fpage(sender_regs->msg_word(1)),
                       receiver->space(), dst_regs->rcv_desc().fpage(),
                       sender_regs->msg_word(0),
                       L4_fpage(sender_regs->msg_word(1)).grant()));

          Proc::cli();

          if (dst_regs->msg_dope().rcv_map_failed())
            ret = Ipc_err::Semapfailed;
        }
      return ret;
    }

  Proc::preemption_point();

  // else long ipc
  prepare_long_ipc(receiver, sender_regs);

  Ipc_err error_ret;

  assert(state() & Thread_send_in_progress);
  receiver->thread_lock()->clear_dirty();

  CNT_IPC_LONG;
  Proc::sti();
  error_ret = do_send_long (receiver, sender_regs);
  Proc::cli();

  assert(receiver->thread_lock()->lock_owner() == this);

  assert(error_ret.has_error() || state()
         & (Thread_send_in_progress | Thread_transfer_in_progress));

  return error_ret;
}


PRIVATE
void Thread::prepare_long_ipc (Thread *receiver,
                                Sys_ipc_frame *sender_regs)
{

  state_add_dirty( Thread_ipc_in_progress | Thread_send_in_progress);

  assert(state() &  Thread_ipc_in_progress);


  // register partner so that we can be dequeued by kill()
  set_receiver (receiver);

  // prepare long IPC

  receiver->clear_pagein_request();

  Sys_ipc_frame* dst_regs = receiver->rcv_regs();

  // XXX check for cancel -- sender?  receiver?
  // Receiver is locked, no cancel, Sender cancel is done in do_send_long

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
      receiver->_pf_timeout = L4_timeout(1, t.snd_pfault(),
                                             1, t.snd_pfault(), 0, 0);
          // XXX should normalize timeout spec, but do_send/do_receive
          // can cope with non-normalized numbers.

  t = dst_regs->timeout();
  if (t.rcv_pfault() == 15) // rcv pfault timeout == 0 ms?
    _pf_timeout = L4_timeout(0,1,0,1,0,0);
  else
    _pf_timeout = L4_timeout(1, t.rcv_pfault(), 1, t.rcv_pfault(), 0, 0);
  // XXX should normalize timeout spec, but do_send/do_receive
  // can cope with non-normalized numbers.

  // set up return code in case we're aborted
  dst_regs->msg_dope_set_error(Ipc_err::Reaborted);

  // switch receiver's state, and put it to sleep.
  // overwrite ipc_in_progress flag a timeout may have deleted --
  // see above
  receiver->state_change(~(Thread_receiving | Thread_busy | Thread_ready),
                         Thread_rcvlong_in_progress | Thread_transfer_in_progress
                         | Thread_ipc_in_progress);

}

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

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION[!utcb]:

IMPLEMENT inline
Pf_msg_utcb_saver::Pf_msg_utcb_saver (const Utcb *)
{}

IMPLEMENT inline
void
Pf_msg_utcb_saver::restore (Utcb *)
{}

IMPLEMENTATION[utcb]:

EXTENSION class Pf_msg_utcb_saver
{
private:
  Mword snd_size, rcv_size;
};

IMPLEMENT inline
Pf_msg_utcb_saver::Pf_msg_utcb_saver (const Utcb *u)
  : snd_size (u->snd_size),
    rcv_size (u->rcv_size)
{}

IMPLEMENT inline
void
Pf_msg_utcb_saver::restore (Utcb *u)
{
  u->snd_size = snd_size;
  u->rcv_size = rcv_size;  
}
