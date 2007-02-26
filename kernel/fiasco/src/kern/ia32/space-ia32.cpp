/*
 * Fiasco-ia32
 * Architecture specific pagetable code
 */

INTERFACE:

EXTENSION class Space
{
  // store IO bit counter somewhere in the IO bitmap page table (if present)
  static const unsigned long io_counter_pd_index
    = (Kmem::_unused4_io_1_addr >> PDESHIFT) & PDEMASK;
  static const unsigned long io_counter_pt_index
    = (Kmem::_unused4_io_1_addr >> PTESHIFT) & PTEMASK;
};

IMPLEMENTATION[ia32]:

#include <cassert>
#include "l4_types.h"
#include "kmem.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "vmem_alloc.h"


/*
 * The following functions are all no-ops on native ia32.
 * Pages appear in an address space when the corresponding PTE is made
 * ... unlike Fiasco-UX which needs these special tricks
 */

IMPLEMENT inline
void
Space::page_map (Address, Address, Address, unsigned)
{}

IMPLEMENT inline
void
Space::page_protect (Address, Address, unsigned)
{}

IMPLEMENT inline
void
Space::page_unmap (Address, Address)
{}

IMPLEMENT inline
void Space::kmem_update( void *addr )
{
  unsigned i = ((Address)addr >> PDESHIFT) & PDEMASK;
  _dir[i] = Kmem::dir()[i];
}

/*
 * Copy multiple PDEs between two address spaces.
 * Be aware that some PDEs are reserved and contain thread-local information
 * like space_index, chief number, I/O bitmap which must not be overwritten.
 * Callers are expected to know which slots are safe to copy.
 */
IMPLEMENT inline NEEDS [<cassert>, "kmem.h"]
void
Space::remote_update (const Address loc_addr, const Space_context *rem,
                      const Address rem_addr, size_t n)
{
  bool tlb_flush    = false;
  unsigned loc_slot = (loc_addr >> PDESHIFT) & PDEMASK,
           rem_slot = (rem_addr >> PDESHIFT) & PDEMASK;

  // PDE boundary check
  assert (loc_slot + n <= 1024);
  assert (rem_slot + n <= 1024);
  
  while (n--)
    {
      if (_dir[loc_slot])
        tlb_flush = true;
      
      // XXX: If we copy a PDE and it maps either a superpage with PDE_GLOBAL
      // or a pagetable with one or multiple PTE_GLOBAL entries, these won't
      // be flushed on a TLB flush. Suggested solution: When mapping a page
      // with global bit, a PDE_NO_LONGIPC bit should be set in the
      // corresponding PDE and this bit should be checked here before copying.

      _dir[loc_slot++] = rem->dir()[rem_slot++];
    }

  if (tlb_flush)
    Kmem::tlb_flush();
}

/* Update a page in the small space window from kmem.
 * @param flush true if TLB-Flush necessary
 */
IMPLEMENT
void
Space::update_small(Address addr, bool flush)
{
#ifdef CONFIG_SMALL_SPACES
    {
      if (is_small_space() &&
	  addr < small_space_size() )
	{
	  Kmem::update_smas_window( small_space_base() + addr,
					     _dir[(addr >> PDESHIFT) & PDEMASK],
					     flush );

	  asm volatile ("" : : : "memory");
	  current_space()->kmem_update((void*)(small_space_base() + addr));
	}
    }
#else
  (void)addr; (void)flush; // prevent gcc warning
#endif
}




PUBLIC
bool 
Space::vmem_range_malloc(Address virt_addr, 
			 int size, 
			 char fill = 0,  
			 unsigned int page_attributes = 
			 (Page_writable | Page_user_accessible) ) 
{
  
  
  Address va = virt_addr;
  int i;
  
  /* frank want it so */
  int pages = (size + Config::PAGE_SIZE - 1) / Config::PAGE_SIZE;

  for (i=0; i<pages; i++) 
    {
      void *page = Kmem_alloc::allocator()->alloc(0);

      if(!page)
	break;
    
      v_insert( Kmem::virt_to_phys(page), 
		va, 
		Config::PAGE_SIZE, 
		page_attributes);
      
      memset(page, fill, Config::PAGE_SIZE);

      va += Config::PAGE_SIZE;
    }
  
  if(i == pages) 
    return true;

  /* kdb_ke("vmem_range_alloc failed\n");  */
  /* cleanup */
  for( int j=i; j > 0; j--) 
    {
      Kmem_alloc::allocator()->
	free(0,reinterpret_cast<void *>(Space::virt_to_phys(va)) );

      va -= Config::PAGE_SIZE;  
    }
  return false;
}
