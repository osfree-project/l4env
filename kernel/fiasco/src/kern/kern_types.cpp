INTERFACE:

#include "types.h"

template< typename _Ty >
struct P_ptr 
{

public:

  typedef _Ty *Raw_type;
  typedef vm_offset_t Unsigned_type;


  P_ptr< _Ty > &operator ++ () // prefix
  {
    ++__ptr;
    return *this;
  }

  P_ptr< _Ty > &operator -- () // prefix
  {
    --__ptr;
    return *this;
  }

  P_ptr< _Ty > operator ++ (int) // postfix
  {
    return P_ptr(__ptr++);
  }

  P_ptr< _Ty > operator -- (int) // postfix
  {
    return P_ptr(__ptr--);
  }

  P_ptr< _Ty > operator + ( Smword pd ) const
  {
    return P_ptr(__ptr + pd);
  }

  P_ptr< _Ty > operator - ( Smword pd ) const
  {
    return P_ptr(__ptr - pd);
  }

  P_ptr< _Ty > &operator += ( Smword pd )
  {
    __ptr += pd;
    return *this;
  }

  P_ptr< _Ty > &operator -= ( Smword pd )
  {
    __ptr -= pd;
    return *this;
  }

  P_ptr< _Ty > operator & ( Mword o )
  {
    return (_Ty*)((Mword)__ptr & o);
  }

  bool operator == ( P_ptr const &o ) const
  {
    return (void*)__ptr == (void*)o.__ptr;
  }

  bool operator != ( P_ptr const &o ) const
  {
    return (void*)__ptr != (void*)o.__ptr;
  }

  bool is_null() const
  {
    return __ptr == (void*)(-1);
  }

  bool is_invalid() const
  {
    return __ptr == (void*)(-1);
  }



  _Ty* get_raw() const 
  {
    return __ptr;
  }

  Unsigned_type get_unsigned() const
  {
    return (Unsigned_type)__ptr;
  }


  P_ptr<void> to_void() const 
  {
    return P_ptr<void>((void*)__ptr);
  }


  P_ptr< _Ty > operator = (_Ty *o)
  {
    return P_ptr(__ptr= o);
  }

  P_ptr< _Ty > operator = (P_ptr<_Ty> const &o)
  {
    return P_ptr(__ptr= o.__ptr);
  }

  template< typename _To >
  static P_ptr cast( P_ptr< _To > const &o)
  {
    return P_ptr<_Ty>(static_cast<_Ty*>(o.get_raw()));
  }
  

  explicit P_ptr( Mword addr ) 
    : __ptr((_Ty*)addr)
  {}


  P_ptr( _Ty *p = (_Ty*)(-1) ) 
    : __ptr(p)
  {}

#if 0
  // avoid this copy constructor so the gcc does more optimization
  P_ptr( P_ptr<_Ty> const &o )
    : __ptr(o.get_raw())
  {}
#endif

  template< typename _To >
  P_ptr( P_ptr<_To> const &o )
    : __ptr(o.get_raw())
  {}

private:

  _Ty *__ptr;


};





IMPLEMENTATION:
