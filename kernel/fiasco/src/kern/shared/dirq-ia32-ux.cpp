IMPLEMENTATION[ia32,ux]:

#include <cassert>

#include "atomic.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "globalconfig.h"
#include "globals.h"
#include "initcalls.h"
#include "kdb_ke.h"
#include "logdefs.h"
#include "pic.h"
#include "receiver.h"
#include "static_init.h"
#include "std_macros.h"
#include "thread_state.h"
#include "thread_lock.h"

/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
                         as opposed to user-lever acknowledgement
    @return true if the binding could be established
 */
IMPLEMENT
bool
Dirq::alloc(Receiver *t, bool ack_in_kernel)
{
  bool ret = cas (&_irq_thread, reinterpret_cast<Receiver*>(0), t);

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
IMPLEMENT
bool
Dirq::free(Receiver *t)
{
  bool ret = cas (&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
      Pic::disable(id().irq());
      sender_dequeue(t->sender_list());
    }

  return ret;
}

/** The corresponding hardware interrupt occurred -- handle it. 
    This method checks whether the attached receiver is ready to receive 
    an IRQ message, and if so, restarts it so that it can pick up the IRQ
    message using ipc_receiver_ready().
 */


/** Hardware interrupt entry point.  Calls corresponding Dirq instance's
    Dirq::hit() method.
    @param irqnum hardware-interrupt number
 */
extern "C" FIASCO_FASTCALL
void
irq_interrupt(Mword irqnum, Mword ip)
{
  extern unsigned gdb_trap_recover;

  LOG_IRQ(irqnum);
  CNT_IRQ;
  (void)ip;

  // don't receive interrupts while kernel debugger is active
  assert(gdb_trap_recover == 0);
  (void)gdb_trap_recover;

  // we're entered with disabled irqs
  Irq *i = Dirq::lookup(irqnum);

  i->maybe_acknowledge();

#if 0				// screen spinner for debugging purposes
  (*(unsigned char*)(kmem::phys_to_virt(0xb8000 + irqnum*2)))++;
#endif

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif

  static_cast<Dirq*>(i)->hit();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.clear_irqdisable(); 
#endif
}

PUBLIC
void 
Dirq::acknowledge()
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  Pic::acknowledge(id().irq());
  Pic::enable(id().irq());
}
