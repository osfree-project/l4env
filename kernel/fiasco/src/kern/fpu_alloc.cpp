INTERFACE:

#include "fpu.h"

class Fpu_alloc : public Fpu
{
};

IMPLEMENTATION:

#include "fpu_state.h"
#include "kmem_slab_simple.h"
#include "slab_cache_anon.h"
#include "kdb_ke.h"

PRIVATE inline static 
slab_cache_anon *
Fpu_alloc::slab_alloc()
{
  static slab_cache_anon *my_slab 
    = new Kmem_slab_simple(Fpu::state_size(),Fpu::state_align());
  return my_slab;
}

PUBLIC static
void
Fpu_alloc::alloc_state(Fpu_state *s) 
{
  s->_state_buffer = slab_alloc()->alloc();

  if (!s->_state_buffer)
    kdb_ke ("out of slab memory!");

  Fpu::init_state(s);
}

PUBLIC static
void
Fpu_alloc::free_state(Fpu_state *s) 
{
  if (s->_state_buffer) {
    slab_alloc()->free (s->_state_buffer);
    s->_state_buffer = 0;
  }
}
