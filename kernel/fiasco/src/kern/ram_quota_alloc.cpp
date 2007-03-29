INTERFACE:

#include "ram_quota.h"
#include "kmem_slab.h"

class Ram_quota;

class Ram_quota_alloc 
{
public:
  typedef slab_cache_anon Allocator;
};


IMPLEMENTATION:

PUBLIC static
Ram_quota *
Ram_quota_alloc::alloc(Ram_quota *q, unsigned long max)
{
  void *nq;
  if (q->alloc(sizeof(Ram_quota) + max) && (nq = allocator()->alloc()))
    return new (nq) Ram_quota(q, max);

  return 0;
}

PUBLIC
static 
void
Ram_quota_alloc::free(Ram_quota *q)
{
  if (q == Ram_quota::root)
    return;

  Ram_quota *p = q->parent();
  if (p)
    p->free(sizeof(Ram_quota)+q->limit());

  allocator()->free(q);
}

PRIVATE static inline NOEXPORT NEEDS["kmem_slab.h"]
Ram_quota_alloc::Allocator *
Ram_quota_alloc::allocator()
{
  static Allocator* slabs = 
    new Kmem_slab_simple (sizeof (Ram_quota), 
	sizeof (Mword), "Ram_quota");

  return slabs;
}

