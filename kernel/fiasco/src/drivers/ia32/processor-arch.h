/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include "types.h"
#include "std_macros.h"

namespace Proc {

  typedef Unsigned32 Status;

  inline
  void stack_pointer( Mword sp )
  {
    asm volatile ("movl %0, %%esp \n" : : "r"(sp) );
  }

  inline
  Mword program_counter ()
  {
    Mword pc;
    asm volatile ("call 1f ; 1: pop %0" : "=rm"(pc));
    return pc;
  }

  inline
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

  inline
  void halt()
  {
    asm volatile (" hlt" : : : "memory");
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
    Status ret;
    asm volatile ("pushfl	\n\t"
		  "popl %0	\n\t"
		  "cli		\n\t"
		  : "=g"(ret) : /* no input */ : "memory");
    return ret;
  }
  
  inline
  void sti_restore( Status st )
  {
    if (st & 0x0200)
      asm volatile ("sti" : : : "memory");
  }

  inline
  Status interrupts()
  {
    Status ret;
    asm volatile ("pushfl         \n"
		  "popl %0        \n"
		  : "=g"(ret) : /* no input */ : "memory");
    return ret & 0x0200;
  }

  inline
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }

};

#endif // PROCESSOR_ARCH_H__
