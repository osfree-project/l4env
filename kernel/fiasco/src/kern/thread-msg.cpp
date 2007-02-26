INTERFACE:

EXTENSION class Thread
{
private:

  /**
   * Compute remote pagefault address from the local pagefault address
   * in the local Long-IPC window.
   * @param pfa Local pagefault address in IPC window
   * @returns Remote pagefault address in partner's address space
   */
  Address remote_fault_addr (Address pfa);

  /**
   * Try to upgrade the local IPC window with a mapping from the
   * partner's address space.
   * @param pfa Local pagefault address in IPC window
   * @param remote_pfa Remote pagefault address in partner's address space
   * @returns true if the partner had the page mapped and we could copy
   *          false if the partner needs to map the page himself first
   */
  Mword update_ipc_window (Address pfa, Address remote_pfa);
};

IMPLEMENTATION [msg]:

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

// OSKIT crap
#include "undef_oskit.h"


/** Handle a page fault that occurred in one of the ``IPC-window'' 
    virtual-address-space regions.
    @param pfa virtual address of page fault
    @return IPC error code if an error occurred during page-fault handling,
            0 if page fault was handled successfully
    @pre current_thread() == this
 */
PRIVATE
L4_msgdope
Thread::handle_ipc_page_fault (Address pfa)
{
  vm_offset_t remote_pfa = remote_fault_addr (pfa);
  L4_msgdope err;

  for (;;)
    {
      cpu_lock.lock();
      if (!receiver()->in_long_ipc (this))
	{
	  cpu_lock.clear();
	  return L4_msgdope(L4_msgdope::SEABORTED); // IPC has been aborted
	}      

      // If the receiver has mapped the page in his address space already,
      // we can just copy the mapping into our IPC window.
      if (update_ipc_window (pfa, remote_pfa))
        return 0;		// Success

      cpu_lock.clear();
      
      // The receiver didn't have it mapped, so it needs to page in data
      state_add(Thread_polling_long);

      if ((err = ipc_pagein_request(receiver(), remote_pfa)).has_error())
	return err;

      // XXX ipc_pagein_request could already put us to sleep...  This
      // would save us another context switch and running this code:
      
      while (state_change_safely(~(Thread_polling_long|Thread_ipc_in_progress
				   |Thread_running),
				 Thread_polling_long|Thread_ipc_in_progress))
	{
	  schedule();
	}

      state_del(Thread_polling_long);

      if (! (state() & Thread_ipc_in_progress))
	{
	  assert (state() & Thread_cancel);

	  return L4_msgdope::SEABORTED;
	}

      // Find receiver's error code
      if (_pagein_error_code)
	return _pagein_error_code;
    }
}

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
L4_msgdope Thread::ipc_send_fpage(Thread* receiver,
				  L4_fpage from_fpage, L4_fpage to_fpage, 
				  vm_offset_t offset, bool finish)
{
  Lock_guard <Thread_lock> guard (receiver->thread_lock());

  if (((receiver->state() & (Thread_rcvlong_in_progress|Thread_ipc_in_progress)) 
       != (Thread_rcvlong_in_progress|Thread_ipc_in_progress))
      || receiver->partner() != this)
    return L4_msgdope(L4_msgdope::SEABORTED);

  L4_msgdope ret = fpage_map(space(), from_fpage,
			     receiver->space(), to_fpage,
			     offset);

  if (finish)			// should we finish the IPC operation?
    {
      // if we must finish the IPC operation, set the receiver's return code
      // and return the sender's return code (instead of the receiver's)
      receiver->receive_regs()->msg_dope( ret );
      ret = ret.has_error() 
	? L4_msgdope(ret.error() | L4_msgdope::SEND_ERROR)
	: L4_msgdope(0);

      state_del(Thread_send_in_progress);
      
      receiver->state_change(~Thread_ipc_mask, Thread_running);

      receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_RECEIVER);
    }
  else 
    receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_SENDER);

  return ret;
}

/** Finish an IPC.
    @param receiver the receiver of our message
    @param new_state sender's IPC error conditions accumulated so far
    @return IPC error code that should be returned to sender
 */
PRIVATE
L4_msgdope Thread::ipc_finish(Thread* receiver, L4_msgdope new_state)
{
  Lock_guard <Thread_lock> guard (receiver->thread_lock());

  if (((receiver->state() & (Thread_rcvlong_in_progress|Thread_ipc_in_progress)) 
       != (Thread_rcvlong_in_progress|Thread_ipc_in_progress))
      || receiver->partner() != this)
    return L4_msgdope(L4_msgdope::SEABORTED);

  receiver->receive_regs()->msg_dope( new_state );

  state_del(Thread_send_in_progress);
  receiver->state_change(~Thread_ipc_mask, Thread_running);

  receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_RECEIVER);

  return (new_state.has_error())
    ? L4_msgdope(new_state.error() | L4_msgdope::SEND_ERROR)
    : L4_msgdope(0);
}

/** Wake up sender.
    A receiver wants to wake up a sender after a page-in request.
    @param ipc_code IPC error code that should be signaled to sender.  
                    This error code flags error conditions that occurred
		    during a page-in in the receiver.
    @return 0 if successful.  Receiver's error code if sender was not
            in IPC anymore.
 */
PUBLIC
L4_msgdope Thread::ipc_continue(L4_msgdope ipc_code)
{
  Lock_guard <Thread_lock> guard (thread_lock());

  // Is the IPC sender still talking to us?
  if (((state() & (Thread_polling_long|Thread_ipc_in_progress
		   |Thread_send_in_progress))
       != (Thread_polling_long|Thread_ipc_in_progress|Thread_send_in_progress))
      || receiver() !=  thread_lock()->lock_owner())
    return L4_msgdope(L4_msgdope::REABORTED);

  // Tell it the error code
  _pagein_error_code = ipc_code.raw();

  // Resume sender.
  state_change(~Thread_polling_long, Thread_running);
  
  thread_lock()->set_switch_hint(SWITCH_ACTIVATE_RECEIVER);

  return L4_msgdope(0);
}

/** Send a page-in request to a receiver.
    Requests that the receiver pages in some pages needed by the sender 
    to copy data into the receiver's address space.
    @param receiver the receiver of our message
    @param address page fault's virtual address in receiver's address space
    @return 0 if request could be sent.  IPC error code if receiver
            was in incorrect state.
 */
PRIVATE
L4_msgdope Thread::ipc_pagein_request(Receiver* receiver, vm_offset_t address)
{
  Lock_guard<Thread_lock> guard (receiver->thread_lock());

  if (! receiver->in_long_ipc(this))
    return L4_msgdope(L4_msgdope::SEABORTED);

  receiver->set_pagein_request (address, this);
  receiver->thread_lock()->set_switch_hint(SWITCH_ACTIVATE_RECEIVER);

  return L4_msgdope(0);
}
