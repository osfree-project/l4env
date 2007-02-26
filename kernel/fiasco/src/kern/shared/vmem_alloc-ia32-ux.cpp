INTERFACE:

EXTENSION class Vmem_alloc
{
private:
  template< typename _Ty >
  static _Ty *phys_to_virt( P_ptr<_Ty> );

  template< typename _Ty >
  static P_ptr<_Ty> linear_virt_to_phys( _Ty* );

  static P_ptr<void> zero_page;
};


IMPLEMENTATION[ia32-ux]:

#include <flux/x86/paging.h> // pd_entry_t
#include <cassert>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "static_init.h"
#include "initcalls.h"

// this static initializer must have a higer priotity than Vmem_alloc::init()
P_ptr<void> Vmem_alloc::zero_page INIT_PRIORITY(MAX_INIT_PRIO);

IMPLEMENT
template< typename _Ty > 
inline
_Ty *Vmem_alloc::phys_to_virt( P_ptr<_Ty> p )
{
  return (_Ty*)(p.get_unsigned() + Kmem::mem_phys);
}

IMPLEMENT
template< typename _Ty > 
inline
P_ptr<_Ty> Vmem_alloc::linear_virt_to_phys( _Ty *v )
{
  return P_ptr<_Ty>((((vm_offset_t)v) - Kmem::mem_phys));
}

IMPLEMENT FIASCO_INIT
void
Vmem_alloc::init()
{
  // Allocate a generic zero page
  void *zpage;
  zpage = Kmem_alloc::allocator()->alloc(0); // 2^0 = 1 pages
  zero_page = Kmem_alloc::allocator()->virt_to_phys(zpage);
  memset (zpage, 0, Config::PAGE_SIZE);
}

IMPLEMENT
void *Vmem_alloc::page_alloc( void *address, int order, Zero_fill zf,
			      Page::Attribs pa )
{
  void *vpage = 0;
  
  if (zf != ZERO_MAP)
    {
      vpage = Kmem_alloc::allocator()->alloc(0);
      if (!vpage)
        {
          kdb_ke("page_alloc: can't alloc new page");
          return 0;
        }
    }

 
  // insert page into master page table
  pd_entry_t *p = Kmem::kdir + (((vm_offset_t)address >> PDESHIFT) & PDEMASK);
  pt_entry_t *e;

  if (! (*p & INTEL_PDE_VALID))
    {
      pt_entry_t *new_pt = (pd_entry_t*)Kmem_alloc::allocator()->alloc(0);
      if (!new_pt)
        {
          kdb_ke("page_alloc: can't alloc page table");
          goto error;
        }

      P_ptr<pt_entry_t> new_ppt = Kmem_alloc::allocator()->virt_to_phys(new_pt);

      memset(new_pt, 0, Config::PAGE_SIZE);
      *p = new_ppt.get_unsigned()
        | INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_REF
        | Kmem::pde_global();
      if (pa != Page::USER_NO)
        *p |= INTEL_PDE_USER;
      e = new_pt + (((vm_offset_t)address >> PTESHIFT) & PTEMASK);
    }
  else if ((*p & INTEL_PDE_SUPERPAGE)
           || (e = Kmem_alloc::allocator()->phys_to_virt(P_ptr<pt_entry_t>((pt_entry_t*)(*p & Config::PAGE_MASK)))
                   + (((vm_offset_t)address >> PTESHIFT) & PTEMASK),
               *e & INTEL_PTE_VALID))
    {
      kdb_ke("page_alloc: address already mapped");
      goto error;               // there's already mapped something there...
    }
     
  if (zf == ZERO_FILL)
    memset(vpage, 0, Config::PAGE_SIZE);

  if (zf == ZERO_MAP)
  {
    *e = zero_page.get_unsigned() |
         INTEL_PTE_VALID | INTEL_PTE_REF | Kmem::pde_global();
    page_map (address, order, zf, zero_page.get_unsigned());

  } else
  {
    P_ptr<void> page = Kmem_alloc::allocator()->virt_to_phys(vpage);

    *e = page.get_unsigned() | INTEL_PTE_WRITE | INTEL_PTE_MOD |
         INTEL_PTE_VALID | INTEL_PTE_REF | Kmem::pde_global();
    page_map (address, order, zf, page.get_unsigned());
  }

  if (pa != Page::USER_NO)   
    *e |= INTEL_PTE_USER;

  return address;

error:
  if (zf != ZERO_MAP)
    Kmem_alloc::allocator()->free(0, vpage); // 2^0 = 1 page
  return 0;

}

IMPLEMENT			   
inline NEEDS[<flux/x86/paging.h>,"config.h"]
void Vmem_alloc::page_free( void *page, int order )
{

// undef the f... oskit defines
#undef PAGE_MASK 

  vm_offset_t phys = Kmem::virt_to_phys(page);

  if (phys == 0xffffffff)
    return;

  // convert back to virt (do not use "page") to get canonic mapping
  if (phys != zero_page.get_unsigned())
    Kmem_alloc::allocator()->free(0, page); // 2^0=1 pages

  vm_offset_t va = reinterpret_cast<vm_offset_t>(page);

  if (va < Kmem::mem_phys)
    {
      // clean out pgdir entry
      (phys_to_virt(P_ptr<pd_entry_t>((pd_entry_t*)(
         Kmem::kdir[(va >> PDESHIFT) & PDEMASK] 
	 & Config::PAGE_MASK))))[(va >> PTESHIFT) & PTEMASK] = 0;

      page_unmap (page, order);

      Kmem::tlb_flush(va);
    }
}
