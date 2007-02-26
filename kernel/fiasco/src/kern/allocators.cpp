
IMPLEMENTATION:

#include "kmem_slab_simple.h"
#include "slab_cache_anon.h"
#include "sched_context.h"
#include "static_init.h"

STATIC_INITIALIZER(allocator_init);

void allocator_init() FIASCO_INIT;

void 
allocator_init()
{
  Sched_context::_slabs = new Kmem_slab_simple (sizeof (Sched_context), 4,
						"Sched_context");
}
