INTERFACE:

#include "mapped_alloc.h"
#include "helping_lock.h"

class Kmem_alloc : public Mapped_allocator
{
public:

  void *alloc( size_t );
  void free( size_t, void * );
  // g++ 2.95 sucks
  // using Mapped_allocator::free;
  void free( size_t, P_ptr<void> );

  void *Kmem_alloc::unaligned_alloc( int pages );
  void Kmem_alloc::unaligned_free( int pages, void *page );

protected:

  void *_phys_to_virt( void* ) const;
  void *_virt_to_phys( void* ) const;

  Kmem_alloc::Kmem_alloc();

public:

  static Kmem_alloc *allocator();

private:

  static void *const lmm;
  static Helping_lock lmm_lock;

};


IMPLEMENTATION:

#include <cstdio>
#include <cassert>

#include "kmem.h"
#include "lmm.h"
#include "config.h"



// implementation details 

static lmm_t        _lmm;

Helping_lock Kmem_alloc::lmm_lock;
void  *const Kmem_alloc::lmm = &_lmm;

IMPLEMENT inline
void Kmem_alloc::free( size_t o, P_ptr<void> p )
{
  Mapped_allocator::free(o,p);
}


IMPLEMENT
Kmem_alloc *Kmem_alloc::allocator()
{
  static Kmem_alloc al;
  return &al;
}

IMPLEMENT
void *Kmem_alloc::alloc( size_t o )
{
  void *ret;
  {
    Helping_lock_guard guard(&lmm_lock);
    ret = lmm_alloc_aligned(&_lmm, (1UL<<(o+Config::PAGE_SHIFT)), 0, 
			    o+Config::PAGE_SHIFT, 0);
  }
  return ret;
}

IMPLEMENT
void Kmem_alloc::free( size_t o, void *p )
{
  unaligned_free( (1UL<<o), p );
}

IMPLEMENT 
void *Kmem_alloc::unaligned_alloc( int pages )
{
  void* ret;

  {
    Helping_lock_guard guard(&lmm_lock);
    ret = lmm_alloc_aligned(&_lmm, pages * Config::PAGE_SIZE, 0, 0, 0);
  }
  return ret;
}

IMPLEMENT 
void Kmem_alloc::unaligned_free( int pages, void *page )
{
  Helping_lock_guard guard(&lmm_lock);
  lmm_free(&_lmm, page, pages * Config::PAGE_SIZE);
}

