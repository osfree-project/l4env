INTERFACE:

EXTENSION class Thread
{
private:
  /**
   * Compute remote pagefault address from the local pagefault address
   * in the local Long-IPC window.
   * @param pfa Local pagefault address in IPC window
   * @return Remote pagefault address in partner's address space
   */
  Address remote_fault_addr (Address pfa);

  /**
   * Try to upgrade the local IPC window with a mapping from the
   * partner's address space.
   * @param pfa Local pagefault address in IPC window
   * @param remote_pfa Remote pagefault address in partner's address space
   * @return true if the partner had the page mapped and we could copy
   *         false if the partner needs to map the page himself first
   */
  Mword update_ipc_window (Address pfa, Address remote_pfa, Mword error_code);
};

IMPLEMENTATION:

// The ``long IPC'' mechanism.

// IDEAS for enhancing this implementation:

// Currently, we flush the address-space window used for copying
// memory during IPC every time we switch threads.  This is costly
// because it involves a TLB flush.  We could maybe avoid a lot of
// flushes by doing them lazily -- that is, we only do them when the
// window is used by someone else. -- I haven't really thought about
// whether this really makes sense with a fully interruptible IPC path
// that is possibly executed by more than one thread at the same time.

// We should use only the first one of the two address-space windows
// if an indirect string actually refers to a buffer that is already
// mapped in the first window.

// The address-space-window setup function should actually fill the
// corresponding page-directory entries safely (using cli) instead of
// leaving that work to the page-fault handler.  This is safe because
// those page-directory entries are flushed during the next thread
// switch.

// XXX: I'm not sure whether we return the correct error codes for
// page fault timeouts in any of the sender's and receiver's address
// spaces.

#include "l4_types.h"

#include "config.h"
#include "cpu_lock.h"
#include "map_util.h"
#include "space.h"
#include "std_macros.h"

/**
 * Resets the ipc window adresses.
 * Should be called at the end of every long IPC to prevent unnecessary 
 * pagefaults.  If we dont do this, the test 
 * if (EXPECT_TRUE (_vm_window0 == address)) in 
 * setup_ipc_window(unsigned win, Address address) is mostly successful.
 * If the test is positive, we dont copy the PDE slots.
 * Because pingpong uses the same ipc buffers again and again,
 * then this so called "optimization" results in pagefaults for EVERY long ipc.
 * Reason: pingpong sets for the first time the _vm_window*, do the long ipc
 * and switch to the receiver. Then receiver switches back to the sender
 * using another ipc. Remember: every thread switch flushes the ipc window pde
 * slots. But, because we have same ipc buffer addresses as before,
 * the test is always positive, and so we dont update the now empty pde slots.
 * The result is, for EVERY long ipc, after the first long ipc, 
 * we get an pagefault.
 */              
PROTECTED inline
void             
Thread::reset_ipc_window()
{                
  _vm_window1 = _vm_window0 = (Address) -1;
}



/** Handle a page fault that occurred in one of the ``IPC-window''
    virtual-address-space regions.
    @param pfa virtual address of page fault
    @param error_code page fault error code
    @return IPC error code if an error occurred during page-fault handling,
            0 if page fault was handled successfully
    @pre current_thread() == this
 */
PRIVATE
Ipc_err
Thread::handle_ipc_page_fault (Address pfa, Mword error_code)
{
  Address remote_pfa = remote_fault_addr (pfa);
  Ipc_err err;

  for (;;)
    {
      {
        Lock_guard <Cpu_lock> guard (&cpu_lock);

        // IPC has been aborted
        if (!receiver()->in_long_ipc (this))
	  return Ipc_err(Ipc_err::Seaborted);

        // If the receiver has mapped the page in his address space already,
        // we can just copy the mapping into our IPC window.
        if (update_ipc_window (pfa, remote_pfa, error_code))
	  return 0;		// Success
      }

      // The receiver didn't have it mapped, so it needs to page in data
      state_add(Thread_polling_long);

      if ((err = ipc_pagein_request(receiver(), remote_pfa,
                                    error_code)).has_error())
	return err;

      // XXX ipc_pagein_request could already put us to sleep...  This
      // would save us another context switch and running this code:

      while (state_change_safely(~(Thread_polling_long|Thread_ipc_in_progress
				   |Thread_ready),
				 Thread_polling_long|Thread_ipc_in_progress))
	{
	  schedule();
	}

      state_del(Thread_polling_long);

      if (! (state() & Thread_ipc_in_progress))
	{
	  assert (state() & Thread_cancel);
	  return Ipc_err::Seaborted;
	}

      // Find receiver's error code
      if (_pagein_status_code)
	return _pagein_status_code;
    }
}

