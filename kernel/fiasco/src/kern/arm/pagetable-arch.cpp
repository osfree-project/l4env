INTERFACE [arm]:

#include "paging.h"

EXTENSION class Page_table
{
private:
  Mword raw[4096];
  
  enum {
    PDE_PRESENT      = 0x03,
    PDE_TYPE_MASK    = 0x03,
    PDE_TYPE_SECTION = 0x02,
    PDE_TYPE_COARSE  = 0x01,
    PDE_TYPE_FINE    = 0x03,
    PDE_TYPE_FREE    = 0x00,

    PDE_DOMAIN_MASK  = 0x01e0,
    PDE_DOMAIN_SHIFT = 5,

    PDE_AP_MASK      = 0x0c00,

    // already mask the lower 12 bits,
    // coz always have a 4kb second level 
    // that spans 4 pd entries
    PT_BASE_MASK     = 0xfffffc00, 

    PTE_PRESENT      = 0x03,
    PTE_TYPE_MASK    = 0x03,
    PTE_TYPE_LARGE   = 0x01,
    PTE_TYPE_SMALL   = 0x02,
    PTE_TYPE_TINY    = 0x03,
    PTE_TYPE_FREE    = 0x00,

    PAGE_BASE_MASK   = 0xfffff000,
  };

private:
  static Page_table *_current;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>

#include "mem_unit.h"
#include "kdb_ke.h"

Page_table *Page_table::_current;

//#include "panic.h"

IMPLEMENT
void * Page_table::operator new( size_t s )
{
  assert(s == 16*1024);
  return alloc()->alloc(14); // 2^14 = 16K
}


IMPLEMENT
void Page_table::operator delete( void *b )
{
  alloc()->free(14, b);
}


IMPLEMENT
Page_table::Page_table()
{
  for( unsigned i = 0; i< 4096; ++i ) 
    raw[i]=0;
}

PUBLIC
void Page_table::free_page_tables(void *start, void *end)
{
  
  for (unsigned i = (Address)start >> 20; i < ((Address)end >> 20); ++i)
    {
      if (pde_coarse(raw + i))
	{
	  void *pt = (void*)Mem_layout::phys_to_pmem(raw[i] & PT_BASE_MASK);
	  alloc()->free(10, pt); 
	}
    }
}


PRIVATE inline
static unsigned Page_table::pd_index( void const *const address )
{
  return (Mword)address >> 20; // 1MB steps
}

PRIVATE inline
static unsigned Page_table::pt_index( void const *const address )
{
  return ((Mword)address >> 12) & 255; // 4KB steps for coarse pts
}

PRIVATE static inline
Mword Page_table::pde_valid(Mword const *pde)
{ return *pde & PDE_PRESENT; }

PRIVATE static inline
Mword Page_table::pte_valid(Mword const *pte)
{ return *pte & PTE_PRESENT; }

PRIVATE static inline
Mword Page_table::pde_section(Mword const *pde)
{ return (*pde & PDE_TYPE_MASK) == PDE_TYPE_SECTION; }

PRIVATE static inline
Mword Page_table::pde_coarse(Mword const *pde)
{ return (*pde & PDE_TYPE_MASK) == PDE_TYPE_COARSE; }

PRIVATE static inline
Mword Page_table::pte_small(Mword const *pte)
{ return (*pte & PTE_TYPE_MASK) == PTE_TYPE_SMALL; }

PRIVATE static inline
Mword Page_table::pte_large(Mword const *pte)
{ return (*pte & PTE_TYPE_MASK) == PTE_TYPE_LARGE; }

PRIVATE static inline
Mword Page_table::pte_tiny(Mword const *pte)
{ return (*pte & PTE_TYPE_MASK) == PTE_TYPE_TINY; }

PRIVATE static inline
Mword Page_table::pte_type(Mword const *pte)
{ return *pte & PTE_TYPE_MASK; }

PRIVATE static inline
Mword Page_table::pde_type(Mword const *pde)
{ return *pde & PDE_TYPE_MASK; }


IMPLEMENT inline
size_t const * const Page_table::page_sizes()
{
  static size_t const _page_sizes[] = {4 *1024, 1024 *1024};
  return _page_sizes;
}

IMPLEMENT inline
size_t const * const Page_table::page_shifts()
{
  static size_t const _page_shifts[] = {12, 20};
  return _page_shifts;
}


PRIVATE inline
bool Page_table::kb64_present( Mword *pt, unsigned pt_idx )
{
  for(unsigned i = 0; i<16; ++i)
    if(pt[(pt_idx & 0x3f0) + i] & PTE_PRESENT) 
      return true;

  return false;
}

PRIVATE static inline
void 
Page_table::set_pte64kb(Mword *pte, Mword value, bool wb, void *va)
{
  for(Mword *p = (Mword*)((Address)pte & ~0x0f);
      p != (Mword*)(((Address)pte & ~0x0f) + 0x10); p++) 
    *p = value;


  if (wb)
    {
      Mem_unit::clean_dcache((Mword*)((Address)pte & ~0x0f), 
	                     (Mword*)(((Address)pte & ~0x0f) + 0x10));
      if (va)
	Mem_unit::dtlb_flush(va);
    }
}

PRIVATE static inline
void 
Page_table::set_pte(Mword *pte, Mword value, bool wb, void *va)
{
  *pte = value; 
  if (wb)
    {
      Mem_unit::clean_dcache(pte, pte);
      if (va)
	Mem_unit::dtlb_flush(va);
    }
}
  

IMPLEMENT
Page_table::Status Page_table::change(void *va, Page::Attribs a, 
                                      bool force_flush)
{ 
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);

  if (!pde_valid(raw + pd_idx)) 
    return E_INVALID;

  if (pde_section(raw + pd_idx)) 
    {
      if (force_flush || _current==this)
	Mem_unit::flush_cache(va, (char*)va + 0x100000);
      
      set_pte(raw + pt_idx, (raw[pd_idx] & ~PDE_AP_MASK) | (a & PDE_AP_MASK), 
	    force_flush || _current==this, va);
    }
  else
    {
      Mword *pt = (Mword *)Mem_layout::phys_to_pmem
	(raw[pd_idx] & PT_BASE_MASK);

      if (!pte_valid(pt + pt_idx)) 
	return E_INVALID;

      unsigned a4 = (a & PDE_AP_MASK);
      unsigned a5 = a4 | (a4 >> 2) | (a4 >> 4) | (a4 >> 6);

      switch(pte_type(pt + pt_idx)) 
	{
	case PTE_TYPE_LARGE:
	  assert(false /*PTE_TYPE_LATGE*/);
#if 0
	  set_pte64kb(pt + pt_idx,(pt[pt_idx] & 0x0ff0) | a5, 
	              force_flush || _current==this, va);
#endif
	  break;
	default:
	case PTE_TYPE_SMALL:
	  if (force_flush || _current==this)
	    Mem_unit::flush_cache(va, (char*)va + 0x1000);
	  
	  set_pte(pt + pt_idx, (pt[pt_idx] & 0x0ff0) | a5,
	          force_flush || _current==this, va);
	  break;
	}
    }
  return E_OK;
}


