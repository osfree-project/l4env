INTERFACE:

#include <cstddef> // size_t

#include "kern_types.h" // P_ptr


class Mapped_allocator
{
public:
  /// allocate s bytes size-aligned
  virtual void *alloc(size_t order) = 0;

  /// free s bytes previously allocated with alloc(s)
  virtual void free(size_t order, void *p) = 0;

  virtual void *unaligned_alloc(int pages) = 0;
  virtual void unaligned_free(int pages, void *p) = 0;
private:
  static Mapped_allocator *_alloc;
};


IMPLEMENTATION:

#include <cassert>

#include "mem_layout.h"

Mapped_allocator *Mapped_allocator::_alloc;

PUBLIC static
Mapped_allocator *
Mapped_allocator::allocator()
{
  assert (_alloc /* uninitialized use of Mapped_allocator */);
  return _alloc;
}

PROTECTED static
void
Mapped_allocator::allocator(Mapped_allocator *a)
{
  _alloc=a;
}

PUBLIC inline NEEDS["mem_layout.h"]
void Mapped_allocator::free_phys(size_t s, P_ptr<void> p)
{
  void *va = (void*)Mem_layout::phys_to_pmem(p.get_unsigned());
  if(va)
    free(s, va);
}

