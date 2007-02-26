/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include "types.h"
#include "std_macros.h"

namespace Proc {

  typedef Unsigned32 Status;

  FIASCO_INLINE
  void stack_pointer( Mword sp )
  {
    asm volatile ( "movl %0, %%esp \n" : : "r"(sp) );
  }

  FIASCO_INLINE
  void pause()
  {
    asm volatile (" .byte 0xf3, 0x90 #pause \n" ); 
  }

  /*
   * The following simple ASM statements need the clobbering to work around
   * a bug in (at least) gcc-3.2.x up to x == 1. The bug was fixed on
   * Jan 9th 2003 (see gcc-bugs #9242 and #8832), so a released gcc-3.2.2
   * won't have it. It's safe to take the clobber statements out after
   * some time (e.g. when gcc-3.3 is used as a standard compiler).
   */

  FIASCO_INLINE
  void halt()
  {
    asm volatile (" hlt" : : : "memory");
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
    Status ret;
    asm volatile ("pushfl	\n\t"
		  "popl %0	\n\t"
		  "cli		\n\t"
		  : "=g"(ret) : /* no input */ : "memory");
    return ret;
  }
  
  FIASCO_INLINE
  void sti_restore( Status st )
  {
    if (st & 0x0200)
      asm volatile ("sti" : : : "memory");
  }

  FIASCO_INLINE
  Status interrupts()
  {
    Status ret;
    asm volatile ("pushfl         \n"
		  "popl %0        \n"
		  : "=g"(ret) : /* no input */ : "memory");
    return ret & 0x0200;
  }

  FIASCO_INLINE
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }
};

#endif // PROCESSOR_ARCH_H__
