INTERFACE:

#include "l4_types.h"
#include "space.h"

EXTENSION class Task
{
public:

  /** Constructor.
   * @param utcb_area Fpage where the UTCBs shall be mapped into.
   * @param kip_location Fpage where the KIP shall be mapped.
   */
  explicit Task (L4_fpage utcb_area, L4_fpage kip_location);
};


IMPLEMENTATION[v4]:

#include <cstdio>

#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "l4_types.h"
#include "map_util.h"

IMPLEMENT 
Task::Task (L4_fpage utcb_area, L4_fpage kip_location)
  : Space (utcb_area, kip_location)
{
  if (!sigma0) sigma0 = this;
	
  // map the KIP
  mem_map (sigma0,				// from: space
	   Kmem::virt_to_phys(Kmem::info()),	// from: address
	   Config::PAGE_SHIFT,			// from: size
	   0, 0,				// write, grant
	   nonull_static_cast<Space*>(this),	// to: space
	   kip_location.page(),			// to: address
	   Config::PAGE_SHIFT,			// to: size
	   0);					// to: offset

  // allocate and map the UTCB area
  for (Address va = utcb_area.page(); 
       va < utcb_area.page() + (1 << utcb_area.size());
       va += Config::PAGE_SIZE)
    mem_map (sigma0,			// look at comments above!
 	     Kmem::virt_to_phys (Kmem_alloc::allocator()->alloc(0)),
 	     Config::PAGE_SHIFT,
 	     1, 0,
 	     nonull_static_cast<Space*>(this),
 	     va, 
 	     Config::PAGE_SHIFT,
 	     0);
}

