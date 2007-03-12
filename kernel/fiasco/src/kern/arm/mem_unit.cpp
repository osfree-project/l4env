INTERFACE [arm]:

#include "kmem.h"
#include "mmu.h"

class Mem_unit : public Mmu< Kmem::Cache_flush_area >
{
public:
  static void tlb_flush();
  static void dtlb_flush( void* va );
  static void dtlb_flush();
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

IMPLEMENT inline
void Mem_unit::tlb_flush()
{
  asm volatile (
      "mcr p15, 0, r0, c8, c7, 0x00 \n" 
      : 
      : 
      : "memory" ); // TLB flush
}

PUBLIC static inline
void Mem_unit::tlb_flush( void* va )
{
  asm volatile (
      "mcr p15, 0, %0, c8, c7, 0x01 \n" 
      : 
      : "r"((unsigned long)va & 0xfffff000) 
      : "memory" ); // TLB flush
}

IMPLEMENT inline
void Mem_unit::dtlb_flush( void* va )
{
  asm volatile (
      "mcr p15, 0, %0, c8, c6, 0x01 \n" 
      : 
      : "r"((unsigned long)va & 0xfffff000) 
      : "memory" ); // TLB flush
}

IMPLEMENT inline
void Mem_unit::dtlb_flush()
{
  asm volatile (
      "mcr p15, 0, %0, c8, c6, 0x0 \n" 
      : 
      : "r"(0) 
      : "memory" ); // TLB flush
}

