INTERFACE:

#include "fpu.h"

class Ram_quota;

class Fpu_alloc : public Fpu
{
};

IMPLEMENTATION:

#include "fpu_state.h"
#include "kmem_slab.h"
#include "ram_quota.h"
#include "ram_quota_alloc.h"
#include "slab_cache_anon.h"


PRIVATE inline static 
slab_cache_anon *
Fpu_alloc::slab_alloc()
{
  static slab_cache_anon *my_slab 
    = new Kmem_slab(Fpu::state_size() + sizeof(Ram_quota*),
	Fpu::state_align(), "Fpu state");
  return my_slab;
}

PUBLIC static
bool
Fpu_alloc::alloc_state(Ram_quota *q, Fpu_state *s) 
{
  unsigned long sz = Fpu::state_size();
  void *b;
  if (!(b = slab_alloc()->q_alloc(q)))
    return false;

  *((Ram_quota **)((char*)b + sz)) = q;
  s->_state_buffer = b;
  Fpu::init_state(s);

  return true;
}

PUBLIC static
void
Fpu_alloc::free_state(Fpu_state *s) 
{
  if (s->_state_buffer) 
    {
      unsigned long sz = Fpu::state_size();
      Ram_quota *q = *((Ram_quota **)((char*)(s->_state_buffer) + sz));
      slab_alloc()->q_free (q, s->_state_buffer);
      s->_state_buffer = 0;

      // transferred FPU state may leed to quotas w/o a task but only FPU 
      // contexts allocated
      if (q->current()==0)
	Ram_quota_alloc::free(q);
    }
}

