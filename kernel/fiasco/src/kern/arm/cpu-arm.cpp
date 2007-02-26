INTERFACE [arm]:

class Cpu
{
public:
  static void early_init();
  static void init();

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cstdio>
#include <cstring>
#include <panic.h>

#include "types.h"
#include "pagetable.h"
#include "kmem_space.h"
#include "mem_unit.h"

IMPLEMENT
void Cpu::early_init()
{
  // switch to supervisor mode and intialize the memory system
  asm volatile ( " mov  r2, r13             \n"
		 " mov  r3, r14             \n"
		 " msr  cpsr_c, %1          \n" 
		 " mov  r13, r2             \n" 
		 " mov  r14, r3             \n"

		 " mcr  p15, 0, %0, c1, c0   \n"
		 
		 :
		 : "r"(0x0000317f), "I"(0x0d3)
		 : "r2","r3"
		 );
}

IMPLEMENT
void Cpu::init()
{
  extern char ivt_start;

  // map the interrupt vector table to 0xffff0000
  if(Kmem_space::kdir()->insert( &ivt_start, 
                                 (void*)Kmem_space::Ivt_base, 4096 )
     !=Page_table::E_OK) 
    {
      panic("FATAL: Error mapping cpu page to %p\n",(void*)Kmem_space::Ivt_base);
    }
}

PUBLIC static inline
bool
Cpu::have_superpages()
{
  return true;
}
