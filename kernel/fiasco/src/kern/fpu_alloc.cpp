INTERFACE:

#include "fpu.h"

class Fpu_alloc : public Fpu
{
public:
  static void alloc_state(Fpu_state *);
  static void free_state(Fpu_state *);

};

IMPLEMENTATION:

#include "fpu_state.h"
#include "kmem_slab_simple.h"
#include "slab_cache_anon.h"
#include "kdb_ke.h"

PRIVATE inline //NEEDS["kmem_slab_simple.h"]
static 
slab_cache_anon *Fpu_alloc::slab_alloc()
{
  static slab_cache_anon *my_slab 
    = new Kmem_slab_simple(Fpu::state_size(),Fpu::state_align());
  return my_slab;
}

IMPLEMENT
void Fpu_alloc::alloc_state(Fpu_state *s) 
{
  s->_state_buffer = slab_alloc()->alloc();

  if (!s->_state_buffer)
    kdb_ke ("out of slab memory!");

  Fpu::init_state(s);
}

IMPLEMENT
void Fpu_alloc::free_state(Fpu_state *s) 
{
  if (s->_state_buffer) {
    slab_alloc()->free (s->_state_buffer);
    s->_state_buffer = 0;
  }
}
