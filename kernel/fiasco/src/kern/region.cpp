INTERFACE:

#include "types.h"

/** Region manager.
    The region manager is a module that keeps track of virtual-memory
    allocations in a predefined virtual-address region.  It only
    allocates virtual addresses -- not physical memory.  In other
    words, virtual-memory regions allocated using this module are not
    backed by physical memory; mapping in physical memory is the
    client's responsibility.
 */
class region
{
};

IMPLEMENTATION:

#include "globals.h"
#include "kmem_slab_simple.h"
#include "helping_lock.h"
#include "amm.h"
#include "panic.h"

static vm_offset_t mem_alloc_region;
static vm_offset_t mem_alloc_region_end;

// 
// helpers for the address map library
// 

static kmem_slab_simple_t *amm_entry_cache;

static amm_entry_t *amm_alloc_func(amm_t *, vm_offset_t, vm_size_t, int)
{
  return reinterpret_cast<amm_entry_t *>(amm_entry_cache->alloc());
}

static void amm_free_func(amm_t *, amm_entry_t *entry)
{
  amm_entry_cache->free(entry);
}

// 
// region
// 

static amm_t region_amm;
static vm_offset_t end_of_last_region;
static Helping_lock region_lock;

/** Initialize the region manager.  This function is called once at
    initialization.
    @param begin begin of the virtual-memory region
    @param end end of the virtual-memory region
 */
PUBLIC static void
region::init (vm_offset_t begin, vm_offset_t end)
{
  mem_alloc_region = begin;
  mem_alloc_region_end = end;

  // Make sure our slab cache only uses single-page slabs (slab_size =
  // PAGE_SIZE).  See note above declaration of amm_entry_cache for
  // more information.
  amm_entry_cache = new kmem_slab_simple_t(sizeof(amm_entry_t), 4);

  amm_init_gen(&region_amm, AMM_FREE, 0, amm_alloc_func, amm_free_func, 0, 0);
  check ( amm_modify(&region_amm, 0, mem_alloc_region, AMM_RESERVED, 0) == 0 );
  check ( amm_modify(&region_amm, mem_alloc_region_end, 
		     AMM_MAXADDR - mem_alloc_region_end, 
		     AMM_RESERVED, 0) == 0 );

  end_of_last_region = mem_alloc_region;
}
 

/** Reserve an address region.  This function only reserves the region -- 
    it does not back it with physical memory.
    @param size      size of the requested region, in bytes.  
    @param alignment size-alignment of the requested region, in bytes.
    @return virtual address of the allocated virtual-memory region, 
            or 0 if an error occurred.
 */
PUBLIC static vm_offset_t
region::reserve_pages(vm_size_t size, unsigned long alignment)
{
  Helping_lock_guard guard(&region_lock);

  vm_offset_t address = end_of_last_region;

  int align_bits = 0;
  while ((alignment >>= 1) != 0)
    align_bits++;

  if (! amm_find_gen(&region_amm, &address, size, AMM_FREE, -1, align_bits,
		     0, 0))
    {
      return 0;			// nothing found
    }

  end_of_last_region = address + size;

  if (amm_modify(&region_amm, address, size, AMM_ALLOCATED, 0) != 0)
    {
      return 0;			// error
    }

  return address;
}

/** Free an address region.  This function only frees a reservation ---
    it does not flush the region itself.
    @param address   virtual address of the allocated virtual memory region.
    @param size      size of the allocated region, in bytes.  
 */
PUBLIC static void 
region::return_pages(vm_offset_t address, vm_size_t size)
{
  Helping_lock_guard guard(&region_lock);

  assert( amm_find_gen(&region_amm, &address, size, AMM_ALLOCATED, -1,
		       0, 0, AMM_EXACT_ADDR) );

  check ( amm_modify(&region_amm, address, size, AMM_FREE, 0) != 0);
}

/** Dump an overview of current allocations to the screen.
 */
PUBLIC static void
region::debug_dump()
{
  amm_dump(& region_amm);
}
