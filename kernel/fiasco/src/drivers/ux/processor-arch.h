/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include <time.h>

#include "types.h"
#include "std_macros.h"

namespace Proc {

  typedef Unsigned32 Status;

  extern Status volatile virtual_processor_state;

  FIASCO_INLINE
  void stack_pointer( Mword sp )
  {
    asm volatile ( "movl %0, %%esp \n" : : "r"(sp) );
  }

  FIASCO_INLINE
  Status processor_state()
  {
    return virtual_processor_state;
  }

  FIASCO_INLINE
  void ux_set_virtual_processor_state( Status s )
  {
    virtual_processor_state = s;
  }

  FIASCO_INLINE
  void pause()
  {
    asm volatile (" .byte 0xf3, 0x90 #pause \n" ); 
  }

  FIASCO_INLINE
  void halt()
  {
    static struct timespec idle = {10, 0};
    nanosleep (&idle, NULL);
  }

  FIASCO_INLINE
  void cli()
  {
    asm volatile ("cli" : : : "memory");
  }

  FIASCO_INLINE
  void sti()
  {
    asm volatile ("sti" : : : "memory");
  }
  
  FIASCO_INLINE
  Status cli_save()
  {
    Status ret = virtual_processor_state;
    Proc::cli();
    return ret;
  }

  FIASCO_INLINE
  void sti_restore( Status st )
  {
    if (st & 0x200)
      Proc::sti();
  }

  FIASCO_INLINE
  Status interrupts()
  {
    return (virtual_processor_state & 0x0200);
  }

  FIASCO_INLINE
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }


};

#endif // PROCESSOR_ARCH_H__
