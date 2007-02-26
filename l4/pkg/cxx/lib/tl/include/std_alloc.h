#ifndef STD_ALLOC_H__
#define STD_ALLOC_H__

#include <cstddef>

inline void *operator new (size_t, void *mem)
{ return mem; }

namespace cxx {

template< typename _Type >
class New_allocator
{
public:
  enum { can_free = true };

  New_allocator() {}
  New_allocator(New_allocator const &) {}

  ~New_allocator() {}
  
  _Type *alloc() 
  { return static_cast<_Type*>(::operator new(sizeof (_Type))); }
  
  void free(_Type *t) 
  { ::operator delete(t); }
};

}

#endif
