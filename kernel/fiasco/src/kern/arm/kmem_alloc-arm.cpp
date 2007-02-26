IMPLEMENTATION [arm]:

#include <cstdio>
#include <cassert>

#include "config.h"
#include "helping_lock.h"
#include "kip_init.h"
#include "kmem.h"
#include "panic.h"

PUBLIC
void Kmem_alloc::debug_dump()
{
  a->dump();
}

//----------------------------------------------------------------------------
IMPLEMENTATION [arm]:

IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  Mword kmem_size = 8*1024*1024;
  Mword alloc_size = kmem_size;

  
  a->init();

  for (;alloc_size > 0;)
    {
      Mem_region r = Kip::k()->last_free();
      if (r.start > r.end)
	  panic("Corrupt memory descscriptor in KIP...");
      
      if (r.start == r.end)
	{
	  panic("not enough kernel memory");
	}
      Mword size = r.end - r.start + 1;
      if (size <= alloc_size)
	{
	  Kip::k()->add_mem_region(Mem_desc(r.start, r.end, 
		                                Mem_desc::Reserved));
	  // printf("ALLOC: [%08x; %08x]\n", r.start, r.end);
	  a->free((void*)Mem_layout::phys_to_pmem(r.start), size);
	  alloc_size -= size;
	}
      else 
	{
	  r.start += (size - alloc_size);
	  Kip::k()->add_mem_region(Mem_desc(r.start, r.end, 
		                                Mem_desc::Reserved));
	  // printf("ALLOC: [%08x; %08x]\n", r.start, r.end);
	  a->free((void*)Mem_layout::phys_to_pmem(r.start), alloc_size);
	  alloc_size = 0;
	}
    }
}


