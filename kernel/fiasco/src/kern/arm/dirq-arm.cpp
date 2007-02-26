IMPLEMENTATION[arm]:

#include "atomic.h"
#include "initcalls.h"
#include "irq.h"
#include "receiver.h"

static char dirq_storage[sizeof(dirq_t)* 32]; // 16 device irq's

IMPLEMENT 
void *dirq_t::operator new ( size_t ) 
{
  static unsigned first = 0;
  return reinterpret_cast<dirq_t*>(dirq_storage)+(first++);
}

IMPLEMENT FIASCO_INIT
void
dirq_t::init()
{
  for( unsigned i = 0; i<32; ++i )
    new dirq_t(i);
}


/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
                         as opposed to user-lever acknowledgement
    @return true if the binding could be established
 */
IMPLEMENT inline NEEDS ["atomic.h"]
bool dirq_t::alloc(Receiver *t, bool ack_in_kernel)
{
  bool ret = smp_cas(&_irq_thread, reinterpret_cast<Receiver*>(0), t);

  if (ret) 
    {
      int irq = id().irq();

      _ack_in_kernel = ack_in_kernel;
      _queued = 0;
#warning Pic must be handled
#if 0
      if (_ack_in_kernel)
	Pic::enable(irq);
      else
	Pic::disable(irq);
#endif
    }

  return ret;
}

/** Release an device interrupt.
    @param t the receiver that ownes the IRQ
    @return true if t really was the owner of the IRQ and operation was 
            successful
 */
IMPLEMENT inline NEEDS ["receiver.h"]
bool dirq_t::free(Receiver *t)
{
  bool ret = smp_cas(&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
#warning Pic must be handled
#if 0
      Pic::disable(id().irq());
#endif
      sender_dequeue(t->sender_list());
    }

  return ret;
}
