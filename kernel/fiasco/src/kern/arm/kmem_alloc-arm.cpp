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

#include "mem_unit.h"
#include "kmem_space.h"
#include "pagetable.h"
#include "ram_quota.h"

PRIVATE //inline
bool
Kmem_alloc::map_pmem(unsigned long phy, unsigned long size)
{
  static unsigned long next_map = Mem_layout::Map_base + (4 << 20);
  phy = Mem_layout::trunc_superpage(phy);
  size = Mem_layout::round_superpage(size);

  if (next_map + size > Mem_layout::Map_end)
    return false;

  for (unsigned long i = 0; i <size; i+=Config::SUPERPAGE_SIZE)
    {
      Pte pte = Kmem_space::kdir()->walk((char*)next_map+i,
	  Config::SUPERPAGE_SIZE, false, Ram_quota::root);
      pte.set(phy+i, Config::SUPERPAGE_SIZE, 
	  Mem_page_attr(Page::KERN_RW | Page::CACHEABLE),
	  true);

    }
  Mem_layout::add_pmem(phy, next_map, size);
  next_map += size;
  return true;
}

IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  Mword kmem_size = 8*1024*1024;
  Mword alloc_size = kmem_size;
  a->init(Mem_layout::Map_base);

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
	  //printf("ALLOC1: [%08lx; %08lx]\n", r.start, r.end);
	  if (Mem_layout::phys_to_pmem(r.start) == ~0UL)
	    if (!map_pmem(r.start, size))
	      panic("could not map physical memory %p\n", (void*)r.start);
	  a->add_mem((void*)Mem_layout::phys_to_pmem(r.start), size);
	  alloc_size -= size;
	}
      else
	{
	  r.start += (size - alloc_size);
	  Kip::k()->add_mem_region(Mem_desc(r.start, r.end,
		                                Mem_desc::Reserved));
	  //printf("ALLOC2: [%08lx; %08lx]\n", r.start, r.end);
	  if (Mem_layout::phys_to_pmem(r.start) == ~0UL)
	    if (!map_pmem(r.start, alloc_size))
	      panic("could not map physical memory %p\n", (void*)r.start);
	  a->add_mem((void*)Mem_layout::phys_to_pmem(r.start), alloc_size);
	  alloc_size = 0;
	}
    }
}
