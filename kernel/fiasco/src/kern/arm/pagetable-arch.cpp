//---------------------------------------------------------------------------
INTERFACE[arm && !vcache]:
class Pte 
{
private:
  enum { Arm_vcache = 0 };
};

//---------------------------------------------------------------------------
INTERFACE[arm && vcache]:
class Pte
{
private:
  enum { Arm_vcache = 1 };
};

INTERFACE [arm]:

#include "paging.h"

class Ram_quota;

EXTENSION class Pte
{
public:
//private:
  unsigned long _pt;
  Mword *_pte;

public:
  Pte(Page_table *pt, unsigned level, Mword *pte)
  : _pt((unsigned long)pt | level), _pte(pte)
  {}

};

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
    PDE_CACHE_MASK   = 0x000c,

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
INTERFACE[arm && !vcache]:
EXTENSION class Page_table 
{
private:
  enum { Arm_vcache = 0 };
};

//---------------------------------------------------------------------------
INTERFACE[arm && vcache]:
EXTENSION class Page_table 
{
private:
  enum { Arm_vcache = 1 };
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>

#include "mem_unit.h"
#include "kdb_ke.h"
#include "ram_quota.h"

Page_table *Page_table::_current;
  
PUBLIC inline
unsigned long 
Pte::valid() const
{ return *_pte & 3; }

PUBLIC inline
unsigned long 
Pte::phys() const
{
  switch(_pt & 3)
    {
    case 0:
      switch (*_pte & 3)
	{
	case 2:  return *_pte & ~((1 << 20) - 1); // 1MB
	default: return ~0UL;
	}
    case 1:
      switch (*_pte & 3)
	{
	case 2: return *_pte & ~((4 << 10) - 1);
	default: return ~0UL;
	}
    default: return ~0UL;
    }
}

PUBLIC inline
unsigned long
Pte::phys(void *virt)
{
  unsigned long p = phys();
  return p | (((unsigned long)virt) & (size()-1));
}

PUBLIC inline
unsigned long 
Pte::lvl() const
{ return (_pt & 3); }

PUBLIC inline
unsigned long
Pte::raw() const
{ return *_pte; }

PUBLIC inline
unsigned long 
Pte::size() const
{
  switch(_pt & 3)
    {
    case 0:
      switch (*_pte & 3)
	{
	case 2:  return 1 << 20; // 1MB
	default: return 1 << 20;
	}
    case 1:
      switch (*_pte & 3)
	{
	case 1: return 64 << 10;
	case 2: return 4 << 10;
	case 3: return 1 << 10;
	default: return 4 << 10;
	}
    default: return 0;
    }
}

PRIVATE inline NEEDS["mem_unit.h"]
void
Pte::__set(unsigned long v, bool write_back)
{
  *_pte = v;
  if (write_back || !Arm_vcache) 
    Mem_unit::clean_dcache(_pte);
}

PUBLIC inline NEEDS[Pte::__set]
void 
Pte::set_invalid(unsigned long val, bool write_back)
{ __set(val & ~3, write_back); }

PUBLIC inline NEEDS[Pte::__set]
void 
Pte::set(Address phys, unsigned long size, Mword attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      if (size != (1 << 20))
	return;
      __set(phys | (attr & Page::MAX_ATTRIBS) | 2, write_back);
      break;
    case 1:
	{ 
	  if (size != (4 << 10))
	    return;
	  unsigned long ap = attr & 0xc00; ap |= ap >> 2; ap |= ap >> 4; 
	  __set(phys | (attr & 0x0c) | ap | 2, write_back);
	}
      break;
    }
}

PUBLIC inline
unsigned long 
Pte::attr() const { return *_pte & 0xc0c; }

PUBLIC inline NEEDS["mem_unit.h"]
void 
Pte::attr(unsigned long attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      __set((*_pte & ~0xc0c) | (attr & 0xc0c), write_back);
      break;
    case 1:
	{ 
	  unsigned long ap = attr & 0xc00; ap |= ap >> 2; ap |= ap >> 4; 
	  __set((*_pte & ~0xffc) | (attr & 0x0c) | ap, write_back);
	}
      break;
    }
}

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
  if (!Arm_vcache)
    Mem_unit::clean_dcache(raw, raw + 4096);
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

