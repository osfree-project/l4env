INTERFACE[ia32,ux]:

EXTENSION class Vmem_alloc
{
private:
  static P_ptr<void> zero_page;
};


IMPLEMENTATION[ia32,ux]:

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

  if (zf != ZERO_MAP)
    {
      vpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
      if (!vpage)
        {
          kdb_ke("page_alloc: can't alloc new page");
          return 0;
        }
    }
 
  // insert page into master page table
  Pd_entry *p = Kmem::kdir->lookup((Address)address);
  Pt_entry *e;

  if (! p->valid())
    {
      Ptab *new_pt = (Ptab*)Mapped_allocator::allocator()
	->alloc(Config::PAGE_SHIFT);
      if (!new_pt)
        {
          kdb_ke("page_alloc: can't alloc page table");
          goto error;
        }

      new_pt->clear();

      *p = Kmem::virt_to_phys(new_pt) | Pd_entry::Valid | Pd_entry::Writable
				      | Pd_entry::Referenced
				      | Pd_entry::global();
      if (pa != Page::USER_NO)
	p->add_attr(Pd_entry::User);

      e = new_pt->lookup((Address)address);
    }
  else if ((p->superpage()) 
	   || (e = p->ptab()->lookup((Address)address), e->valid()))
    {
      kdb_ke("page_alloc: address already mapped");
      goto error;
    }
     
  if (zf == ZERO_FILL)
    memset(vpage, 0, Config::PAGE_SIZE);

  if (zf == ZERO_MAP)
    {
      *e = zero_page.get_unsigned() 
	 | Pt_entry::Valid | Pt_entry::Referenced | Pt_entry::global();
      page_map (address, 0, zf, zero_page.get_unsigned());
    }
  else
    {
      P_ptr<void> page = P_ptr<void>(Mem_layout::pmem_to_phys((Address)vpage));

      *e = page.get_unsigned() | Pt_entry::Writable | Pt_entry::Dirty 
	 | Pt_entry::Valid | Pt_entry::Referenced | Pt_entry::global();
      page_map (address, 0, zf, page.get_unsigned());
    }

  if (pa != Page::USER_NO)   
    e->add_attr(Pt_entry::User);

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
      *(Kmem::kdir->lookup(va)->ptab()->lookup(va)) = 0;
      page_unmap (page, 0);
      Mem_unit::tlb_flush(va);
    }
}


IMPLEMENT
void*
Vmem_alloc::page_attr (void *address, Page::Attribs pa)
{
  Pd_entry *p = Kmem::kdir->lookup((Address)address);
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

/** Allocate size pages of kernel memory and map it sequentially
    to the given address space, starting at  virt_addr.
    @param space Target address space.
    @param virt_addr Virtual start address.
    @param size Size in pages.
    @param page_attributes Page table attributes per allocated page.
    @see local_free()
 */
PUBLIC static
bool
Vmem_alloc::local_alloc(Space *space,
		        Address virt_addr, 
			int size,
		//	char fill,  
			unsigned int page_attributes)
{
  assert((virt_addr & (Config::PAGE_SIZE -1)) == 0);
  (void)page_attributes;
  
  Address va = virt_addr;
  int i;

  for (i=0; i<size; i++) 
    {
      void *page = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);

      if(!page)
	break;
    
      check(space->v_insert(Kmem::virt_to_phys(page), va, Config::PAGE_SIZE, 
			    page_attributes) == Space::Insert_ok);

      memset(page, 0, Config::PAGE_SIZE);

      va += Config::PAGE_SIZE;
    }
  
  if(i == size) 
    return true;

  /* kdb_ke ("Vmem_alloc::local_alloc() failed");  */
  /* cleanup */
  local_free(space, virt_addr, i);

  return false;
}


/** Free the sequentially mapped memory in the given address space,
    starting at  virt_addr.
    The page table in the address space is not deleted. 
    @param space Target address space.
    @param virt_addr Virtual start address.
    @param size Size in pages.
    @see local_alloc()
 */
PUBLIC static
void 
Vmem_alloc::local_free(Space *space, Address virt_addr, int size)
{
  assert((virt_addr & (Config::PAGE_SIZE -1)) == 0);

  Address va = virt_addr;
  
  for (int i=0; i<size; i++)
    {
      Address phys; 
      if (!space->v_lookup (va, &phys))
        kdb_ke ("not mapped");

      space->v_delete (va, Config::PAGE_SIZE); 
      Mapped_allocator::allocator()
	->free(Config::PAGE_SHIFT, Kmem::phys_to_virt (phys));
      va += Config::PAGE_SIZE;
    }
}
