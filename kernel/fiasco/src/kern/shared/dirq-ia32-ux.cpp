IMPLEMENTATION[ia32,amd64,ux]:

#include <cassert>

#include "cpu_lock.h"
#include "globalconfig.h"
#include "globals.h"
#include "logdefs.h"
#include "std_macros.h"

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


#if 0				// screen spinner for debugging purposes
  (*(unsigned char*)(kmem::phys_to_virt(0xb8000 + irqnum*2)))++;
#endif

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif

  i->hit();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.clear_irqdisable(); 
#endif
}

