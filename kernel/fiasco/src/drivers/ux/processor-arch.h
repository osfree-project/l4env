/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include <time.h>

#include "types.h"
#include "std_macros.h"

namespace Proc {

  typedef Unsigned32 Status;

  extern Status volatile virtual_processor_state;

  inline
  void stack_pointer( Mword sp )
  {
    asm volatile ( "movl %0, %%esp \n" : : "r"(sp) );
  }

  inline
  Status processor_state()
  {
    return virtual_processor_state;
  }

  inline
  void ux_set_virtual_processor_state( Status s )
  {
    virtual_processor_state = s;
  }

  inline
  void pause()
  {
    asm volatile (" .byte 0xf3, 0x90 #pause \n" ); 
  }

  inline
  void halt()
  {
    static struct timespec idle;
    idle.tv_sec  = 10;
    idle.tv_nsec = 0;
    nanosleep (&idle, NULL);
  }

  inline
  void cli()
  {
    asm volatile ("cli" : : : "memory");
  }

  inline
  void sti()
  {
    asm volatile ("sti" : : : "memory");
  }
  
  inline
  Status cli_save()
  {
    Status ret = virtual_processor_state;
    Proc::cli();
    return ret;
  }

  inline
  void sti_restore( Status st )
  {
    if (st & 0x200)
      Proc::sti();
  }

  inline
  Status interrupts()
  {
    return (virtual_processor_state & 0x0200);
  }

  inline
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }

};

#endif // PROCESSOR_ARCH_H__
