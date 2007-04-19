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
  enum
  {
    Pt_base_mask     = 0xfffffc00, 
    Pde_type_coarse  = 0x01,
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
bool 
Pte::superpage() const
{ return !(_pt & 3) && ((*_pte & 3) == 2); }

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


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

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


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6]:

PUBLIC inline NEEDS[Pte::__set]
void 
Pte::set(Address phys, unsigned long size, Mword attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      if (size != (1 << 20))
	return;
	{
	  unsigned long a = attr & 0x0c; // C & B
	  a |= ((attr ^ 0x200) & 0xfe0) << 6;
	  __set(phys | a | 0x402, write_back);
	}
      break;
    case 1:
      if (size != (4 << 10))
	return;
      __set(phys | ((attr ^ 0x200) & Page::MAX_ATTRIBS) | 0x12, write_back);
      break;
    }
}

PUBLIC inline
unsigned long 
Pte::attr() const 
{
  switch (_pt & 3)
    {
    case 0:
	{
	  unsigned long a = *_pte & 0x0c; // C & B
	  a |= (*_pte >> 6) & 0xfe0;
	  return a ^ 0x200;
	}
    case 1:
    default:
      return (*_pte & Page::MAX_ATTRIBS) ^ 0x200;
    }
}

PUBLIC inline NEEDS["mem_unit.h"]
void 
Pte::attr(unsigned long attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 1:
      __set((*_pte & ~Page::MAX_ATTRIBS) 
	  | ((attr & Page::MAX_ATTRIBS) ^ 0x200), write_back);
      break;
    case 0:
	{ 
	  unsigned long a = attr & 0x0c;
	  a |= ((attr ^ 0x200) & 0xfe0) << 6;
	  __set((*_pte & ~0x3f8c) | a, write_back);
	}
      break;
    }
}

PUBLIC /*inline*/
void Page_table::activate(unsigned long asid)
{
  Pte p = current()->walk(this,0,false,0);
  if(_current!=this) 
    {
      _current = this;
      asm volatile (
	  "mcr p15, 0, r0, c7, c10, 4   \n"
	  "mcr p15, 0, %0, c2, c0       \n" // pdbr
	  "mcr p15, 0, %1, c13, c0, 1   \n"

	  "mrc p15, 0, r1, c2, c0       \n" 
	  "mov r1,r1                    \n"
	  "sub pc,pc,#4                 \n"
	  
	  : 
	  : "r"(p.phys(this)), "r"(asid)
	  : "r1" );
      
    }
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm]:

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
      Pte p(this, 0, raw + i);
      if (p.valid() && !p.superpage())
	{
	  void *pt = (void*)Mem_layout::phys_to_pmem(p.raw() & Pt_base_mask);
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

PUBLIC
Pte 
Page_table::walk(void *va, unsigned long size, bool write_back, Ram_quota *q)
{
  unsigned const pd_idx = pd_index(va);

  Mword *pt = 0;

  Pte pde(this, 0, raw + pd_idx);

  if (!pde.valid()) 
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
	    return pde;

	  memset(pt, 0, 1024);
	  
	  if (write_back || !Arm_vcache)
	    Mem_unit::clean_dcache(pt, (char*)pt + 1024);

	  raw[pd_idx] = current()->walk(pt, 0, false, 0).phys(pt)
	    | Pde_type_coarse;
	  
	  if (write_back || !Arm_vcache)
	    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx);
	}
      else
	return pde;
    }
  else if (pde.superpage()) 
    return pde;

  if (!pt)
    pt = (Mword *)Mem_layout::phys_to_pmem(pde.raw() & Pt_base_mask);

  unsigned const pt_idx = pt_index(va);

  return Pte(this, 1, pt + pt_idx);
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
			 void *base, size_t size, unsigned long asid)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  unsigned o_pd_idx = pd_index(base);
  bool need_flush = false;

  //printf("copy_in: %03x-%03x from %03x\n", pd_idx, pd_idx_max, o_pd_idx);
 
  if (asid != ~0UL)
    {
      for (unsigned i = pd_idx;  i < pd_idx_max; ++i)
	if (Pte(this, 0, raw + i).valid())
	  {
	    Mem_unit::flush_vdcache();
	    need_flush = true;
	    break;
	  }
    }
  
  for (unsigned i = pd_idx; i < pd_idx_max; ++i, ++o_pd_idx)
    raw[i] = o->raw[o_pd_idx];

  if ((asid != ~0UL) || !Arm_vcache)
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);

  if (need_flush && (asid != ~0UL))
    Mem_unit::dtlb_flush(asid);
}

PUBLIC
void
Page_table::invalidate(void *my_base, unsigned size, unsigned long asid = ~0UL)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  bool need_flush = false;
  
  //printf("invalidate: %03x-%03x\n", pd_idx, pd_idx_max);

  if (asid != ~0UL)
    {
      for (unsigned i = pd_idx; i < pd_idx_max; ++i)
	if (Pte(this, 0, raw + i).valid())
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
  if ((asid != ~0UL) || !Arm_vcache)
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);
  
  if (need_flush && (asid != ~0UL))
    Mem_unit::tlb_flush(asid);
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

