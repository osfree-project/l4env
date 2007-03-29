INTERFACE:

#include "mapped_alloc.h"
#include "helping_lock.h"
#include "initcalls.h"

class List_alloc;

class Kmem_alloc : public Mapped_allocator
{
  Kmem_alloc();

private:
  static Helping_lock lmm_lock;
  static List_alloc *a;
};


IMPLEMENTATION:

#include <cassert>

#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "list_alloc.h"

static List_alloc _a;
List_alloc *Kmem_alloc::a = &_a;
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
    ret = a->alloc(1UL << o, o);
  }
  
  if (!ret)
    {
      Mapped_alloc_reaper::morecore (/* despeate= */ true);
      
      Helping_lock_guard guard (&lmm_lock);
      ret = a->alloc(1UL << o, o);
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
Kmem_alloc::unaligned_alloc(int pages)
{
  void* ret;

  {
    Helping_lock_guard guard(&lmm_lock);
    ret = a->alloc(pages * Config::PAGE_SIZE, 0);
  }

  if (!ret)
    kdb_ke ("KERNEL: Out of memory!");

  return ret;
}

PUBLIC
void
Kmem_alloc::unaligned_free(int pages, void *page)
{
  Helping_lock_guard guard (&lmm_lock);
  a->free(page, pages * Config::PAGE_SIZE);
}

