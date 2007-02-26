/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include "types.h"
#include "std_macros.h"

namespace Proc {
  typedef Mword Status;

  inline
  void stack_pointer( Mword sp )
  {
    asm volatile ( "mov sp, %0 \n" : : "r"(sp) );
  }

  inline
  void pause()
  {
  }

  inline
  void halt()
  {
  }

  inline
  void cli()
  {
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    orr    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : : : "r6" 
		   );
  }
  
  inline
  Status cli_save()
  {
    Status ret;
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    mov    %0, r6      \n"
		   "    orr    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : "=r"(ret) : : "r6" 
		   );
    return ret;
  }

  inline
  Status interrupts()
  {
    Status ret;
    asm volatile ("   mrs  %0, cpsr  \n"
		  : "=r"(ret)
		  );
    return !(ret & 128);
  }

  inline
  void sti()
  {
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    bic    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : : : "r6" 
		   );
  }
  
  inline
  void sti_restore( Status st )
  {
    asm volatile ( "    tst    %0, #128    \n"
		   "    bne    1f          \n"
                   "    mrs    r6, cpsr    \n"
		   "    bic    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
                   "1:                     \n"
		   : : "r"(st) : "r6" 
		   );
  }

  inline
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }

};

#endif // PROCESSOR_ARCH_H__
