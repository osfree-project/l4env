INTERFACE:

/**
 * Encapsulation of the platforms interrupt controller
 */
class Pic
{
public:
  /**
   * The type holding the saved Pic state.
   */
  typedef unsigned Status;

  /**
   * Static initalization of the interrupt controller
   */
  static void init();

  /**
   * Disable the given irq.
   */
  static void disable( unsigned irqnum );

  /**
   * Enable the given irq.
   */
  static void enable(unsigned irqnum, unsigned prio=0);

  /**
   * Acknowledge the given IRQ. 
   */
  static void acknowledge( unsigned irq );
  
  /**
   * Block the given IRQ til the next ACK. 
   */
  static void block(unsigned irq);

  /**
   * Disable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void disable_locked( unsigned irqnum );

  /**
   * Enable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void enable_locked(unsigned irqnum, unsigned prio = 0);

  /**
   * Temporarily block the IRQ til the next ACK. 
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void block_locked(unsigned irqnum);

  /**
   * Disable all IRQ's and and return the old Pic state.
   * @pre Must be done with disabled interrupts. 
   */
  static Status disable_all_save();

  /**
   * Restore the IRQ's to the saved state s.
   * @pre Must be done with disabled interrupts.
   * @param s, the saved state.
   */
  static void restore_all( Status s );

  /**
   * Acknowledge the given IRQ.
   * @pre The Cpu_lock must be locked (cli'ed).
   * @param irq, the irq to acknowledge.
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void acknowledge_locked( unsigned irq );
};


IMPLEMENTATION:

#include "processor.h"

// 
// Save implementations of disable, enable, and acknowledge.
// Do first a cli_save and then a sti_restore.
//

IMPLEMENT inline NEEDS["processor.h"]
void Pic::disable( unsigned irqnum )
{
  Proc::Status s = Proc::cli_save();
  disable_locked( irqnum );
  Proc::sti_restore(s);
}

IMPLEMENT inline NEEDS["processor.h"]
void Pic::enable(unsigned irqnum, unsigned prio)
{
  Proc::Status s = Proc::cli_save();
  enable_locked(irqnum, prio);
  Proc::sti_restore(s);
}

IMPLEMENT inline NEEDS["processor.h"]
void Pic::acknowledge( unsigned irq )
{
  Proc::Status s = Proc::cli_save();
  acknowledge_locked(irq);
  Proc::sti_restore(s);
}

IMPLEMENT inline NEEDS["processor.h"]
void Pic::block(unsigned irq)
{
  Proc::Status s = Proc::cli_save();
  block_locked(irq);
  Proc::sti_restore(s);
}

