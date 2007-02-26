
IMPLEMENTATION:

#include "kmem_slab_simple.h"
#include "slab_cache_anon.h"
#include "sched_context.h"

slab_cache_anon *Sched_context::_slabs = new Kmem_slab_simple
                                         (sizeof (Sched_context), 4);
