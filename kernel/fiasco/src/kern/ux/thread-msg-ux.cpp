/*
 * Fiasco-UX
 * Architecture specific Long-IPC code
 */

IMPLEMENTATION [ux]:

#include "long_msg.h"

/**
 * Carry out long IPC.
 *
 * This method assumes that IPC handshake has finished successfully
 * (do_ipc()).  Its arguments must correspond to those
 * supplied to do_ipc().
 * The method copies data from the sender's address space into the receiver's
 * using direct user-to-user address-space copies via the physical pages that
 * back both buffers. During copying, page table lookup failures in both the
 * sender's and the receiver's buffer can occur, leading to page-fault IPC
 * handling.
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

      snd_descr = (message_header*)regs->snd_desc().msg();
      if (EXPECT_FALSE(invalid_ipc_buffer(snd_descr)))
          return ipc_finish (partner, Ipc_err::Remsgcut);

      if(_target_desc.rmap())
	rcv_descr = 0;
      else if (EXPECT_TRUE(!partner->invalid_ipc_buffer(_target_desc.msg())))
	rcv_descr = (message_header*) _target_desc.msg();
      else
	 return ipc_finish (partner, Ipc_err::Remsgcut);

      // Treat dwords as fpage mappings
      bool snd_fpages = regs->snd_desc().map();

      // make sure this is not a register-only send operation -- such
      // operations should have been handled by do_ipc()
      // long IPC, or sending (short or long) fpage (or both)
      assert (snd_descr || snd_fpages);

      // set up return value in case we're aborted; we must have read
      // all information from eax at this point
      commit_ipc_success (regs, Ipc_err::Seaborted);

      // Make it possible to recover from bad user-level page faults
      // and IPC aborts
      _recover_jmpbuf = &pf_recovery;

      // read the fpage option from the receiver's message buffer
      L4_fpage rcv_fpage (rcv_descr
                          ? partner->mem_space()->peek_user (&rcv_descr->fp)
                          : _target_desc.fpage());

      // check if we are not expected to send flexpages
      if (EXPECT_FALSE (snd_fpages && !rcv_fpage.is_valid()))
        {
          _recover_jmpbuf = 0;
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
	  if (EXPECT_FALSE (snd_fpage.size() < 12))
	    {
	      // This is not a valid flexpage, so do not try to send it.
	      // Also, don't try to transfer any more flexpages.
	      // XXX this test could also be done in short ipc

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
				   regs->msg_word (0),
				   snd_fpage.grant(), snd_short_fpage);

	      // short-fpage send?
	      if (snd_short_fpage)
		{
		  _recover_jmpbuf = 0;
		  return ret;
		}

	      result_err.combine(ret);

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
	    result_err.error (Ipc_err::Remsgcut);

	  partner->rcv_regs()->msg_dope (result_dope);
	  return ipc_finish (partner, result_err);
        }

      // OK, so we have a long receive buffer.

      L4_msgdope rcv_sizedope (partner->mem_space()->
				  peek_user (&rcv_descr->size_dope));
      int rcv_words = rcv_sizedope.mwords();
      Mword min = (snd_words > rcv_words) ? rcv_words : snd_words;

      if (min > Sys_ipc_frame::num_reg_words())
	{
	  // XXX if the memcpy() fails somewhere in the middle because
	  // of a page fault, we don't report the memory words that
	  // have already been copied.
	  mem_space()->copy_user_to_user (partner->mem_space(),
	      rcv_descr->words + Sys_ipc_frame::num_reg_words(),
	      snd_descr->words + Sys_ipc_frame::num_reg_words(),
	      min - Sys_ipc_frame::num_reg_words());
	  result_dope.mwords (min);
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
	      if (snd_fpage.size() < 12)
		{
		  snd_fpages = false;
		  break;
		}

	      // otherwise, map
	      ret = ipc_snd_fpage (partner, snd_fpage, rcv_fpage,
				   regs->msg_word ((Mword *) snd_descr, n),
				   snd_fpage.grant(), false);

	      if (EXPECT_FALSE (ret.has_error()))
		break;

	      result_err.combine (ret);

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
      int rcv_strings = rcv_sizedope.strings();

      while (snd_strings && rcv_strings)
	{
	  Unsigned8 *from      = mem_space()->peek_user(&snd_strdope->snd_str);
	  Address    from_size = mem_space()->peek_user(&snd_strdope->snd_size);
	  Unsigned8 *to        = partner->mem_space()->
				    peek_user (&rcv_strdope->rcv_str);
	  Address    to_size   = partner->mem_space()->
				    peek_user (&rcv_strdope->rcv_size);

	  if (EXPECT_FALSE(invalid_ipc_buffer(from) || partner->invalid_ipc_buffer(to)))
	    {
	      _recover_jmpbuf = 0;
	      return ipc_finish (partner, Ipc_err::Remsgcut);
	    }

	  // silently limit sizes to 4MB
	  if (to_size > Config::SUPERPAGE_SIZE)
	    to_size = Config::SUPERPAGE_SIZE;
	  if (from_size > Config::SUPERPAGE_SIZE)
	    from_size = Config::SUPERPAGE_SIZE;

	  min = from_size > to_size ? to_size : from_size;
	  if (min > 0)
	    mem_space()->copy_user_to_user (partner->mem_space(), 
					    to, from, min);

	  // indicate size of received data
	  // XXX also overwrite rcv_strdope->snd_str?
	  partner->mem_space()->poke_user (&rcv_strdope->snd_size, min);

	  if (EXPECT_FALSE(from_size > to_size))
	    break;

	  snd_strings--;
	  rcv_strings--;
	  snd_strdope++;
	  rcv_strdope++;

	  result_dope.strings (result_dope.strings()+1);
	}

      _recover_jmpbuf = 0;

      if (EXPECT_FALSE(snd_strings))
	result_err.combine (Ipc_err (Ipc_err::Remsgcut));

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
      if (error == 1)			// did we got an IPC error code?
	error = Ipc_err::Semsgcut;	// no -> use a generic one
      result_err.combine(error);

      partner->rcv_regs()->msg_dope (result_dope);
      return ipc_finish (partner, result_err);
    }
}

IMPLEMENT inline                       
Address
Thread::remote_fault_addr (Address pfa)
{                                          
  return pfa;
}
