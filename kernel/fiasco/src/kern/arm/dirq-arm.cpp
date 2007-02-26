IMPLEMENTATION [arm]:

#include "atomic.h"
#include "config.h"
#include "initcalls.h"
#include "irq.h"
#include "receiver.h"


/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
                         as opposed to user-lever acknowledgement
    @return true if the binding could be established
 */
IMPLEMENT inline NEEDS ["atomic.h"]
bool Dirq::alloc(Receiver *t, bool /*ack_in_kernel*/)
{
  bool ret = cas (&_irq_thread, reinterpret_cast<Receiver*>(0), t);

  if (ret) 
    {
      int irq = id().irq();
      _queued = 0;
      if ((unsigned long)t != ~0UL)
	// Assign the receivers prio to the IRQ
        Pic::enable(irq, t->sched()->prio());
      else
	// Assign the highes possible IRQ prio to JDB IRQs
	Pic::enable(irq, ~0U);
    }

  return ret;
}

/** Release an device interrupt.
    @param t the receiver that ownes the IRQ
    @return true if t really was the owner of the IRQ and operation was 
            successful
 */
IMPLEMENT inline NEEDS ["receiver.h"]
bool Dirq::free(Receiver *t)
{
  bool ret = cas (&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
      Pic::disable(id().irq());
      sender_dequeue(t->sender_list());
    }

  return ret;
}

PUBLIC
void 
Dirq::acknowledge()
{ Pic::acknowledge(id().irq()); }