PUBLIC
Pte 
Page_table::walk(void *va, unsigned long size, bool write_back, Ram_quota *q)
{
  unsigned const pd_idx = pd_index(va);

  Mword *pt = 0;

  if (!pde_valid(raw + pd_idx)) 
    {
      if (size == (4 << 10))
	{
	  assert(q);
	  if (q->alloc(1<<10))
	    {
	      pt = (Mword*)alloc()->alloc(10);
	      if (EXPECT_FALSE(!pt))
		q->free(1<<10);
	    }
	  if (!pt) 
	    return Pte(this, 0, raw + pd_idx);

	  memset(pt, 0, 1024);
	  
	  if (write_back || !Arm_vcache)
	    Mem_unit::clean_dcache(pt, (char*)pt + 1024);

	  raw[pd_idx] = current()->walk(pt, 0, false, 0).phys(pt)
	    | PDE_TYPE_COARSE;
	  
	  if (write_back || !Arm_vcache)
	    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx);
	}
      else
	return Pte(this, 0, raw + pd_idx);
    }
  else if (pde_section(raw + pd_idx)) 
    return Pte(this, 0, raw + pd_idx);

  if (!pt)
    pt = (Mword *)Mem_layout::phys_to_pmem(raw[pd_idx] & PT_BASE_MASK);

  unsigned const pt_idx = pt_index(va);

  return Pte(this, 1, pt + pt_idx);
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
	*a = (Page::Attribs)(raw[pd_idx] & (PDE_AP_MASK | PDE_CACHE_MASK));

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
	*a = pt[pt_idx] & (PDE_AP_MASK | PDE_CACHE_MASK);

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

#if 0
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
#endif


IMPLEMENT inline
size_t const Page_table::num_page_sizes()
{
  return 2;
}

PUBLIC /*inline*/
void Page_table::activate()
{
  Pte p = current()->walk(this,0,false,0);
  if(_current!=this) 
    {
      _current = this;
      Mem_unit::flush_vcache();
      asm volatile ( 
	  "mcr p15, 0, r0, c8, c7, 0x00 \n" // TLB flush
	  "mcr p15, 0, %0, c2, c0       \n" // pdbr

	  "mrc p15, 0, r1, c2, c0       \n" 
	  "mov r1,r1                    \n"
	  "sub pc,pc,#4                 \n"
	  
	  : 
	  : "r"(p.phys(this)) 
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
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  unsigned o_pd_idx = pd_index(base);
  bool need_flush = false;

  //printf("copy_in: %03x-%03x from %03x\n", pd_idx, pd_idx_max, o_pd_idx);
 
  if (force_flush)
    {
      for (unsigned i = pd_idx;  i < pd_idx_max; ++i)
	if (pde_valid(raw + i))
	  {
	    Mem_unit::flush_vdcache();
	    need_flush = true;
	    break;
	  }
    }
  
  for (unsigned i = pd_idx; i < pd_idx_max; ++i, ++o_pd_idx)
    raw[i] = o->raw[o_pd_idx];

  if (force_flush || !Arm_vcache)
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);

  if (need_flush && force_flush)
    Mem_unit::dtlb_flush();
}

PUBLIC
void
Page_table::invalidate(void *my_base, unsigned size, bool flush = true)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  bool need_flush = false;
  
  //printf("invalidate: %03x-%03x\n", pd_idx, pd_idx_max);

  if (flush)
    {
      for (unsigned i = pd_idx; i < pd_idx_max; ++i)
	if (pde_valid(raw + i))
	  {
	    Mem_unit::flush_vdcache();
	    need_flush = true;
	    break;
	  }
    }

  for (unsigned i = pd_idx; i < pd_idx_max; ++i)
    raw[i] = 0;
  
  // clean the caches if manipulating the current pt or in the case if phys.
  // tagged caches.
  if (flush || !Arm_vcache)
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

