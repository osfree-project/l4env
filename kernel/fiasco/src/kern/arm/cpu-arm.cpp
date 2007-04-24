INTERFACE [arm]:

#include "types.h"

class Cpu
{
public:
  static void early_init();
  static void init();

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

enum { Cp15_c1 = 0x327f };


//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6]:

enum { Cp15_c1 = 0x00803007 };

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cstdio>
#include <cstring>
#include <panic.h>

#include "pagetable.h"
#include "kmem_space.h"
#include "mem_unit.h"
#include "ram_quota.h"

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
		 : 
		 "r"(Cp15_c1),
		 //"r"(0x00002173/*disabled cache*/),
		 "I"(0x0d3)
		 : "r2","r3"
		 );
}


PUBLIC static inline
bool
Cpu::have_superpages()
{ return true; }

PUBLIC static inline
void
Cpu::debugctl_enable()
{}

PUBLIC static inline
void
Cpu::debugctl_disable()
{}

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{ return 0; }

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_tsc_to_us()
{ return 0; }

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_ns_to_tsc()
{ return 0; }

PUBLIC static inline
bool
Cpu::have_tsc()
{ return 0; }

PUBLIC static inline
Unsigned64
Cpu::rdtsc (void)
{ return 0; }

IMPLEMENT
void Cpu::init()
{
  extern char ivt_start;
  // map the interrupt vector table to 0xffff0000
  Pte pte = Kmem_space::kdir()->walk((void*)Kmem_space::Ivt_base, 4096,
      true, Ram_quota::root);

  pte.set((unsigned long)&ivt_start, 4096, 
      Mem_page_attr(Page::KERN_RW | Page::CACHEABLE),
      true);

  Mem_unit::tlb_flush();
}


PUBLIC inline static
void
Cpu::memcpy_mwords( void *dst, void const *src, unsigned words)
{
  __builtin_memcpy(dst, src, words * sizeof(unsigned long));
}
