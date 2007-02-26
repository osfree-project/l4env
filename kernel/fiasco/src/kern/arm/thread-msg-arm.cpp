IMPLEMENTATION[msg-arm]:

#include "kdb_ke.h"
#include "long_msg.h"


IMPLEMENT /*inline*/
Address Thread::remote_fault_addr (Address /*pfa*/)
{
  kdb_ke("UNIMPLEMENTED: remote_fault_addr");
#if 0
  return pfa < Kmem::ipc_window(1) ? pfa - Kmem::ipc_window(0) + _vm_window0
                                   : pfa - Kmem::ipc_window(1) + _vm_window1;
#endif
  return 0;
}

IMPLEMENT /*inline*/
Mword Thread::update_ipc_window (Address /*pfa*/, Address /*remote_pfa*/)
{
  kdb_ke("UNIMPLEMENTED: remote_fault_addr");
#if 0
  // XXX: We don't care about the page-fault error code here, but
  // we should: If we want to write to a read-only page, we would
  // fail here.  Space::update() probably should take an
  // error-code argument, as should
  // Thread::ipc_pagein_request().

  if (space()->update (pfa, receiver()->space_context(), remote_pfa))
    {
      cpu_lock.clear();

      // It's OK if the PF occurs again: This can happen if we're
      // preempted after the call to update() above.  (This code
      // corresponds to code in Thread::handle_page_fault() that
      // checks for double page faults.)
      if (Config::monitor_page_faults)
        {
          _last_pf_address = (vm_offset_t) -1;
        }

      return 1;		// Success
    }
#endif
  return 0;		// Failure
}

/** Carry out long IPC.
    This method assumes that IPC handshake has finished successfully
    (ipc_send_regs()).  Its arguments arguments must correspond to those
    supplied to ipc_send_regs().
    The method copies data from the sender's address space into the receiver's
    using IPC address-space windows.  During copying, page faults in
    both the sender's and the receiver's buffer can occur, leading to
    page-fault--IPC handling.
    When using small address spaces great care must be taken when copying
    out of the users address space, as kernel and user may use different
    segments.
    @param partner the receiver of our message
    @param regs sender's IPC registers
    @return sender's IPC error code
 */