IMPLEMENTATION:

/** Send a flexpage.
    Assumes we are in the middle of long IPC.
    @param receiver the receiver of our message
    @param from_fpage sender's flexpage
    @param to_fpage   receiver's flexpage
    @param offset     sender-specified offset into receiver's flexpage
    @param finish     if true, finish IPC operation after sending flexpage;
                      if false, leave IPC partners in IPC-in-progress state
    @return IPC error code if an error occurred.  0 if successful
 */
PRIVATE
Ipc_err Thread::ipc_snd_fpage(Thread* receiver,
			      L4_fpage from_fpage, L4_fpage to_fpage,
			      Address offset, bool finish)
{
  receiver->thread_lock()->lock();

  if (EXPECT_FALSE (((receiver->state() & (Thread_rcvlong_in_progress|
					   Thread_ipc_in_progress))
				       != (Thread_rcvlong_in_progress|
					   Thread_ipc_in_progress))
	|| receiver->partner() != this) )
    return Ipc_err(Ipc_err::Seaborted);

  Ipc_err ret = fpage_map(space(), from_fpage,
			  receiver->space(), to_fpage, offset);
  if(!finish)
    {
      receiver->thread_lock()->clear();
      return ret;
    }

  reset_ipc_window();
      
  // if we must finish the IPC operation, set the receiver's return code
  // and return the sender's return code (instead of the receiver's)
  receiver->rcv_regs()->msg_dope (ret);
  ret = ret.has_error() ? Ipc_err(ret.error() | Ipc_err::Send_error)
    : Ipc_err(0);
  
  return ret;
}

IMPLEMENTATION:


/** Finish an IPC.
    @param receiver the receiver of our message
    @param new_state sender's IPC error conditions accumulated so far
    @return IPC error code that should be returned to sender
 */
PRIVATE
Ipc_err
Thread::ipc_finish (Thread *receiver, Ipc_err new_state)
{
  receiver->thread_lock()->lock();

  reset_ipc_window();

  if (EXPECT_FALSE (((receiver->state() & (Thread_rcvlong_in_progress|
					   Thread_ipc_in_progress))
				       != (Thread_rcvlong_in_progress|
					   Thread_ipc_in_progress))
	|| receiver->partner() != this) )
    return Ipc_err(Ipc_err::Seaborted);

  receiver->commit_ipc_success (receiver->rcv_regs(), new_state);

  return new_state.has_error()
         ? Ipc_err (new_state.error() | Ipc_err::Send_error)
         : Ipc_err (0);
}

/** Wake up sender.
    A receiver wants to wake up a sender after a page-in request.
    @param ipc_code IPC status code that should be signalled to sender.
                    This status code flags error conditions that occurred
		    during a page-in in the receiver.
    @return 0 if successful, Reaborted if sender was not in IPC anymore.
 */
PUBLIC
Ipc_err Thread::ipc_continue(Ipc_err ipc_code)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  // Is the IPC sender still talking to us?
  if (((state() & (Thread_polling_long|Thread_ipc_in_progress
		   |Thread_send_in_progress))
       != (Thread_polling_long|Thread_ipc_in_progress|Thread_send_in_progress))
      || receiver() !=  thread_lock()->lock_owner())
    return Ipc_err(Ipc_err::Reaborted);
  // Tell it the error code
  _pagein_status_code = ipc_code.raw();

  // Resume sender.
  state_change(~Thread_polling_long, Thread_ready);

  thread_lock()->set_switch_hint(SWITCH_ACTIVATE_LOCKEE);

  return Ipc_err(0);
}

/** Send a page-in request to a receiver.
    Requests that the receiver pages in some pages needed by the sender
    to copy data into the receiver's address space.
    @param receiver the receiver of our message
    @param address page fault's virtual address in receiver's address space
    @param error_code page fault's error code
    @return 0 if request could be sent.  IPC error code if receiver
            was in incorrect state.
 */
