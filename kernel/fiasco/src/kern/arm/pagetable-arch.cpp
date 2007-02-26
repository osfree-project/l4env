INTERFACE:

#include "paging.h"


EXTENSION class Page_table
{
private:
  Unsigned32 raw[4096];
  
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
    PT_BASE_MASK     = 0xfffff000, 

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


IMPLEMENTATION[arch]:

#include <cassert>
#include <cstring>


#include "mem_unit.h"

Page_table *Page_table::_current;

//#include "panic.h"

IMPLEMENT
void * Page_table::operator new( size_t s )
{
  assert(s == 16*1024);
  return alloc()->alloc(2); // 2^2 = 4 pages = 16K
}


IMPLEMENT
void Page_table::operator delete( void *b )
{
  alloc()->free( 2, b );
}


IMPLEMENT
Page_table::Page_table()
{
  for( unsigned i = 0; i< 4096; ++i ) raw[i]=0;
}


PRIVATE inline
static unsigned Page_table::pd_index( void const *const address )
{
  return (Mword)address >> 20; // 1MB steps
}

PRIVATE inline
static unsigned Page_table::pt_index( void const *const address )
{
  return ((Mword)address >> 12) & 1023; // 4KB steps for coarse pts
}


IMPLEMENT inline
size_t const * const Page_table::page_sizes()
{
  static size_t const _page_sizes[] = {4 *1024, 64 *1024, 1024 *1024};
  return _page_sizes;
}

IMPLEMENT inline
size_t const * const Page_table::page_shifts()
{
  static size_t const _page_shifts[] = {12, 16, 20};
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


IMPLEMENT
Page_table::Status Page_table::insert_invalid( void *va, 
					       size_t size, 
					       Mword value )
{
  return __insert( P_ptr<void>((void*)value), va, size, 0, false, true );
}


PRIVATE inline NEEDS [<cassert>, <cstring>, "mem_unit.h",
		      Page_table::page_sizes, 
		      Page_table::page_shifts, Page_table::pd_index,
		      Page_table::pt_index, Page_table::kb64_present]
Page_table::Status Page_table::__insert( P_ptr<void> pa, void const *const va, 
					 size_t size, Page::Attribs a, 
					 bool replace, bool invalid)
{
  size_t const * const sizes = page_sizes();
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);

  if( size == sizes[2] ) { // 1MB
    // try to insert or replace a 1MB superpage,
    // which are located in the page directory

    // assert the right alignment
    assert(invalid || ((pa.get_unsigned() & (sizes[2]-1)) == 0));
    assert(((Mword)va & (sizes[2]-1)) == 0);

    if(raw[pd_idx] & PDE_PRESENT) {
      if(!replace)
	return E_EXISTS;
      
      if((raw[pd_idx] & PDE_TYPE_MASK) != PDE_TYPE_SECTION) {
	// if there was anything else than a section mapping
	// get the virtual address of the 4KB second level page 
	// that contains four adjacent second level page tables
	Mword *pt = alloc()->phys_to_virt(P_ptr<Mword>(
			(Mword*)(raw[pd_idx] & PT_BASE_MASK)));

	// insert the entry, either a valid mapping
	// or some invalid data
	if(!invalid)
	  raw[pd_idx] = pa.get_unsigned() 
	    | PDE_TYPE_SECTION | (a & Page::MAX_ATTRIBS);
	else
	  raw[pd_idx] = pa.get_unsigned() & ~PDE_PRESENT;

	// if we are current, we want to 
	// make the changes visible to the MMU
	if(_current==this)
	  Mem_unit::write_back_data_cache( raw + pd_idx );

	// try to free the four adjacent 
	// second level page tables if non 
	// of them is sed anymore
	unsigned i = 0;
	for(; i<4; ++i) {
	  if((raw[(pd_idx & 0xffc) + i] & PDE_TYPE_MASK)==PDE_TYPE_COARSE)
	    break;
	}
	if(i==4) alloc()->free(0, pt);
	return E_OK;
      }
    }

    // there was nothing mapped before
    if(!invalid)
      raw[pd_idx] = pa.get_unsigned() 
	| PDE_TYPE_SECTION | (a & Page::MAX_ATTRIBS);
    else
      raw[pd_idx] = pa.get_unsigned() & ~PDE_PRESENT;

    // if we are current, we want to 
    // make the changes visible to the MMU
    if(_current==this)
      Mem_unit::write_back_data_cache( raw + pd_idx );

  } else if( size == sizes[1] ) { 
    // 64KB Mapping requested (located in the scond level)
    // (use 16 slots per page)

    // assert the right alignment
    assert(invalid || ((pa.get_unsigned() & (sizes[1]-1)) == 0));
    assert(((Mword)va & (sizes[1]-1)) == 0);


    Mword *pt = 0;

    if((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_SECTION) {
      if(!replace) {
	return E_EXISTS;
      }
    } else if((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_COARSE) {

      // threre is already a second level,
      // get the virtual address
      pt = alloc()->
	phys_to_virt(P_ptr<Mword>((Mword*)(raw[pd_idx] 
					   & PT_BASE_MASK)));

      // check if there is any valid mapping 
      // within the 16 entries
      if(!replace && kb64_present(pt,pt_idx)) {
	return E_EXISTS;
      }
      

    } else {
      // fine page tables are not used !!
      assert((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_FREE);

      // look if there is already a 4KB page allocated 
      // for the second level
      unsigned i = 0;
      for(; i<4; ++i)
	if((raw[(pd_idx & 0x0ffc) + i] & PDE_TYPE_MASK) == PDE_TYPE_COARSE)
	  break;

      // i<4 means there is already a second level 
      if(i<4) 
	pt = alloc()->phys_to_virt(P_ptr<Mword>((Mword*)
	       (raw[(pd_idx & 0x0ffc) + i] & PT_BASE_MASK)));
      else {
	pt = (Mword*)alloc()->alloc(0);
	memset(pt,0,4096);
	if(!pt) return E_NOMEM;
      }
      
      raw[pd_idx] = (alloc()->virt_to_phys(pt).get_unsigned() + 
		     1024*(pd_idx & 0x03)) | PDE_TYPE_COARSE;
      if(_current==this)
	Mem_unit::write_back_data_cache( raw + pd_idx );

	
    }

    if(!invalid) {
      unsigned ap = a & PDE_AP_MASK;
      for(unsigned i=0;i<16;++i) {
	pt[(pt_idx & 0x03f0) + i] = pa.get_unsigned() | 
	  ap | (ap >> 2) | (ap >> 4) | (ap >> 6) | 
	  (a & Page::MAX_ATTRIBS) | PTE_TYPE_LARGE;
	if(_current==this)
	  Mem_unit::write_back_data_cache( pt + (pt_idx & 0x03f0) + i );
      }
    } else {
      pt[(pt_idx & 0x03f0)] = pa.get_unsigned() & ~PTE_PRESENT;
    }
    

  } else {
    // A 4KB mapping is requested

    assert(size == sizes[0]);
    // assert the right alignment
    assert(invalid || ((pa.get_unsigned() & (sizes[0]-1)) == 0));
    assert(((Mword)va & (sizes[0]-1)) == 0);

    Mword *pt = 0;

    if((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_SECTION) {
      if(!replace) 
	return E_EXISTS;

    } else if((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_COARSE) {
      pt = alloc()->
	phys_to_virt(P_ptr<Mword>((Mword*)(raw[pd_idx] 
					   & PT_BASE_MASK)));

      if(pt[pt_idx] & PTE_PRESENT) {
	if(!replace) 
	  return E_EXISTS;
	else {
	  if((pt[pt_idx] & PTE_TYPE_MASK) == PTE_TYPE_LARGE) {
	    // kick the 64KB mapping
	    for(unsigned i=0;i<16;++i) {
	      pt[(pt_idx & 0x3f0) +i] = 0; // free
	      if(_current==this)
		Mem_unit::write_back_data_cache( pt + (pt_idx & 0x03f0) + i );
	    }
	  }
	}
      }
    } else {
      // fine page tables are not used !!
      assert((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_FREE);

      // look if there is already a 4KB 
      // second level allocated
      unsigned i = 0;
      for(; i<4; ++i)
	if((raw[(pd_idx & 0x0ffc) + i] & PDE_TYPE_MASK) == PDE_TYPE_COARSE)
	  break;

      if(i<4)
	// yes there is
	pt = alloc()->phys_to_virt(P_ptr<Mword>((Mword*)
	  (raw[(pd_idx & 0x0ffc) + i] & PT_BASE_MASK)));
      else {
	// no allocate one
	pt = (Mword*)alloc()->alloc(0);
	memset(pt,0,4096);
	if(!pt) return E_NOMEM;

	// I write back the whole data cache, may be more efficent
	// than iterate over 4096 bytes, but also may be not
	if(_current==this)
	  Mem_unit::write_back_data_cache();
      }
      
      raw[pd_idx] = (alloc()->virt_to_phys(pt).get_unsigned() + 
		     1024*(pd_idx & 0x03)) | PDE_TYPE_COARSE;

      if(_current==this)
	Mem_unit::write_back_data_cache(raw + pd_idx);

      
    }

    if(!invalid) {
      unsigned ap = a & PDE_AP_MASK;
      
      pt[pt_idx] = pa.get_unsigned() | PTE_TYPE_SMALL |
	ap | (ap >> 2) | (ap >> 4) | (ap >> 6) | 
	(a & Page::MAX_ATTRIBS);
    } else {
      pt[pt_idx] = pa.get_unsigned() & ~PTE_PRESENT;
    }
      
    if(this==_current)
      Mem_unit::write_back_data_cache(pt + pt_idx);
	
  }
    
  return E_OK;

}

IMPLEMENT inline NEEDS [Page_table::__insert]
Page_table::Status Page_table::insert( P_ptr<void> pa, void* va, size_t size, 
				       Page::Attribs a)
{
  return __insert( pa, va, size, a, false, false );
}

IMPLEMENT inline NEEDS [Page_table::__insert] 
Page_table::Status Page_table::replace( P_ptr<void> pa, void* va, size_t size, 
				       Page::Attribs a)
{
  return __insert( pa, va, size, a, true, false );
}


IMPLEMENT inline
Page_table::Status Page_table::remove( void* )
{
  return E_OK;
}


PRIVATE inline
Mword Page_table::__lookup( void *va, size_t *size, Page::Attribs *a,
			    bool &valid ) const
{
  unsigned const pd_idx = pd_index(va);
  unsigned const pt_idx = pt_index(va);
  valid = false;

  if(!(raw[pd_idx] & PDE_PRESENT)) 
    return raw[pd_idx];

  if((raw[pd_idx] & PDE_TYPE_MASK) == PDE_TYPE_SECTION) {
    if(size)
      *size = 1024*1024;
    if(a)
      *a = (Page::Attribs)(raw[pd_idx] & PDE_AP_MASK);

    valid = true;
    return (raw[pd_idx] & 0xfff00000) | ((Unsigned32)va & 0x0fffff );
    
  } else {
    // no test for fine pgt
    Unsigned32 *pt = alloc()->
      phys_to_virt(P_ptr<Unsigned32>(raw[pd_idx] 
				     & PT_BASE_MASK));

    if(! (pt[pt_idx] & PTE_PRESENT)) 
      return pt[pt_idx];
    
    valid = true;
    Unsigned32 ret = pt[pt_idx] & PAGE_BASE_MASK;
    if(a) *a = pt[pt_idx] & PDE_AP_MASK;

    switch(pt[pt_idx] & PTE_TYPE_MASK) {
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
P_ptr<void> Page_table::lookup( void *va, size_t *size, Page::Attribs *a ) const
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
  return 3;
}



PUBLIC /*inline*/
void Page_table::activate()
{
  P_ptr<Page_table> p = allocator->virt_to_phys(this);
  if(_current!=this) {
    _current = this;
    Mem_unit::write_back_data_cache();
    asm volatile ( "mcr p15, 0, %0, c2, c0       \n" // pdbr
		   "mcr p15, 0, r0, c8, c7, 0x00 \n" // TLB flush
		   "mcr p15, 0, r0, c7, c7, 0x00 \n" // Cache flush
		   : :
		   "r"(p.get_unsigned())
		   );
  }

}

IMPLEMENT
void Page_table::init()
{
  unsigned domains      = 0x0001;
  _current = 0;

  asm volatile ( "mcr p15, 0, %0, c3, c0       \n" // domains
		 :
		 :
		 "r"(domains)
	        );

  // kernel mode should acknowledge write-protected page table entries
  //  set_cr0(get_cr0() | CR0_WP);
}


IMPLEMENT inline
Page_table *Page_table::current()
{
  return _current;
}
