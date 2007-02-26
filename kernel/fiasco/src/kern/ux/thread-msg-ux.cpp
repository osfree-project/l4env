/*
 * Fiasco-UX
 * Architecture specific Long-IPC code
 */

IMPLEMENTATION[msg-ux]:

#include "long_msg.h"

IMPLEMENT inline                       
Address
Thread::remote_fault_addr (Address pfa)
{                                          
  return pfa;
}

/** Carry out long IPC.
 *  This method assumes that IPC handshake has finished successfully 
 *  (ipc_send_regs()).  Its arguments must correspond to those
 *  supplied to ipc_send_regs().
 *  The method copies data from the sender's address space into the receiver's
 *  using direct user-to-user address-space copies via the physical pages that
 *  back both buffers. During copying, page table lookup failures in both the
 *  sender's and the receiver's buffer can occur, leading to page-fault IPC
 *  handling.
 *  @param partner the receiver of our message
 *  @param regs sender's IPC registers
 *  @return sender's IPC error code
 */
PRIVATE 
L4_msgdope
Thread::do_send_long (Thread *partner, Sys_ipc_frame *i_regs)
{
   Long_msg *regs = (Long_msg*) i_regs;

  // just copy the message here.  don't care about pages not being
  // mapped in -- this is completely handled in the page fault handler.
  
  // XXX bound check missing!

  // 2 dwords in registers
  L4_msgdope result (Sys_ipc_frame::num_reg_words(), 0);
  
  jmp_buf pf_recovery;  
  unsigned error;

  if (EXPECT_TRUE ((error = setjmp(pf_recovery)) == 0) )   
    {
      // set up message buffers
      struct message_header {
        L4_fpage     fp;
        L4_msgdope   size_dope;
        L4_msgdope   send_dope;
        Mword        words[1];
      } *snd_descr, *rcv_descr;

      snd_descr = (message_header *) regs->snd_desc().msg();
      rcv_descr = (message_header *) 
			(_target_desc.rmap() ? 0 : _target_desc.msg());

      // Treat dwords as fpage mappings
      bool send_fpages = regs->snd_desc().map();
      
      // make sure this is not a register-only send operation -- such
      // operations should have been handled by ipc_send_regs()
      // long IPC, or sending (short or long) fpage (or both)
      assert (snd_descr || send_fpages);

      // set up return value in case we're aborted; we must have read
      // all information from eax at this point
      regs->msg_dope (L4_msgdope::SEABORTED);

      // Make it possible to recover from bad user-level page faults
      // and IPC aborts      
      _recover_jmpbuf = &pf_recovery;

      // read the fpage option from the receiver's message buffer
      L4_fpage rcv_fpage (rcv_descr ? partner->peek_user (&rcv_descr->fp) :
                                      _target_desc.fpage());

      // check if we are not expected to send flexpages
      if (send_fpages && !rcv_fpage.is_valid())
        {
          _recover_jmpbuf = 0;
          return ipc_finish (partner, L4_msgdope::REMSGCUT);
        }

      L4_msgdope ret;
      
      if (send_fpages) {			// send flexpages?

        // transfer the first flexpage -- the one transferred in the
        // message registers.

        bool send_short_fpage = !snd_descr;
        L4_fpage snd_fpage (regs->msg_word (1));

        if (snd_fpage.size() < 12) {	// fpage valid?

          // This is not a valid flexpage, so do not try to send it.
          // Also, don't try to transfer any more flexpages.
          // XXX this test could also be done in ipc_send_regs().

          if (send_short_fpage)       		// short-fpage send?
            {
              _recover_jmpbuf = 0;
              return ipc_finish (partner, 0);	// if yes, we're finished
            }

          send_fpages = false;		// don't try to send more flexpages

        } else {

          ret = ipc_send_fpage (partner, snd_fpage, rcv_fpage, 
    			    regs->msg_word (0) & Config::PAGE_MASK,
                                send_short_fpage);
      
          if (send_short_fpage)		// short-fpage send?
            {
              _recover_jmpbuf = 0;
              return ret;		// if yes, we're finished
            }

          result.combine(ret);

          if (result.has_error())
            {
              _recover_jmpbuf = 0;
              return ipc_finish (partner, result);
            }
        }
      }

      // OK, so we have to send a long message buffer.
      assert (snd_descr);

      // We start by transferring dwords and do flexpage mappings on our way.
      L4_msgdope snd_send_dope (peek_user (&snd_descr->send_dope));
      int snd_words = snd_send_dope.mwords();

      // We must stop now if we don't have a receive buffer.  Check
      // for cut message.
      if (!rcv_descr)
        {
          _recover_jmpbuf = 0;
          if (!(snd_words <= (int) Sys_ipc_frame::num_reg_words() &&
                snd_send_dope.strings() == 0))
            result.error (L4_msgdope::REMSGCUT);

          return ipc_finish (partner, result);
        }

      // OK, so we have a long receive buffer.

      L4_msgdope rcv_size_dope (partner->peek_user (&rcv_descr->size_dope));
      int rcv_words = rcv_size_dope.mwords();

      unsigned min = (snd_words > rcv_words) ? rcv_words : snd_words;

      if (min > Sys_ipc_frame::num_reg_words()) {

        // XXX if the memcpy() fails somewhere in the middle because
        // of a page fault, we don't report the memory words that
        // have already been copied.

        copy_user_to_user (partner,
                           rcv_descr->words + Sys_ipc_frame::num_reg_words(),
	                   snd_descr->words + Sys_ipc_frame::num_reg_words(),
                           min - Sys_ipc_frame::num_reg_words());

        result.mwords (min);
      }

      if (send_fpages) {

        // Start with message word 2
        unsigned n = 2, s = snd_words, r = rcv_words;

        // We have already successfully transferred the
        // register-dwords flexpage. Just care about flexpages in
        // the memory buffer here.

        while (s >= Sys_ipc_frame::num_reg_words() + 2 &&
               r >= Sys_ipc_frame::num_reg_words() + 2) {

          L4_fpage snd_fpage (regs->msg_word (snd_descr, n + 1));

          if (snd_fpage.size() < 12) {	// if not a valid flexpage, break
            send_fpages = false;
            break;
          }

          // otherwise, map
          ret = ipc_send_fpage (partner, snd_fpage, rcv_fpage,
                                regs->msg_word (snd_descr, n) & Config::PAGE_MASK,
                                false);

          if (ret.has_error())
            break;

          result.combine (ret);

          n += 2;
          s -= 2;
          r -= 2;
        }
      }

      if (ret.has_error())		// a mapping couldn't be established
        {
          _recover_jmpbuf = 0;
          ret.combine(result);
          return ipc_finish(partner, ret);
        }

      else if (snd_words > rcv_words)	// the message has been cut
        {
          _recover_jmpbuf = 0;
          result.combine(L4_msgdope(L4_msgdope::REMSGCUT));
          return ipc_finish(partner, result);
        }

      // All right -- the direct string in the message buffer has been
      // transferred.  Now tackle the indirect strings.

      L4_msgdope snd_size_dope (peek_user (&snd_descr->size_dope));

      L4_str_dope *snd_strdope = reinterpret_cast<L4_str_dope *>
            (snd_descr->words + snd_size_dope.mwords());

      L4_str_dope *rcv_strdope = reinterpret_cast<L4_str_dope *>
            (rcv_descr->words + rcv_size_dope.mwords());

      int snd_strings = snd_send_dope.strings();
      int rcv_strings = rcv_size_dope.strings();

      while (snd_strings && rcv_strings) {

        Unsigned8 *from   = peek_user (&snd_strdope->snd_str);
        Address from_size = peek_user (&snd_strdope->snd_size);
        Unsigned8 *to     = partner->peek_user (&rcv_strdope->rcv_str);
        Address to_size   = partner->peek_user (&rcv_strdope->rcv_size);

        min = from_size > to_size ? to_size : from_size;

        if (min > 0)
          copy_user_to_user (partner, to, from, min);

        // indicate size of received data
        // XXX also overwrite rcv_strdope->snd_str?
        partner->poke_user (&rcv_strdope->snd_size, min);

        if (from_size > to_size) 
          break;      

        snd_strings--;
        rcv_strings--;
        snd_strdope++;
        rcv_strdope++;

        result.strings (result.strings() + 1);
      }
      
      _recover_jmpbuf = 0;
     
      if (snd_strings)
        result.combine (L4_msgdope (L4_msgdope::REMSGCUT));

      return ipc_finish (partner, result);
    }
  else
    {
      _recover_jmpbuf = 0;
      
      // Finish IPC for partner.  It may very well be that our partner
      // has already bailed out of the IPC, but in this case we're
      // just returned L4_msgdope::SEABORTED -- that doesn't hurt.
      error &= ~L4_msgdope::SEND_ERROR;
      result.combine(error);

      return ipc_finish(partner, result);
    }                                 
}