PRIVATE
Ipc_err Thread::ipc_pagein_request(Receiver* receiver,
				   Address address, Mword error_code)
{
  Lock_guard<Thread_lock> guard (receiver->thread_lock());

  if (! receiver->in_long_ipc(this))
    return Ipc_err(Ipc_err::Seaborted);

  receiver->set_pagein_request (address, error_code, this);
  receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_LOCKEE);

  return Ipc_err(0);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!arm]:

PRIVATE inline 
bool
Thread::invalid_ipc_buffer(void const *a)
{
  return Mem_layout::in_kernel(((Address)a & Config::SUPERPAGE_MASK)
			       + Config::SUPERPAGE_SIZE - 1);
}

// --------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

/**
 * Carry out long IPC.
 *
 * This method assumes that IPC handshake has finished successfully
 * (do_ipc()).  Its arguments must correspond to those
 * supplied to do_ipc().
 * The method copies data from the sender's address space into the receiver's
 * using IPC address-space windows.  During copying, page faults in
 * both the sender's and the receiver's buffer can occur, leading to
 * page-fault--IPC handling.
 * When using small address spaces great care must be taken when copying
 * out of the users address space, as kernel and user may use different
 * segments.
 *
 * @param partner the receiver of our message
 * @param regs sender's IPC registers
 * @return sender's IPC error code
 */
