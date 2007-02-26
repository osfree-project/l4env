INTERFACE:

#include <stddef.h>		// size_t
#include "helping_lock.h"	// Helping_lock

#include "slab_cache_anon.h"		// slab_cache_anon

class kmem_slab_simple_t : public slab_cache_anon
{
  // DATA
  Helping_lock _lock;
};

IMPLEMENTATION:

// kmem_slab_simple_t -- A type-independent slab cache allocator for Fiasco,
// derived from a generic slab cache allocator (slab_cache_anon in
// lib/slab.cpp).

// This specialization adds low-level page allocation and locking to
// the slab allocator implemented in our base class (slab_cache_anon).
//-

#include <cassert>

#include "panic.h"
#include "config.h"
#include "kmem_alloc.h"

// OSKIT crap
#include "undef_oskit.h"

// We only support slab size == PAGE_SIZE.
PUBLIC
kmem_slab_simple_t::kmem_slab_simple_t(unsigned elem_size, 
				       unsigned alignment)
  : slab_cache_anon(Config::PAGE_SIZE, elem_size, alignment)
{
}

// Specializations providing their own block_alloc()/block_free() can
// also request slab sizes larger than one page.
PROTECTED
kmem_slab_simple_t::kmem_slab_simple_t(unsigned long slab_size, 
				       unsigned elem_size, 
				       unsigned alignment)
  : slab_cache_anon(slab_size, elem_size, alignment)
{}

PUBLIC
kmem_slab_simple_t::~kmem_slab_simple_t()
{
  Helping_lock_guard guard(&_lock);
  destroy();
}

// We overwrite some of slab_cache_anon's functions to faciliate locking.
PUBLIC
void *
kmem_slab_simple_t::alloc()		// request initialized member from cache
{
  Helping_lock_guard guard(&_lock);
  return slab_cache_anon::alloc();
}

PUBLIC
void 
kmem_slab_simple_t::free(void *cache_entry) // return initialized member to cache
{
  Helping_lock_guard guard(&_lock);
  slab_cache_anon::free(cache_entry);
}

PUBLIC
bool 
kmem_slab_simple_t::reap()
{
  if (_lock.test())
    return false;		// this cache is locked -- can't get memory now

  Helping_lock_guard guard(&_lock);
  return slab_cache_anon::reap();
}

// Callback functions called by our super class, slab_cache_anon, to
// allocate or free blocks

virtual void *
kmem_slab_simple_t::block_alloc(unsigned long size, unsigned long)
{
  // size must be exactly PAGE_SIZE
  assert(size == Config::PAGE_SIZE);
  (void)size;

  return Kmem_alloc::allocator()->alloc(0);
}

virtual void 
kmem_slab_simple_t::block_free(void *block, unsigned long)
{
  Kmem_alloc::allocator()->free(0,block);
}

// memory management

static void *slab_mem = 0;

PUBLIC
void *
kmem_slab_simple_t::operator new(size_t size)
{
//#warning do we really need dynamic allocation of slab allocators?
  assert(size<=sizeof(kmem_slab_simple_t));
  (void)size; // prevent gcc warning
  if(!slab_mem)
    {
      slab_mem = Kmem_alloc::allocator()->alloc(0);
      if(!slab_mem)
	panic("Out of memory (new kmem_slab_simple_t)");

      char* s;
      for( s = (char*)slab_mem; 
	  s < ((char*)slab_mem) + Config::PAGE_SIZE - sizeof(kmem_slab_simple_t); 
	  s+=sizeof(kmem_slab_simple_t) ) 
	{
	  *((void**)s) = s+sizeof(kmem_slab_simple_t);
	}

      *((void**)s) = 0;
    }

  void *sl = slab_mem;
  slab_mem = *((void**)slab_mem);
  return sl;
  
}

PUBLIC
void 
kmem_slab_simple_t::operator delete(void *block, size_t size)
{
  assert(size<=sizeof(kmem_slab_simple_t));
  (void)size; // prevent gcc warning
  *((void**)block) = slab_mem;
  slab_mem = block;
}

