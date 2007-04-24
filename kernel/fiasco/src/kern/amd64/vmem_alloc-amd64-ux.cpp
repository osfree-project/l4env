IMPLEMENTATION[amd64]:

#include <cassert>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "mapped_alloc.h"
#include "mem_layout.h"
#include "mem_unit.h"
#include "paging.h"
#include "static_init.h"
#include "initcalls.h"
#include "space.h"

IMPLEMENT FIASCO_INIT
void
Vmem_alloc::init()
{}

IMPLEMENT
void*
Vmem_alloc::page_alloc (void *address, Zero_fill zf, unsigned mode)
{
  void *vpage = 0;
  Unsigned64 attr = Pd_entry::Valid | Pd_entry::Referenced;

  vpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
  if (EXPECT_FALSE(!vpage))
    return 0;

  if (mode & User)
    attr |= Page::USER_RW;
  else
    attr |= Page::KERN_RW;

  if (zf == ZERO_FILL)
    memset(vpage, 0, Config::PAGE_SIZE);
 
  Address page = Mem_layout::pmem_to_phys((Address)vpage);

  // insert page into master page table
  if (!Kmem::kdir->map_slow_page(page, (Address)address, 
				Mapped_allocator::allocator(), attr)) 
    goto error;

  page_map (address, 0, zf, page);

#warning !!! pa attributes ignored !!!

  // XXX ???
  /*
  if (pa != Page::USER_NO)   
    e->add_attr(Pt_entry::User);
  */
  return address;

error:
  Mapped_allocator::allocator()->free(Config::PAGE_SHIFT, vpage); // 2^0 = 1 page
  return 0;
}

IMPLEMENT inline NEEDS ["paging.h", "mem_layout.h", "mem_unit.h"]
void *
Vmem_alloc::page_unmap (void *page)
{
  Address phys = Kmem::virt_to_phys(page);

  if (phys == (Address) -1)
    return 0;
  
  Address va = reinterpret_cast<Address>(page);
  void *res = (void*)Mem_layout::phys_to_pmem(phys);

  if (va < Mem_layout::Vmem_end)
    {
      // clean out pgdir entry
      *(Kmem::kdir->lookup(va)
	  ->pdp()->lookup(va)
	    ->pdir()->lookup(va)
	      ->ptab()->lookup(va)) = 0;
      page_unmap (page, 0);
      Mem_unit::tlb_flush(va);
    }

  return res;
}


IMPLEMENT inline NEEDS ["paging.h", "config.h", "kmem.h", "mapped_alloc.h",
			"mem_unit.h"]
void
Vmem_alloc::page_free (void *page)
{
  Address phys = Kmem::virt_to_phys(page);

  if (phys == (Address) -1)
    return;

  // convert back to virt (do not use "page") to get canonic mapping
  Mapped_allocator::allocator()->free(Config::PAGE_SHIFT, page); // 2^0=1 pages

  Address va = reinterpret_cast<Address>(page);

  if (va < Mem_layout::Vmem_end)
    {
      // clean out pgdir entry
      *(Kmem::kdir->lookup(va)
	  ->pdp()->lookup(va)
	    ->pdir()->lookup(va)
	      ->ptab()->lookup(va)) = 0;
      page_unmap (page, 0);
      Mem_unit::tlb_flush(va);
    }
}


