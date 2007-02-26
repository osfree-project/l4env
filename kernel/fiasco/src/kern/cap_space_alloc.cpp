IMPLEMENTATION[caps]:

#include "cap_space.h"
#include "initcalls.h"
#include "kmem_slab_simple.h"
#include "static_init.h"

class Cap_space_alloc : public Cap_space
{
};

PUBLIC static inline
void 
Cap_space_alloc::allocator_init()
{
  static slab_cache_anon* slabs 
    = new Kmem_slab_simple (sizeof (Mword) * Cap_words, 
			    sizeof (Mword),
			    "Cap_space");
  // If Fiasco would kill all tasks even when exiting through the
  // kernel debugger, we could use a deallocating version of the above:
  //
  // static auto_ptr<slab_cache_anon> slabs
  //   (new Kmem_slab_simple (sizeof (Cap_space), sizeof (Mword)))
  // return slabs.get();

  _slabs = slabs;
}

STATIC_INITIALIZER(cap_space_alloc_init);
void cap_space_alloc_init() FIASCO_INIT;
void cap_space_alloc_init() { Cap_space_alloc::allocator_init(); }
