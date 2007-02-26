INTERFACE:

#include "kmem_slab_simple.h"

class kmem_slab_t : public kmem_slab_simple_t
{
};

IMPLEMENTATION:

// kmem_slab_t -- A type-independent slab cache allocator for Fiasco,
// derived from a generic slab cache allocator (slab_cache_anon in
// lib/slab.{h,cc}) and from kmem_slab_simple_t which handles locking
// and kmem_slab_t-instance allocation for us.

// This specialization adds multi-page slabs.
// We allocate multi-page slabs as multiple, potentially
// non-contiguous physical pages and map them contiguously into a
// region of our virtual address space managed by region_t (in
// region.{h,cc}).

// XXX This implementation technique has one important drawback: When
// using virtual pages, we need TLB entries for these 4KB pages, thus
// increasing the number of 4KB-page TLB misses, whereas accesses to
// physical pages would use superpage (4MB) mappings using the
// 4MB-page TLB.  The load on the superpage TLB is substantially
// lower.  Maybe we should try to find a contiguous physical region
// first and only use virtual regions as a last resort?
//-

#include <cassert>

#include "config.h"
#include "vmem_alloc.h"
#include "kmem_alloc.h"
#include "region.h"

#undef PAGE_SIZE

extern "C" {
  extern char _mappings_1, _mappings_end_1;
}

PUBLIC
kmem_slab_t::kmem_slab_t(unsigned long slab_size, unsigned elem_size, 
			 unsigned alignment)
  : kmem_slab_simple_t (slab_size, elem_size, alignment)
{
  static bool region_initialized = false;

  if (!region_initialized)
    {
      region_initialized = true; // set this first to avoid recursion
                                 // because region allocs a slab of its own
      region::init(reinterpret_cast<vm_offset_t>(&_mappings_1),
		   reinterpret_cast<vm_offset_t>(&_mappings_end_1));
    }
}

// Callback functions called by our super class, slab_cache_anon, to
// allocate or free blocks

virtual void *
kmem_slab_t::block_alloc(unsigned long size, unsigned long alignment)
{
  // size must be a power of two of PAGE_SIZE
  assert(size >= Config::PAGE_SIZE
	 && (size & (size - 1)) == 0); // only one bit set -> power of two

  // If the size is just one page, just allocate a page from kmem; do
  // not reserve a region for it.

  if (size == Config::PAGE_SIZE
      && alignment <= Config::PAGE_SIZE)
    {
      return Kmem_alloc::allocator()->alloc(0); // 2^0 = 1 page
    }

  vm_offset_t vaddr = region::reserve_pages(size, alignment);
  if (! vaddr)
    return 0;

  // Fine, we reserved virtual addresses for the buffer.  Now actually
  // allocate pages.
  for (vm_offset_t a = vaddr;
       a < vaddr + size;
       a += Config::PAGE_SIZE)
    {
      if (Vmem_alloc::page_alloc((void*)a, 0, Vmem_alloc::ZERO_FILL))
	continue;		// successfully allocated a page

      // Error - out of memory.  Undo everything.
      for (vm_offset_t i = vaddr;
	   i < a;
	   i += Config::PAGE_SIZE)
	{
	  Vmem_alloc::page_free((void*)i);
	}

      region::return_pages(vaddr, size);
      
      return 0;			// return error
    }

  return reinterpret_cast<void*>(vaddr);
}

virtual void 
kmem_slab_t::block_free(void *block, unsigned long size)
{
  for (char *p = reinterpret_cast<char*>(block);
       p < reinterpret_cast<char*>(block) + size;
       p += Config::PAGE_SIZE)
    {
      Kmem_alloc::allocator()->free(0,p);
    }

  // We didn't reserve a memory region if the allocation was just one
  // page.  Otherwise, we need to free the region.
  if (size != Config::PAGE_SIZE)	
    region::return_pages(reinterpret_cast<vm_offset_t>(block), size);
}