PRIVATE 
Ipc_err
Thread::do_send_long (Thread *partner, Sys_ipc_frame *i_regs)
{
  Long_msg *regs = (Long_msg*)i_regs;
  // just copy the message here.  don't care about pages not being
  // mapped in -- this is completely handled in the page fault handler.

  // XXX bound check missing!

  // 2 dwords in registers
  L4_msgdope result_dope (i_regs->snd_desc(),
      Sys_ipc_frame::num_reg_words(), 0);
  Ipc_err result_err (0);

  barrier();

  // set up exception handling
  jmp_buf pf_recovery;  
  unsigned error;

  if (EXPECT_TRUE ((error = setjmp(pf_recovery)) == 0) )
    {
      // set up pointers to message buffers
      struct message_header {
        L4_fpage     fp;
        L4_msgdope   size_dope;
        L4_msgdope   snd_dope;
        Mword        words[1];
      } *snd_descr, *rcv_descr;

      // save the address of the snd_header for later use
      // be careful, although it is a pointer, there is no valid
      // data behind it (see small address spaces)
     
      // Check if the address of the direct message part is within the
      // kernel memory
      snd_descr = (message_header*)regs->snd_desc().msg();
      if (EXPECT_FALSE(invalid_ipc_buffer(snd_descr)))
	{
	  _recover_jmpbuf = 0;
          WARNX(Info, "long-IPC: invalid IPC buffer at "L4_PTR_FMT,
                      (Address)snd_descr);
          return ipc_finish (partner, Ipc_err::Remsgcut);
	}

      rcv_descr = 0;   // default is receiver expects short message

      // Check for long IPC reciever and if the receive buffer is not
      // in kernel memory
      if (EXPECT_TRUE (_target_desc.msg() != 0 && !_target_desc.rmap() &&
		       !partner->invalid_ipc_buffer(_target_desc.msg())))
        {
          // the receiver's message buffer is mapped into VM window 1.
          setup_ipc_window(0, ((Address)(_target_desc.msg())) 
						     & Config::SUPERPAGE_MASK);

        // IPC has been aborted
	  if (!partner->in_long_ipc (this))
	    return ipc_finish (partner, Ipc_err::Reaborted);

          rcv_descr = reinterpret_cast<message_header*>
            (Kmem::ipc_window(0)
	     + ((Address)_target_desc.msg() & ~Config::SUPERPAGE_MASK));
        }

      // Treat dwords as fpage mappings
      bool snd_fpages = regs->snd_desc().map();

      // make sure this is not a register-only send operation -- such
      // operations should have been handled by do_ipc().
      // long IPC, or sending (short or long) fpage (or both)
      assert (snd_descr || snd_fpages);

      // set up return value in case we're aborted; we must have read
      // regs->snd_desc() is inavlid from now
      regs->msg_dope( Ipc_err::Seaborted );
  
      // Make it possible to recover from bad user-level page faults
      // and IPC aborts
      _recover_jmpbuf = &pf_recovery;

      // read the fpage option from the receiver's message buffer
      L4_fpage rcv_fpage (rcv_descr ? rcv_descr->fp
				    : _target_desc.fpage());

      // check if we are not expected to send flexpages
      if (EXPECT_FALSE (snd_fpages && !rcv_fpage.is_valid()))
        {
          _recover_jmpbuf = 0;
          WARNX(Info, "long-IPC: receiver does not expect fpages");
          return ipc_finish (partner, Ipc_err::Remsgcut);
        }

      Ipc_err ret;

      // send flexpages?
      if (snd_fpages)
	{
	  // transfer the first flexpage -- the one transferred in the
	  // message registers.

	  bool snd_short_fpage = !snd_descr;
	  L4_fpage snd_fpage (regs->msg_word (1));

	  // fpage valid?
	  if (EXPECT_FALSE (!snd_fpage.is_valid()))
	    {
	      // This is not a valid flexpage, so do not try to send it.
	      // Also, don't try to transfer any more flexpages.
	      // XXX this test could also be done in short ipc.

	      // short-fpage send?
	      if (snd_short_fpage)
		{
		  _recover_jmpbuf = 0;
		  return ipc_finish (partner, 0);
		}

	      // don't try to send more flexpages
	      snd_fpages = false;
	    }
	  else
	    {
	      ret = ipc_snd_fpage (partner, snd_fpage, rcv_fpage,
				   regs->msg_word (0), snd_short_fpage);

	      // short-fpage send?
	      if (snd_short_fpage)
		{
		  _recover_jmpbuf = 0;
		  return ret;
		}

	      result_err.combine(ret);
	      barrier();

	      if (result_err.has_error())
		{
		  _recover_jmpbuf = 0;
		  partner->rcv_regs()->msg_dope (result_dope);
		  return ipc_finish (partner, result_err);
		}
	    }
	}

      // OK, so we have to send a long message buffer.
      assert (snd_descr);

      // We start by transferring dwords and do flexpage mappings on our way.
      L4_msgdope snd_snddope (mem_space()->peek_user (&snd_descr->snd_dope));
      int snd_words = snd_snddope.mwords();

      // We must stop now if we don't have a receive buffer.  Check
      // for cut message.
      if (!rcv_descr)
	{
	  _recover_jmpbuf = 0;
	  if (snd_words > (int)Sys_ipc_frame::num_reg_words() ||
	      snd_snddope.strings() != 0)
            {
              WARNX(Info, "long-IPC: receiver expects short message");
              result_err.error (Ipc_err::Remsgcut);
            }

	  partner->rcv_regs()->msg_dope (result_dope);
	  return ipc_finish (partner, result_err);
        }

      // OK, so we have a long receive buffer.

      int rcv_words = rcv_descr->size_dope.mwords();
      unsigned min = (snd_words > rcv_words) ? rcv_words : snd_words;

      if (min > Sys_ipc_frame::num_reg_words())
	{
	  // XXX if the memcpy() fails somewhere in the middle because
	  // of a page fault, we don't report the memory words that
	  // have already been copied.
	  mem_space()->copy_from_user (
	      rcv_descr->words + Sys_ipc_frame::num_reg_words(),
	      snd_descr->words + Sys_ipc_frame::num_reg_words(),
	      min - Sys_ipc_frame::num_reg_words());
	  result_dope.mwords (min);
	  barrier();
	}

      if (snd_fpages)
	{
	  // Start with message word 2
	  unsigned n = 2, s = snd_words, r = rcv_words;

	  // We have already successfully transferred the
	  // register-dwords flexpage. Just care about flexpages in
	  // the memory buffer here.

	  while (s >= (Sys_ipc_frame::num_reg_words() + 2) &&
		 r >= (Sys_ipc_frame::num_reg_words() + 2))
	    {
	      L4_fpage snd_fpage (regs->msg_word ((Mword *) snd_descr, n+1));

	      // if not a valid flexpage, break
	      if (!snd_fpage.is_valid())
		{
		  snd_fpages = false;
		  break;
		}

	      // otherwise, map
	      ret = ipc_snd_fpage (partner, snd_fpage, rcv_fpage,
				   regs->msg_word ((Mword *) snd_descr, n),
				   false);

	      if (EXPECT_FALSE (ret.has_error()))
		break;

	      result_err.combine (ret);
	      barrier();

	      n += 2;
	      s -= 2;
	      r -= 2;
	    }
	}

      if (ret.has_error())
	{
	  // a mapping couldn't be established
	  _recover_jmpbuf = 0;
	  partner->rcv_regs()->msg_dope (result_dope);
	  return ipc_finish(partner, ret);
	}
      else if (snd_words > rcv_words)
	{
	  // the message has been cut
	  _recover_jmpbuf = 0;
	  result_err.combine (Ipc_err (Ipc_err::Remsgcut));
	  partner->rcv_regs()->msg_dope (result_dope);
          WARNX(Info, "long-IPC: snd_words (%d) > rcv_words (%d)",
                      snd_words, rcv_words);
	  return ipc_finish(partner, result_err);
	}

      // All right -- the direct string in the message buffer has been
      // transferred.  Now tackle the indirect strings.

      L4_msgdope snd_sizedope (mem_space()->peek_user (&snd_descr->size_dope));

      L4_str_dope *snd_strdope = reinterpret_cast<L4_str_dope *>
            (snd_descr->words + snd_sizedope.mwords());

      L4_str_dope *rcv_strdope = reinterpret_cast<L4_str_dope *>
            (rcv_descr->words + rcv_words);

      int snd_strings = snd_snddope.strings();
      int rcv_strings = rcv_descr->size_dope.strings();

      while (snd_strings && rcv_strings)
	{
	  Unsigned8 *from      = mem_space()->peek_user(&snd_strdope->snd_str);
	  size_t     from_size = mem_space()->peek_user(&snd_strdope->snd_size);
	  Unsigned8 *to        = rcv_strdope->rcv_str;
	  size_t     to_size   = rcv_strdope->rcv_size;

	  if (EXPECT_FALSE(invalid_ipc_buffer(from) || 
			   partner->invalid_ipc_buffer(to)))
	    {
	      _recover_jmpbuf = 0;
              WARNX(Info, "long-IPC: invalid string address ("L4_PTR_FMT" or "
                          L4_PTR_FMT, (Address)from, Address(to));
	      return ipc_finish (partner, Ipc_err::Remsgcut);
	    }

	  // silently limit sizes to superpage size
	  if (to_size > Config::SUPERPAGE_SIZE)
	    to_size = Config::SUPERPAGE_SIZE;
	  if (from_size > Config::SUPERPAGE_SIZE)
	    from_size = Config::SUPERPAGE_SIZE;

	  min = from_size > to_size ? to_size : from_size;
	  if (min > 0)
	    {
	      // XXX no bounds checking!
	      setup_ipc_window(1, ((Address)to) & Config::SUPERPAGE_MASK);

	      // IPC has been aborted
	      if (!partner->in_long_ipc (this))
		return ipc_finish (partner, Ipc_err::Reaborted);

	      mem_space()->copy_from_user
		((Unsigned8*) (Kmem::ipc_window(1)
			       + (((Address)to) & ~Config::SUPERPAGE_MASK)),
		 from, min);
	    }

          // indicate size of received data
          // XXX also overwrite rcv_strdope->snd_str?
          rcv_strdope->snd_size = min;

	  if (EXPECT_FALSE(from_size > to_size))
	    break;

	  snd_strings--;
	  rcv_strings--;
	  snd_strdope++;
	  rcv_strdope++;

	  result_dope.strings (result_dope.strings()+1);
	  barrier();
	}

      _recover_jmpbuf = 0;

      if (EXPECT_FALSE(snd_strings))
        {
          WARNX(Info, "long-IPC: %d snd_strings left", snd_strings);
	  result_err.combine (Ipc_err (Ipc_err::Remsgcut));
        }

      partner->rcv_regs()->msg_dope (result_dope);
      return ipc_finish (partner, result_err);
    }
  else
    {
      // an exception occured
      _recover_jmpbuf = 0;

      // Finish IPC for partner.  It may very well be that our partner
      // has already bailed out of the IPC, but in this case we're
      // just returned Ipc_err::Seaborted -- that doesn't hurt.
      error &= ~Ipc_err::Send_error;
      result_err.combine(error);

      partner->rcv_regs()->msg_dope (result_dope);
      return ipc_finish (partner, result_err);
    }
}

