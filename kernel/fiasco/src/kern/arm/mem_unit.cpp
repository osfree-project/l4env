INTERFACE:

class Mem_unit
{
public:
  static void write_back_data_cache();
  static void write_back_data_cache( void * );
  static void tlb_flush();
  static void dtlb_flush( void* va );
};

IMPLEMENTATION:

#include "kmem.h"
#include "types.h"

IMPLEMENT inline NEEDS["kmem.h","types.h"]
void Mem_unit::write_back_data_cache()
{
  volatile Unsigned32 *base = (Unsigned32*)Kmem::CACHE_FLUSH_AREA;
  Unsigned32 *const end = (Unsigned32*)(Kmem::CACHE_FLUSH_AREA + 8192);
  Unsigned32 dummy;
  do {
    dummy = *base;  
    base+=8;
  } while(base!=end) ;
}


IMPLEMENT inline
void Mem_unit::write_back_data_cache( void *addr )
{
  asm volatile ( "mcr p15, 0, %0, c7, c10, 1 \n"
		 :
		 : "r"(addr)
	       );
}

IMPLEMENT inline
void Mem_unit::tlb_flush()
{
  asm volatile ("mcr p15, 0, r0, c8, c7, 0x00 \n" : : : "memory" ); // TLB flush
}

IMPLEMENT inline
void Mem_unit::dtlb_flush( void* va )
{
  asm volatile ("mcr p15, 0, %0, c8, c6, 0x01 \n" : : "r"(va) : "memory" ); // TLB flush
}