IMPLEMENT inline NEEDS[Page_table::__insert]
Page_table::Status Page_table::insert_invalid(void *va, size_t size, 
                                              Mword value, bool flush)
{
  return __insert(P_ptr<void>((void*)value), va, size, 0, true, true, flush);
}


#include <cstdio>
#include "mem_unit.h"

PRIVATE /*inline NEEDS [<cassert>, <cstring>, "mem_unit.h",
		      Page_table::page_sizes, 
		      Page_table::page_shifts, Page_table::pd_index,
		      Page_table::pt_index, Page_table::kb64_present,
                      "kdb_ke.h"]*/
Page_table::Status 
Page_table::__insert(P_ptr<void> pa, void *va, 
		     size_t size, Page::Attribs a, 
		     bool replace, bool invalid, 
		     bool force_flush)
{
  size_t const * const sizes = page_sizes();
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);
  bool need_flush = false;

  if (size == sizes[1]) 
    { // 1MB
      // try to insert or replace a 1MB superpage,
      // which are located in the page directory

      // assert the right alignment
      assert(invalid || ((pa.get_unsigned() & (sizes[1]-1)) == 0));
      assert(((Mword)va & (sizes[1]-1)) == 0);

      // printf("X: insert 1MB mapping va=%p, pa=%08x\n", va, pa.get_unsigned());

      if (pde_valid(raw + pd_idx)) 
	{
	  if(!replace)
	    return E_EXISTS;

	  need_flush = true;

	  if (!pde_section(raw + pd_idx)) 
	    {
	      kdb_ke("Overwrite non-section in PD");
	      return E_OK;
	    }
	}

      // there was nothing mapped before
      if (!invalid)
	set_pte(raw + pd_idx, pa.get_unsigned() | PDE_TYPE_SECTION 
	                      | (a & Page::MAX_ATTRIBS),
		force_flush || _current==this, va);
      else
        set_pte(raw + pd_idx, pa.get_unsigned() & ~PDE_PRESENT,
	        force_flush || _current==this, va);

    } 
  else
    {
      //printf("X: insert 4KB mapping va=%p, pa=%08x\n", va, pa.get_unsigned());
      // A 4KB mapping is requested
      assert(size == sizes[0]);
      // assert the right alignment
      assert(invalid || ((pa.get_unsigned() & (sizes[0]-1)) == 0));
      assert(((Mword)va & (sizes[0]-1)) == 0);

      Mword *pt = 0;

      if (pde_section(raw + pd_idx)) 
	{
	  if(!replace) 
	    return E_EXISTS;
	  else 
	    need_flush = true;

	} 
      else if (pde_coarse(raw + pd_idx)) 
	{
	  pt = (Mword*)Mem_layout::phys_to_pmem(raw[pd_idx] & PT_BASE_MASK);

	  if (pte_valid(pt + pt_idx))
	    {
	      if (!replace) 
		return E_EXISTS;
	      else 
		need_flush = true;
	    }
	} 
      else
	{
	  // fine page tables are not used !!
	  assert(!pde_valid(raw + pd_idx));
	  // no allocate one
	  pt = (Mword*)alloc()->alloc(10);
	  if (!pt) 
	    return E_NOMEM;

	  memset(pt, 0, 1024);

	  // I write back the whole data cache, may be more efficent
	  // than iterate over 4096 bytes, but also may be not
	  if (force_flush || _current==this)
	    Mem_unit::clean_dcache(pt, ((char*)pt) + 1024);

	  // printf("X: write %08x to PD @%p\n", Mem_layout::pmem_to_phys((Address)pt) | PDE_TYPE_COARSE, raw + pd_idx);
	  set_pte(raw + pd_idx, Mem_layout::pmem_to_phys((Address)pt) 
	                        | PDE_TYPE_COARSE,
		  force_flush || _current==this, 0);
	}

      if (!invalid)
	{
	  unsigned ap = a & PDE_AP_MASK;

	  // printf("X: write %08x to PT @%p\n", pa.get_unsigned() | PTE_TYPE_SMALL | ap | (ap >> 2) | (ap >> 4) | (ap >> 6) | (a & Page::MAX_ATTRIBS), pt + pt_idx);


	  set_pte(pt + pt_idx, pa.get_unsigned() | PTE_TYPE_SMALL 
	                       | ap | (ap >> 2) | (ap >> 4) | (ap >> 6) 
			       | (a & Page::MAX_ATTRIBS),
	          force_flush || this==_current, va);
	} 
      else
	{
	  // printf("X: write to PT @ %p\n", pt + pt_idx);
	  set_pte(pt + pt_idx, pa.get_unsigned() & ~PTE_PRESENT,
	          force_flush || this==_current, va);
	}
    }

  if (need_flush && (this==_current))
    Mem_unit::tlb_flush();

  return E_OK;

}

