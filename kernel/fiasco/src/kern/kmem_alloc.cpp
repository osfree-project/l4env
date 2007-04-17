INTERFACE:

#include "mapped_alloc.h"
#include "helping_lock.h"
#include "initcalls.h"

class Buddy_alloc;

class Kmem_alloc : public Mapped_allocator
{
  Kmem_alloc();

public:
  typedef Buddy_alloc Alloc;
private:
  static Helping_lock lmm_lock;
  static Alloc *a;
};


IMPLEMENTATION:

#include <cassert>

#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "buddy_alloc.h"

static Kmem_alloc::Alloc _a;
Kmem_alloc::Alloc *Kmem_alloc::a = &_a;
Helping_lock Kmem_alloc::lmm_lock;

PUBLIC static FIASCO_INIT
void
Kmem_alloc::init()
{
  static Kmem_alloc al;
  Mapped_allocator::allocator(&al);
}

PUBLIC
void
Kmem_alloc::dump() const
{ a->dump(); }

PUBLIC
void *
Kmem_alloc::alloc(size_t o)
{
  assert(o>=8 /*NEW INTERFACE PARANIOIA*/);
  void *ret;
  {
    Helping_lock_guard guard (&lmm_lock);
    ret = a->alloc(1UL << o);
  }
  
  if (!ret)
    {
      Mapped_alloc_reaper::morecore (/* despeate= */ true);
      
      Helping_lock_guard guard (&lmm_lock);
      ret = a->alloc(1UL << o);
    }

  return ret;
}


PUBLIC
void
Kmem_alloc::free(size_t o, void *p)
{
  assert(o>=8 /*NEW INTERFACE PARANIOIA*/);
  Helping_lock_guard guard (&lmm_lock);
  a->free(p, 1UL << o);
}

PUBLIC 
void *
Kmem_alloc::unaligned_alloc(unsigned long size)
{
  void* ret;

  {
    Helping_lock_guard guard(&lmm_lock);
    ret = a->alloc(size);
  }

  if (!ret)
    kdb_ke ("KERNEL: Out of memory!");

  return ret;
}

PUBLIC
void
Kmem_alloc::unaligned_free(unsigned long size, void *page)
{
  Helping_lock_guard guard (&lmm_lock);
  a->free(page, size);
}

