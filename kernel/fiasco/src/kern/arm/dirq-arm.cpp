IMPLEMENTATION[arm]:

#include "atomic.h"
#include "initcalls.h"
#include "irq.h"
#include "receiver.h"

static char dirq_storage[sizeof(Dirq)* 32]; // 16 device irq's

IMPLEMENT 
void *Dirq::operator new ( size_t ) 
{
  static unsigned first = 0;
  return reinterpret_cast<Dirq*>(dirq_storage)+(first++);
}

IMPLEMENT FIASCO_INIT
void
Dirq::init()
{
  for( unsigned i = 0; i<32; ++i )
    new Dirq(i);
}


/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
                         as opposed to user-lever acknowledgement
    @return true if the binding could be established
 */
IMPLEMENT inline NEEDS ["atomic.h"]
bool Dirq::alloc(Receiver *t, bool ack_in_kernel)
{
  bool ret = smp_cas(&_irq_thread, reinterpret_cast<Receiver*>(0), t);

  if (ret) 
    {
      int irq = id().irq();

      _ack_in_kernel = ack_in_kernel;
      _queued = 0;

      if (_ack_in_kernel)
	Pic::enable(irq);
      else
	Pic::disable(irq);

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
  bool ret = smp_cas(&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
      Pic::disable(id().irq());
      sender_dequeue(t->sender_list());
    }

  return ret;
}