IMPLEMENT inline NEEDS [Page_table::__insert]
Page_table::Status Page_table::insert(P_ptr<void> pa, void* va, size_t size, 
				      Page::Attribs a, bool force_flush)
{
  return __insert(pa, va, size, a, false, false, force_flush);
}

IMPLEMENT inline NEEDS [Page_table::__insert] 
Page_table::Status Page_table::replace(P_ptr<void> pa, void* va, size_t size, 
				       Page::Attribs a, bool force_flush)
{
  return __insert(pa, va, size, a, true, false, force_flush);
}


IMPLEMENT /*inline*/
Page_table::Status Page_table::remove(void *va, bool force_flush)
{
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);

  if (!pde_valid(raw + pd_idx))
    return E_OK;

  if (pde_section(raw + pd_idx))
    {
      if (force_flush || this == _current)
	Mem_unit::flush_cache(va, (char*)va + 0x100000);
	
      set_pte(raw+ pd_idx, 0, force_flush || this == _current, va);
      return E_OK;
    }

  Mword *pt = (Mword *)Mem_layout::phys_to_pmem
    (raw[pd_idx] & PT_BASE_MASK);

  if (!pte_valid(pt + pt_idx))
    return E_OK;
  
  switch (pte_type(pt + pt_idx))
    {
    case PTE_TYPE_LARGE: // 64k
      assert(false /*PTE_TYPE_LARGE*/);
#if 0
      for( unsigned idx = pt_idx & ~0x0f, end = idx + 16; idx < end; 
	   ++idx)
	pt[idx] = 0;
      if (force_flush || this == _current)
        Mem_unit::clean_dcache(pt + (pt_idx & ~0x0f), 
	                       pt + (pt_idx & ~0x0f) + 16);
#endif
      return E_OK;
    case PTE_TYPE_SMALL: // 4k
      if (force_flush || this == _current)
	Mem_unit::flush_cache(va, (char*)va + 4096);
      
      set_pte(pt + pt_idx, 0, force_flush || this == _current, va);
      return E_OK;
    default:
      return E_OK;
    }
}


