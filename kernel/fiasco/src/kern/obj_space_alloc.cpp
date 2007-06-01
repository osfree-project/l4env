IMPLEMENTATION[caps]:

#include "obj_space.h"
#include "initcalls.h"
#include "kmem_slab_simple.h"
#include "static_init.h"

class Obj_space_alloc : public Obj_space
{
protected:
  Obj_space_alloc() : Obj_space(0) {}
};

PUBLIC static inline
void 
Obj_space_alloc::allocator_init()
{
  static slab_cache_anon* slabs 
    = new Kmem_slab_simple (sizeof (Capability) * Cap_words, 
			    sizeof (Capability),
			    "X_cap_space");
  // If Fiasco would kill all tasks even when exiting through the
  // kernel debugger, we could use a deallocating version of the above:
  //
  // static auto_ptr<slab_cache_anon> slabs
  //   (new Kmem_slab_simple (sizeof (X_cap_space), sizeof (Mword)))
  // return slabs.get();

  _slabs = slabs;
}

STATIC_INITIALIZER(obj_space_alloc_init);
void obj_space_alloc_init() FIASCO_INIT;
void obj_space_alloc_init() { Obj_space_alloc::allocator_init(); }