PRIVATE 
L4_msgdope Thread::do_send_long(Thread */*partner*/,               // l4_timeout_t t
				Sys_ipc_frame */*i_regs*/)
{
  kdb_ke("UNIMPLEMENTED: remote_fault_addr");
  return 0;
#if 0
  Long_msg *regs = (Long_msg*)i_regs;
  // just copy the message here.  don't care about pages not being  
  // mapped in -- this is completely handled in the page fault handler.

  // XXX bound check missing!
  
  // The nice asm statements (asm volatile ("" : : "r"(&result) : "memory");)
  // must occur after all write accesses to result that shall be seen after 
  // an eventual jongjmp recovery.
  L4_msgdope result(Sys_ipc_frame::num_reg_words(), 0); // 2 dwords in regs
  asm volatile ("" : : : "memory");

  // set up exception handling
  jmp_buf pf_recovery;  
  unsigned error;
  if ((error = setjmp(pf_recovery)) == 0)
    {
      // set up pointers to message buffers
  
      struct message_header {   
        L4_fpage     fp;
        L4_msgdope   size_dope;
        L4_msgdope   send_dope;
        Mword        words[1];
      } *snd_descr, *rcv_header;
  
      //save the address of the snd_header for later use
      //be careful, although it is a pointer, there is no valid
      //data behind it (see small address spaces)
      snd_descr = (message_header*)regs->snd_desc().msg();
  
      if (_target_desc.msg() == 0) // receiver expects short message
        {
          rcv_header = 0;
        }
      else
        { 
          // the receiver's message buffer is mapped into VM window 1.
          setup_ipc_window(0, ((Address)(_target_desc.msg())) 
						     & Config::SUPERPAGE_MASK);
          rcv_header = reinterpret_cast<message_header*>
            (Kmem::ipc_window(0) + ((Address)_target_desc.msg() 
						    & ~Config::SUPERPAGE_MASK));
        }
  
      bool send_fpages = regs->snd_desc().map();
      
      // make sure this is not a register-only send operation -- such
      // operations should have been handled by ipc_send_regs()
      assert(snd_descr || send_fpages);// long IPC, or sending
                                        // (short or long) fpage (or both)

      // set up return value in case we're aborted; we must have read
      // regs->snd_desc() is inavlid from now
      regs->msg_dope( L4_msgdope::SEABORTED );
  
      // Make it possible to recover from bad user-level page faults
      // and IPC aborts
      _recover_jmpbuf = &pf_recovery;

      // read the fpage option from the receiver's message buffer
      L4_fpage rcv_fpage = 
        rcv_header ? rcv_header->fp
                   : _target_desc.fpage();

      // check if we cannot send flexpages
      if (send_fpages && ! rcv_fpage.is_valid())
        {
          _recover_jmpbuf = 0;
          return ipc_finish(partner, L4_msgdope::REMSGCUT);
        }

      L4_msgdope ret;

      // send flexpages?
      if (send_fpages)  
        {
          // transfer the first flexpage -- the one transferred in the
          // message registers.

          bool send_short_fpage = !snd_descr;
          L4_fpage snd_fpage = L4_fpage(regs->msg_word(1));

          if (/* snd_fpage.fpage == 0 || */ snd_fpage.size() < 12) // fpage valid?
            {
              // This is not a valid flexpage, so do not try to send it.
              // Also, don't try to transfer any more flexpages.
              // XXX this test could also be done in ipc_send_regs().

              if (send_short_fpage) // short-fpage send?
                {               // if yes, we're finished
                  _recover_jmpbuf = 0;
                  return ipc_finish(partner, 0);
                }

              send_fpages = false; // don't try to send more flexpages
            }
          else
            { 
              ret = ipc_send_fpage(partner, snd_fpage, rcv_fpage,
				   regs->msg_word(0) & Config::PAGE_MASK,
				   send_short_fpage);
	      
              if (send_short_fpage) // short-fpage send?
                {               // if yes, we're finished
                  _recover_jmpbuf = 0;
                  return ret;
                }

              result.combine(ret);
	      asm volatile ("" : : : "memory");

              if (result.has_error())
                {
                  _recover_jmpbuf = 0;
                  return ipc_finish(partner, result);
                }
            }
        }

      // OK, so we have to send a long message buffer.
      assert(snd_descr);

      // We start by transferring dwords and do flexpage mappings on our way.

      L4_msgdope snddope(peek_user(&snd_descr->send_dope));

      int snd_words = snddope.mwords();

      // We must stop now if we don't have a receive buffer.  Check
      // for cut message.
      if (! rcv_header)  
        {
          _recover_jmpbuf = 0;
	  if(!(snd_words <= (int)Sys_ipc_frame::num_reg_words() 
	       && snddope.strings() == 0))
	    result.error(L4_msgdope::REMSGCUT);
          return ipc_finish(partner, result);
        }

      // OK, so we have a long receive buffer.

      int rcv_words = rcv_header->size_dope.mwords();
      
      unsigned s = snd_words, r = rcv_words;
      unsigned min = (snd_words > rcv_words) ? rcv_words : snd_words;

      if (min>Sys_ipc_frame::num_reg_words())
        {
          // XXX if the memcpy() fails somewhere in the middle because
          // of a page fault, we don't report the memory words that   
          // have already been copied.
	  copy_from_user<Mword>
	    (rcv_header->words + Sys_ipc_frame::num_reg_words(),
	     snd_descr->words  + Sys_ipc_frame::num_reg_words(),
	     min - Sys_ipc_frame::num_reg_words());
          result.mwords( min );
	  asm volatile ("" : : : "memory");
        }

      if (send_fpages)
        {
          // We have already successfully transferred the
          // register-dwords flexpage.  Just care about flexpages in
          // the memory buffer here.
	  unsigned pos = 2; // start with message word 2

          while (s >= (Sys_ipc_frame::num_reg_words() + 2)  
		 && r >= (Sys_ipc_frame::num_reg_words() + 2))
            {

              L4_fpage snd_fpage(regs->msg_word(snd_descr,pos+1));

              // if not a valid flexpage, break
              if (/* snd_fpage.fpage == 0 || */ snd_fpage.size() < 12)
                {
                  send_fpages = false;
                  break;
                }
#warning shall we mask the offset (page sized)
              // otherwise, map
              ret = ipc_send_fpage(partner, snd_fpage, rcv_fpage,
				   regs->msg_word(snd_descr,pos), false);
              if (ret.has_error())
                break;

              result.combine(ret);
	      asm volatile ("" : : : "memory");

	      pos += 2;
              s -= 2;
              r -= 2;
            }
        }

      if (ret.has_error())
        {      
          // a mapping couldn't be established
          _recover_jmpbuf = 0;
	  ret.combine(result);
          return ipc_finish(partner, ret);
        }
      else if (snd_words > rcv_words)
        {
          // the message has been cut
          _recover_jmpbuf = 0;
	  result.combine(L4_msgdope(L4_msgdope::REMSGCUT));
          return ipc_finish(partner, result);
        }
      // All right -- the direct string in the message buffer has been
      // transferred.  Now tackle the indirect strings.
 
      L4_msgdope sizedope(peek_user(&(snd_descr->size_dope)));

      L4_str_dope *snd_strdope = reinterpret_cast<L4_str_dope *>
        (snd_descr->words + sizedope.mwords());

      L4_str_dope *rcv_strdope = reinterpret_cast<L4_str_dope *>
        (rcv_header->words + rcv_words);

      int snd_strings = snddope.strings();
      int rcv_strings = rcv_header->size_dope.strings();

      s = snd_strings;
      r = rcv_strings;

      while (s && r)
        {
          vm_offset_t from = peek_user(&snd_strdope->snd_str);
          vm_offset_t from_size = peek_user(&snd_strdope->snd_size);
          vm_offset_t to = rcv_strdope->rcv_str;
          vm_offset_t to_size = rcv_strdope->rcv_size;

          // silently limit sizes to 4MB
          if (to_size > Config::SUPERPAGE_SIZE)
	    to_size = Config::SUPERPAGE_SIZE;
          if (from_size > Config::SUPERPAGE_SIZE)
	    from_size = Config::SUPERPAGE_SIZE;

          min = (from_size > to_size) ? to_size : from_size;
          if (min > 0)
            {
              // XXX no bounds checking!
              setup_ipc_window(1, to & Config::SUPERPAGE_MASK);
              copy_from_user<Unsigned8>((void*)(Kmem::ipc_window(1) 
						+ (to & ~Config::SUPERPAGE_MASK)),
					(void*)from, min);
            }

          // indicate size of received data
          rcv_strdope->snd_size = min;
          // XXX also overwrite rcv_strdope->snd_str?

          if (from_size > to_size)
            break;      

          s--;
          r--;
          snd_strdope++;
          rcv_strdope++;

          result.strings(result.strings()+1);
	  asm volatile ("" : : : "memory");
        }
  
      _recover_jmpbuf = 0;
      if(s)
	result.combine(L4_msgdope(L4_msgdope::REMSGCUT));

      return ipc_finish(partner, result);

    }
  else                          // an exception occurred
    {
      // printf("*Pexc: err=%x\n", error);

      _recover_jmpbuf = 0;

      // Finish IPC for partner.  It may very well be that our partner
      // has already bailed out of the IPC, but in this case we're
      // just returned L4_msgdope::SEABORTED -- that doesn't hurt.
      error &= ~L4_msgdope::SEND_ERROR;      
      result.combine(error);
      
      return ipc_finish(partner, result);
    }
#endif
}