PRIVATE inline NEEDS[Page_table::pd_index, Page_table::pt_index,
	             Page_table::pte_valid, Page_table::pde_valid,
		     Page_table::pde_section, Page_table::pte_type]
Mword Page_table::__lookup(void *va, Address *size, Page::Attribs *a,
			   bool &valid) const
{
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);
  valid = false;

  if (!pde_valid(raw + pd_idx)) 
    {
      if (size) *size = 1024*1024;
      return raw[pd_idx];
    }

  if (pde_section(raw + pd_idx)) 
    {
      if(size)
	*size = 1024*1024;
      if(a)
	*a = (Page::Attribs)(raw[pd_idx] & PDE_AP_MASK);

      valid = true;
      return (raw[pd_idx] & 0xfff00000) | ((Unsigned32)va & 0x0fffff );

    } 
  else 
    {
      // no test for fine pgt
      Mword *pt = (Mword *)Mem_layout::phys_to_pmem
	(raw[pd_idx] & PT_BASE_MASK);

      if (!pte_valid(pt + pt_idx)) 
	{
	  // the size value must be 1024 if, the page table
	  // is a fine one (but we do not support this at the
	  // momemnt).
	  if (size) *size = 4096;
  	  return pt[pt_idx];
	}

      valid = true;
      Unsigned32 ret = pt[pt_idx] & PAGE_BASE_MASK;
      if (a) 
	*a = pt[pt_idx] & PDE_AP_MASK;

      switch(pte_type(pt + pt_idx)) 
	{
	case PTE_TYPE_LARGE:
	  if(size) *size = 64*1024;
	  return (ret | ((Unsigned32)va & 0x00ffff ));
	default:
	case PTE_TYPE_SMALL:
	  if(size) *size = 4096;
	  return (ret | ((Unsigned32)va & 0x000fff ));
	case PTE_TYPE_TINY: // should never occur
	  if(size) *size = 1024;
	  return (ret | ((Unsigned32)va & 0x0003ff ));
	}
    }
}

