IMPLEMENTATION[ipc]:

#include "entry_frame.h"
#include "globalconfig.h"
#include "l4_types.h"
#include "receiver.h"
#include "thread_state.h"


/** Sender-activation function called when receiver gets ready.
    irq_t::hit() actually ensures that this method is always called
    when an interrupt occurs, even when the receiver was already
    waiting. 
 */
#ifdef CONFIG_APIC_MASK
extern "C" unsigned apic_timer_entry;
extern "C" unsigned apic_irq_mask;
extern "C" unsigned apic_irq_nr;
#endif
PUBLIC 
virtual void 
irq_t::ipc_receiver_ready()
{
  assert(current() == _irq_thread);

  if (_queued && _irq_thread->ipc_try_lock(nonull_static_cast<Sender*>(this)) == 0)
    {
      _irq_thread->receive_regs()->msg_dope(0);	// state = OK

#ifdef CONFIG_APIC_MASK
      if (id().lh.low-1 == apic_irq_nr)
	{
	  _irq_thread->receive_regs()->edx = apic_timer_entry;
	  _irq_thread->receive_regs()->ebx = apic_irq_mask;
	}
#endif

      _irq_thread->state_change(~(Thread_waiting | Thread_receiving
				  | Thread_busy  | Thread_ipc_in_progress),
				Thread_running);

      // XXX receiver should also get a fresh timeslice
      if (consume() < 1)	// last interrupt in queue?
	{
	  sender_dequeue(_irq_thread->sender_list());

	  // Now that the interrupt has been delivered, it is OK for it to
	  // occur again.
	  maybe_enable();
	}
      // else remain queued if more interrupts are left

      _irq_thread->ipc_unlock();
    }
  
  return;
}
