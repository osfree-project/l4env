INTERFACE[amd64]:

EXTENSION class Vmem_alloc
{
private:
  static P_ptr<void> zero_page;
};


IMPLEMENTATION[amd64]:

#include <cassert>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "mapped_alloc.h"
#include "mem_unit.h"
#include "paging.h"
#include "static_init.h"
#include "initcalls.h"
#include "space.h"

// this static initializer must have a higer priotity than Vmem_alloc::init()
P_ptr<void> Vmem_alloc::zero_page INIT_PRIORITY(MAX_INIT_PRIO);

IMPLEMENT FIASCO_INIT
void
Vmem_alloc::init()
{
  // Allocate a generic zero page
  void *zpage;
  zpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT); // 2^0 = 1 pages
  zero_page = P_ptr<void>(Mem_layout::pmem_to_phys((Address)zpage));
  memset (zpage, 0, Config::PAGE_SIZE);
}

IMPLEMENT
void*
Vmem_alloc::page_alloc (void *address, Zero_fill zf, Page::Attribs pa)
{
  void *vpage = 0;
  P_ptr<void> page;
  Unsigned64 attr = Pd_entry::Valid | Pd_entry::Referenced;

  if (zf != ZERO_MAP)
    {
      vpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
      if (!vpage)
        {
          kdb_ke("page_alloc: can't alloc new page");
          return 0;
        }
      attr |= Pd_entry::Writable;
    }
  if (zf == ZERO_FILL)
    memset(vpage, 0, Config::PAGE_SIZE);
 
  if (zf == ZERO_MAP)
    {
      page = zero_page; 
    }
  else
    page = P_ptr<void>(Mem_layout::pmem_to_phys((Address)vpage));

  if (pa | Mem_space::Page_user_accessible)
    attr |= Mem_space::Page_user_accessible;

  // insert page into master page table
  if (!Kmem::kdir->map_slow_page(page.get_unsigned(), (Address)address, 
				Mapped_allocator::allocator(), attr)) 
    goto error;

  page_map (address, 0, zf, page.get_unsigned());

#warning !!! pa attributes ignored !!!

  // XXX ???
  /*
  if (pa != Page::USER_NO)   
    e->add_attr(Pt_entry::User);
  */
  return address;

error:
  if (zf != ZERO_MAP)
    Mapped_allocator::allocator()->free(Config::PAGE_SHIFT, vpage); // 2^0 = 1 page
  return 0;
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
  if (phys != zero_page.get_unsigned())
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


IMPLEMENT
void*
Vmem_alloc::page_attr (void *address, Page::Attribs pa)
{
  Pd_entry *p = Kmem::kdir->lookup((Address)address)
    			->pdp()->lookup((Address)address)
			  ->pdir()->lookup((Address)address);
  Pt_entry *e;

  if (!p->valid() || p->superpage())
    return 0;

  e = p->ptab()->lookup((Address)address);
  if (!e->valid())
    return 0;

  e->del_attr(Page::MAX_ATTRIBS);
  e->add_attr(pa);
  return address;
}

