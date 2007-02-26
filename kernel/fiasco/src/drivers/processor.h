/* -*- c++ -*- */

#ifndef PROCESSOR_H__
#define PROCESSOR_H__

#include "processor-arch.h"

/// Central processor specific methods.
namespace Proc {
  // Status is defined in arch part

  /** 
   * The following functions are implemented 
   * in the architecture specific parts (see
   * "<arch>/processor-arch.h").
   */
  //@{

  /// Block external interrupts
  void cli();

  /// Unblock external inetrrupts
  void sti();
  
  /// Are external interrupts enabled ?
  Status interrupts();

  /// Block external interrupts and save the old state
  Status cli_save();

  /// Conditionally unblock ext. interrupts
  void   sti_restore( Status );

  void pause();

  void halt();

  void irq_chance();

  void stack_pointer( Mword sp );
  //@}
};


#endif // PROCESSOR_H__
