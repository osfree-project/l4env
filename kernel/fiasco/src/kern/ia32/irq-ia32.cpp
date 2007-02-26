IMPLEMENTATION[ia32]:

#include "config.h"
#include "idt.h"
#include "kmem.h"
#include "processor.h"
#include "pic.h"

Pic::Status pic_status;

#ifdef CONFIG_APIC_MASK
static unsigned apic_timer_irq_enabled;
#endif



/** If this is a kernel-acknowledged IRQ, disable and acknowledge the
    IRQ with the PIC.
    XXX We assume here that we have the kernel lock.
 */
PUBLIC inline NEEDS ["pic.h"]
void irq_t::maybe_acknowledge()
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
void irq_t::maybe_enable()
{
  if (_ack_in_kernel)
    {
      unsigned irqnum = id().irq();
      Pic::enable(irqnum);
    }
}


extern "C" void timer_interrupt(void);
extern "C" void timer_interrupt_slow(void);

void
set_timer_vector_run(void)
{
  if (Config::esc_hack || Config::serial_esc==Config::SERIAL_ESC_NOIRQ || 
      Config::watchdog)
    // set timer interrupt to (slower) debugging version
    Idt::set_vector (Config::scheduling_using_pit ? 0x20 : 0x28,
                    (unsigned) timer_interrupt_slow, false);
  else
    // set timer interrupt to non-debugging version
    Idt::set_vector (Config::scheduling_using_pit ? 0x20 : 0x28,
                    (unsigned) timer_interrupt, false);
}

extern "C" void timer_interrupt_stop(void);

void
set_timer_vector_stop(void)
{
  // acknowledge timer interrupt once to keep timer interrupt alive because
  // we could be called from thread_timer_interrupt_slow() before ack
  Timer::acknowledge();

  // set timer interrupt to dummy doing nothing
  Idt::set_vector (Config::scheduling_using_pit ? 0x20 : 0x28,
                  (unsigned) timer_interrupt_stop, false);
}
