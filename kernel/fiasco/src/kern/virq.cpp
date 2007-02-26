INTERFACE:

#include "observer.h"
#include "irq.h"

class Virq : public Irq, public Observer
{
private:
  Virq();
  Virq(Virq&);
};


IMPLEMENTATION:

#include "config.h"
#include "kdb_ke.h"
#include "atomic.h"
#include "receiver.h"
#include "thread_state.h"
#include "static_init.h"

PUBLIC inline
explicit
Virq::Virq(unsigned irqnum) 
  : Irq(irqnum)
{
}

/** 
 * Bind a receiver to this virtual interrupt.
 * @param t the receiver that wants to receive IPC messages for this IRQ
 * @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
 *        as opposed to user-level acknowledgement
 * @return true if the binding could be established
 */
PUBLIC inline NEEDS ["atomic.h"]
bool
Virq::alloc(Receiver *t, bool /*ack_in_kernel*/)
{
  bool ret = cas (&_irq_thread, static_cast<Receiver*>(0), t);

  if (ret)
    {
      _ack_in_kernel = 0;
      _queued = 0;
    }

  return ret;
}

/** Release an virtual interrupt.
 * @param t the receiver that owns the IRQ
 * @return true if t really was the owner of the IRQ and operation was 
 * successful
 */
PUBLIC inline NEEDS ["receiver.h"]
bool
Virq::free(Receiver *t)
{
  bool ret = cas (&_irq_thread, t, static_cast<Receiver*>(0));

  if (ret) 
    {
      sender_dequeue(t->sender_list());
    }

  return ret;
}

/** The corresponding virtual interrupt occurred -- handle it. 
    This method checks whether the attached receiver is ready to receive 
    an IRQ message, and if so, restarts it so that it can pick up the IRQ
    message using ipc_receiver_ready().
 */
PUBLIC inline NEEDS ["receiver.h","kdb_ke.h","config.h"]
void
Virq::hit()
{
  // We're entered holding the kernel lock, which also means irqs are
  // disabled on this CPU (XXX always correct?).  We never enable irqs
  // in this stack frame (except maybe in a nonnested invocation of
  // switch_exec() -> switchin_context()) -- they will be re-enabled
  // once we return from it (iret in entry.S:all_irqs) or we switch to
  // a different thread.
  if (_irq_thread == (void*)-1) /* debugger attached to IRQ */ 
    {
#if defined(CONFIG_JDB)
      kdb_ke("IRQ ENTRY");
#endif
      return;
    }

  if (! _irq_thread)
    return;

  if (_queued++ == 0)	// increase hit counter
    {
      set_receiver (_irq_thread);
      sender_enqueue(_irq_thread->sender_list(), _irq_thread->sched()->prio());
      
      // if the thread is waiting for this interrupt, make it ready;
      // this will cause it to run irq->receiver_ready(), which
      // handles the rest

      // XXX careful!  This code may run in midst of a do_ipc()
      // operation (or similar)!

      if (_irq_thread->sender_ok (this))
	{
	  // we don't need to manipulate the state in a safe way
	  // because we are still running with interrupts turned off
	  _irq_thread->state_change_dirty(~Thread_busy, Thread_ready);

	  _irq_thread->ready_enqueue();
	}
    }
}

PUBLIC inline
void
Virq::notify()
{
  hit();
}

PUBLIC 
void 
Virq::acknowledge()
{
}

