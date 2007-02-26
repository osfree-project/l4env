/* -*- c++ -*- */

#ifndef PROCESSOR_ARCH_H__
#define PROCESSOR_ARCH_H__

#include "types.h"
#include "std_macros.h"

namespace Proc {
  typedef Mword Status;

  FIASCO_INLINE
  void stack_pointer( Mword sp )
  {
    asm volatile ( "mov sp, %0 \n" : : "r"(sp) );
  }

  FIASCO_INLINE
  void pause()
  {
  }

  FIASCO_INLINE
  void halt()
  {
  }


  FIASCO_INLINE
  void cli()
  {
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    orr    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : : : "r6" 
		   );
  }
  
  FIASCO_INLINE
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

  

  FIASCO_INLINE
  Status interrupts()
  {
    Status ret;
    asm volatile ("   mrs  %0, cpsr  \n"
		  : "=r"(ret)
		  );
    return !(ret & 128);
  }

  
  FIASCO_INLINE
  void sti()
  {
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    bic    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : : : "r6" 
		   );
  }
  
  FIASCO_INLINE
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

  FIASCO_INLINE
  void irq_chance()
  {
    asm volatile ("nop; nop;" : : : "memory");
  }

  
};

#endif // PROCESSOR_ARCH_H__
