INTERFACE:

#include <cstddef> // size_t

#include "kern_types.h" // P_ptr


class Mapped_allocator
{
public:
  static size_t size_to_order( size_t size );

  /// allocate s bytes size-aligned
  virtual void *alloc( size_t order ) = 0;

  /// free s bytes previously allocated with alloc(s)
  virtual void free( size_t order, void *p ) = 0;

  /// free s bytes previously allocated with alloc(s) 
  /// and converted to phys address via virt_to_phys.
  void free( size_t order, P_ptr<void> p );

  template< typename _Ty >
  _Ty *phys_to_virt( P_ptr< _Ty > ) const;

  template< typename _Ty >
  P_ptr< _Ty > virt_to_phys( _Ty * ) const;

protected:

  virtual void *_phys_to_virt( void* ) const = 0;
  virtual void *_virt_to_phys( void* ) const = 0;


};


IMPLEMENTATION:

IMPLEMENT inline
size_t Mapped_allocator::size_to_order( size_t size )
{
  size_t h, x=1;
  for(h=0; x<size; ++h, x <<= 1);
  return h;
}


IMPLEMENT inline
void Mapped_allocator::free( size_t s, P_ptr<void> p )
{
  void *va = phys_to_virt(p);
  if(va)
    free(s, va);
}


IMPLEMENT inline
template< typename _Ty >
_Ty *Mapped_allocator::phys_to_virt( P_ptr< _Ty > p ) const 
{
  return (_Ty*)_phys_to_virt( (void*)p.get_raw() );
}

IMPLEMENT /*inline*/
template< typename _Ty >
P_ptr<_Ty> Mapped_allocator::virt_to_phys( _Ty *v ) const
{
  return (_Ty*)_virt_to_phys( (void*)v );
}

