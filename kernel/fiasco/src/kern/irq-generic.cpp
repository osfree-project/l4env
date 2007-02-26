IMPLEMENTATION[generic]:

#include "pic.h"


/** If this is a kernel-acknowledged IRQ, disable and acknowledge the
    IRQ with the PIC.
    XXX We assume here that we have the kernel lock.
 */
PUBLIC inline NEEDS ["pic.h"]
void Irq::maybe_acknowledge()
{
  if (_ack_in_kernel)
    {
      unsigned irqnum = id().irq();
      
      Pic::disable_locked(irqnum);
      Pic::acknowledge_locked(irqnum);
    }
}
 
/** If this is a kernel-acknowledged IRQ, enable the IRQ with the PIC. */
PUBLIC inline NEEDS ["pic.h"]
void Irq::maybe_enable()
{
  if (_ack_in_kernel)
    {
      unsigned irqnum = id().irq();
      Pic::enable(irqnum);
    }
}
