INTERFACE:

/**
 * @brief Encapsulation of the platforms interrupt controller
 */
class Pic
{
public:
  /**
   * @brief The type holding the saved Pic state.
   */
  typedef unsigned Status;

  /**
   * @brief Static initalization of the interrupt controller
   */
  static void init();

  /**
   * @brief Disable the given irq.
   */
  static void disable( unsigned irqnum );

  /**
   * @brief Enable the given irq.
   */
  static void enable ( unsigned irqnum );

  /**
   * @brief Acknowledge the given IRQ. 
   */
  static void acknowledge( unsigned irq );
  
  /**
   * @brief Disable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void disable_locked( unsigned irqnum );

  /**
   * @brief Enable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the 
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void enable_locked ( unsigned irqnum );

  /**
   * @brief Disable all IRQ's and and return the old Pic state.
   * @pre Must be done with disabled interrupts. 
   */
  static Status disable_all_save();

  /**
   * @brief Restore the IRQ's to the saved state s.
   * @pre Must be done with disabled interrupts.
   * @param s, the saved state.
   */
  static void restore_all( Status s );

  /**
   * @brief Acknowledge the given IRQ.
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
//#include "static_init.h"

//STATIC_INITIALIZE_P(Pic,PIC_INIT_PRIO);

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
void Pic::enable( unsigned irqnum )
{
  Proc::Status s = Proc::cli_save();
  enable_locked( irqnum );
  Proc::sti_restore(s);
}

IMPLEMENT inline NEEDS["processor.h"]
void Pic::acknowledge( unsigned irq )
{
  Proc::Status s = Proc::cli_save();
  acknowledge_locked(irq);
  Proc::sti_restore(s);
}
