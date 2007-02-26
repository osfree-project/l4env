IMPLEMENTATION[arm]:

/** If this is a kernel-acknowledged IRQ, disable and acknowledge the
    IRQ with the PIC.
    XXX We assume here that we have the kernel lock.
 */
PUBLIC inline
void irq_t::maybe_acknowledge()
{
  if (_ack_in_kernel)
    {
#warning Pic handling
#if 0      
      unsigned irqnum = id().irq();
      Pic::disable_locked(irqnum);
      Pic::acknowledge_locked(irqnum);
#endif
    }
}
 
/** If this is a kernel-acknowledged IRQ, enable the IRQ with the PIC. */
PUBLIC inline
void irq_t::maybe_enable()
{
  if (_ack_in_kernel)
    {
#warning Pic handling
#if 0      
      unsigned irqnum = id().irq();
      Pic::enable(irqnum);
#endif
    }
}

