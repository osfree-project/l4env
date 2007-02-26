INTERFACE:

#include "paging.h"


EXTENSION class Page_table // must be 16Kb alligned
{
private:

  Unsigned32 _raw[1024];


  enum {
    PDE_BASE_MASK = 0xfffff000,
    PDE_PRESENT   = 0x001,
    PDE_RW        = 0x002,
    PDE_USER      = 0x004,
    PDE_DIS_CACHE = 0x010,
    PDE_ACCESSED  = 0x020,
    PDE_DIRTY     = 0x040,
    PDE_SUPERPAGE = 0x080,
    PDE_GLOBAL    = 0x100,


    // SELF_INDEX       = 1022,
    PAGE_DIR_INDEX   = 0, // 1024 * 1022,
    //    PAGE_CACHE_INDEX = 0, //1024 * 1023,

    SUPER_PAGE_SIZE = (1U << 22),
    
  };


};



IMPLEMENTATION[arch]:

#if 0

#include <cassert>
#include <cstring>

#include "globals.h"
#include "panic.h"

#include "atomic.h"

#include <flux/x86/proc_reg.h>


//extern "C" Unsigned32   _page_table_location[];
//extern "C" Page_table   _page_table_cache;



PRIVATE static inline 
Unsigned32 Page_table::pd_index( void* address )
{
  return (Unsigned32)address >> 22;
}

PRIVATE static inline 
Unsigned32 Page_table::pt_index( void* address )
{
  return ((Unsigned32)address >> 12) & 0x3ff;
}




/**
 *
 *
 */
IMPLEMENT 
Page_table::Page_table()
{
  for( unsigned i = 0; i< 1024; ++i ) {
    _raw[i]=0;
  }
}

IMPLEMENT 
Page_table *Page_table::current()
{
//  extern Page_table _page_table_location;
//  return &_page_table_location;
	return 0;
}


PRIVATE inline NEEDS[<cassert>,"panic.h"]
/*		     Page_table::alloc_4k_page]*/
Page_table::Status Page_table::__insert( P_ptr<void> pa, void const *const va, 
				       size_t size, Page::Attribs a, 
				       bool replace)
{

}

PRIVATE inline
bool
Page_table::super_page_aligned(Unsigned32 addr)
{
  return !(addr & ((SUPER_PAGE_SIZE) - 1));
}



#include <cstdio>


/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param p_addr  Physical address (naturally aligned).
    @param v_addr Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB - (IA32 specific).
    @param p_attr Additional flags describing the behavior of the mapping,
              e.g. cacheable, read-only etc.

    @return E_OK if a new mapping was created;
             E_NOMEM if the mapping could not be inserted because
                              the kernel is out of memory;
             E_EXISTS if the mapping could not be inserted because
                               another mapping occupies the virtual-address
                               range
			       (use replace if existing mappings shall be
			       automatically upgraded).
    @pre phys and virt need to be size-aligned according to the size argument.
    @pre the page table is linearly mapped at <this>, so it is possible
         to used array arithmetic instead of the more general tree traversal
 */