IMPLEMENT inline NEEDS[Page_table::__lookup]
P_ptr<void> Page_table::lookup( void *va, Address *size, Page::Attribs *a ) const
{
  bool valid = false;
  Mword ret = __lookup(va, size, a, valid);
  if(valid)
    return P_ptr<void>(ret);
  else
    return P_ptr<void>();
}


IMPLEMENT inline NEEDS[Page_table::__lookup]
Mword Page_table::lookup_invalid( void *va ) const
{
  bool valid = false;
  Mword ret = __lookup(va, 0, 0, valid);
  if(valid)
    return (Mword)-1;
  else
    return ret;
}





IMPLEMENT inline
size_t const Page_table::num_page_sizes()
{
  return 2;
}

PUBLIC /*inline*/
void Page_table::activate()
{
  P_ptr<Page_table> p = P_ptr<Page_table>(Mem_layout::pmem_to_phys
      ((Address)this));
  if(_current!=this) 
    {
      _current = this;
      Mem_unit::flush_cache();
      asm volatile ( 
	  "mcr p15, 0, r0, c8, c7, 0x00 \n" // TLB flush
	  "mcr p15, 0, %0, c2, c0       \n" // pdbr

	  "mrc p15, 0, r1, c2, c0       \n" 
	  "mov r1,r1                    \n"
	  "sub pc,pc,#4                 \n"
	  
	  : 
	  : "r"(p.get_unsigned()) 
	  : "r1" );
      
    }
}

IMPLEMENT
void Page_table::init(Page_table *current)
{
  unsigned domains      = 0x0001;
  _current = current;

  asm volatile ( 
      "mcr p15, 0, %0, c3, c0       \n" // domains
      :
      : "r"(domains) );
}


IMPLEMENT /*inline*/
void Page_table::copy_in(void *my_base, Page_table *o, 
			 void *base, size_t size, bool force_flush )
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index((char*)my_base + size);
  unsigned o_pd_idx = pd_index(base);
  bool need_flush = false;

  //printf("copy_in: %03x-%03x from %03x\n", pd_idx, pd_idx_max, o_pd_idx);
 
  if (force_flush)
    {
      for (unsigned i = pd_idx; (i & 0x0fff) != pd_idx_max; ++i)
	if (pde_valid(raw + i))
	  {
	    Mem_unit::flush_dcache();
	    need_flush = true;
	    break;
	  }
    }
  
  for (unsigned i = pd_idx; (i & 0x0fff) != pd_idx_max; ++i, ++o_pd_idx)
    raw[i] = o->raw[o_pd_idx];

  if (force_flush)
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);
  
  if (need_flush && force_flush)
    Mem_unit::dtlb_flush();

}

PUBLIC
void
Page_table::invalidate(void *my_base, unsigned size, bool flush = true)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index((char*)my_base + size);
  bool need_flush = false;
  
  //printf("invalidate: %03x-%03x\n", pd_idx, pd_idx_max);

  if (flush)
    {
      for (unsigned i = pd_idx; (i & 0x0fff) != pd_idx_max; ++i)
	if (pde_valid(raw + i))
	  {
	    Mem_unit::flush_dcache();
	    need_flush = true;
	    break;
	  }
    }

  for (unsigned i = pd_idx; (i & 0x0fff) != pd_idx_max; ++i)
    raw[i] = 0;
  
  if (flush)
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);
  
  if (need_flush && flush)
    Mem_unit::tlb_flush();
}

IMPLEMENT inline
Page_table *Page_table::current()
{
  return _current;
}

IMPLEMENT
void *
Page_table::dir() const
{
  return const_cast<Page_table *>(this);
}

