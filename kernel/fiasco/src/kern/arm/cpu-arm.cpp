INTERFACE:

class Cpu
{
public:
  static void early_init();
  static void init();

};

IMPLEMENTATION[arm]:

#include <cstdio>
#include <cstring>
#include <panic.h>

#include "types.h"
#include "pagetable.h"
#include "kmem_alloc.h"
#include "kmem_space.h"
#include "mem_unit.h"

//STATIC_INITIALIZER_P(init_cpu_1, CPU_INIT_PRIO);

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
		 : "r"(0x0000397f), "I"(0x0d3)
		 : "r2","r3"
		 );
		
}



// run after Kmem_alloc is up
//STATIC_INITIALIZER_P(init_cpu_2, CPU_INIT_2_PRIO);

IMPLEMENT
void Cpu::init()
{
  extern char ivt_start;

#if 0
  Mapped_allocator * const alloc = Kmem_alloc::allocator();
  
  void *cpu_page = alloc->alloc(4096);
  if(!cpu_page)
    panic("FATAL: Allocation of cpu page faild!");

  printf("Cpu: init 2 allocated cpu_page @ %p\n",cpu_page);

  memset( cpu_page, 0, 4096 );

  // must have the ivt at the beginning of the cpu page

#endif

  // map the interrupt vector table to 0xffff0000
  if(Kmem_space::kdir()->insert( &ivt_start, 
				 (void*)Kmem_space::IVT_BASE, 4096 )
     !=Page_table::E_OK) {

    panic("FATAL: Error mapping cpu page to %p\n",(void*)Kmem_space::IVT_BASE);

  }

  // make sure that the pt entry is in memory,
  // coz we are on the active page table
  Mem_unit::write_back_data_cache();
    
}