IMPLEMENT
Page_table::Status Page_table::insert( P_ptr<void> p_addr, 
				       void *v_addr, size_t size, 
				       Page::Attribs p_attr )
{
#if 0
  assert(allocator);
  assert( !((p_addr.get_unsigned() | ((Unsigned32) v_addr)) & 0x0fff) );
  
  Unsigned32 &pde = _raw[ pd_index(v_addr) ];
  Unsigned32* pde_virt;

  if ( ! ( pde &  PDE_PRESENT)) {
    if (size == SUPER_PAGE_SIZE)	{
      assert(((p_addr.get_unsigned() | (Unsigned32)v_addr) & (SUPER_PAGE_SIZE-1)) == 0);

      pde = p_addr.get_unsigned() | 
	(p_attr &  PAGE_MAX_ATTRIBS)            | 
	PDE_PRESENT | PDE_ACCESSED | PDE_DIRTY  |
	PDE_SUPERPAGE;
	return E_OK;
    } else {
      assert(size == 4096);
      // not a superpage

      pde_virt = (Unsigned32*)allocator->alloc(4096);
      if(pde_virt==0)
	return E_NOMEM;

      memset(pde_virt,0,4096);

      P_ptr<void> pde_phys;
      pde_phys = allocator->virt_to_phys(pde_virt);

      pde = pde_phys.get_unsigned() | (p_attr &  PAGE_MAX_ATTRIBS)| 
	PDE_PRESENT | PDE_ACCESSED | PDE_DIRTY;


      // from now handle the same as if there already was a page dir entry
    }
  } else {
    if( size == SUPER_PAGE_SIZE || pde & PDE_SUPERPAGE )
      return E_EXISTS;

    pde_virt = allocator->phys_to_virt(P_ptr<Unsigned32>((Unsigned32*)(pde & PDE_BASE_MASK)));
    
    if (pde_virt[pt_index(v_addr)] & PDE_PRESENT)
      return E_EXISTS;
    
  }
  
  pde_virt[pt_index(v_addr)] = p_addr.get_unsigned() |
    (p_attr &  PAGE_MAX_ATTRIBS) | 
    PDE_PRESENT | PDE_ACCESSED | PDE_DIRTY;
#endif  
  return E_OK;
  
}

IMPLEMENT
Page_table::Status Page_table::replace( P_ptr<void>, void*, size_t, Page::Attribs )
{
}

IMPLEMENT
Page_table::Status Page_table::change( void*, Page_attribs )
{
}

IMPLEMENT
Page_table::Status Page_table::remove( void* )
{
}


IMPLEMENT
P_ptr<void> Page_table::lookup( void *address, size_t *s, Page::Attribs *a ) const
{
#if 0 

  Unsigned32 pde = _raw[pd_index(address)];
  if( pde & PDE_PRESENT ) {
    if( pde & PDE_SUPERPAGE ) {
      if(s) *s = SUPER_PAGE_SIZE;
      if(a) *a = pde & PAGE_MAX_ATTRIBS;
      return (void*)(pde & PDE_BASE_MASK);
    } else {
      Unsigned32 *pt = 
	allocator->phys_to_virt(P_ptr<Unsigned32>((Unsigned32*)(pde & PDE_BASE_MASK)));
      
      Unsigned32 pte = pt[pt_index(address)];
      if(pte & PDE_PRESENT) {
	if(s) *s = 4096;
	if(a) *a = pte & PAGE_MAX_ATTRIBS;
	return (void*)(pte & PDE_BASE_MASK);
      } else {
	return (void*)0; // error
      }
    }
  } else {
    return (void*)0;
  }
#endif
}


IMPLEMENT inline
size_t const Page_table::num_page_sizes()
{
  return 2;
}


IMPLEMENT inline
size_t const * const Page_table::page_sizes()
{
  static size_t const _page_sizes[] = {4096, 4096*1024};
  return _page_sizes;
}



PUBLIC /*inline*/
void Page_table::activate()
{
  P_ptr<Page_table> p = allocator->virt_to_phys(this);
  asm volatile( " mov %0, %%cr3 " : : "r"(p.get_unsigned()) );
}

IMPLEMENT
void Page_table::init()
{
  // kernel mode should acknowledge write-protected page table entries
  set_cr0(get_cr0() | CR0_WP);
}


IMPLEMENT
void Page_table::copy_in( void* my_base, Page_table *o, void* base, size_t size )
{
  (Unsigned32&)base &= ~(SUPER_PAGE_SIZE -1);
  (Unsigned32&)my_base &= ~(SUPER_PAGE_SIZE -1);
  unsigned idx = pd_index(base);
  unsigned my_idx =  pd_index(my_base);
  if(size == 0) size = SUPER_PAGE_SIZE;
  for(; idx < pd_index((char*)base + size); ++idx,++my_idx) {
    _raw[my_idx] = o->_raw[idx];
  }
}

#endif
