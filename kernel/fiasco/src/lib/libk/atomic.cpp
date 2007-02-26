INTERFACE:

#include "types.h"

extern "C" void cas_error_type_with_bad_size_used(void);


#define MACRO_CAS_ASSERT(rs,cs) \
  if( (rs) != (cs) ) \
    cas_error_type_with_bad_size_used()



IMPLEMENTATION:

template< typename Type >
inline 
bool up_cas( Type *ptr, Type oldval, Type newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),sizeof(Mword));
  return up_cas_unsafe(reinterpret_cast<Mword*>(ptr), 
		       *reinterpret_cast<Mword*>(&oldval), 
		       *reinterpret_cast<Mword*>(&newval));
}



template< typename Type >
inline 
bool smp_cas( Type *ptr, Type oldval, Type newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),sizeof(Mword));
  return smp_cas_unsafe(reinterpret_cast<Mword*>(ptr), 
			*reinterpret_cast<Mword*>(&oldval), 
			*reinterpret_cast<Mword*>(&newval));
}



template< typename Type >
inline 
bool up_cas2( Type *ptr, Type *oldval, Type *newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),(sizeof(Mword)*2));
  return up_cas2_unsafe(reinterpret_cast<Mword*>(ptr), 
			reinterpret_cast<Mword*>(oldval), 
			reinterpret_cast<Mword*>(newval));
}



template< typename Type >
inline 
bool smp_cas2( Type *ptr, Type *oldval, Type *newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),(sizeof(Mword)*2));
  return smp_cas2_unsafe(reinterpret_cast<Mword*>(ptr), 
			 reinterpret_cast<Mword*>(oldval), 
			 reinterpret_cast<Mword*>(newval));
}


template <typename T> inline 
void atomic_add (T *p, T value)
{
  T old;
  do { old = *p; }
  while ( !smp_cas (p, old, old + value));
}

template <typename T> inline 
void atomic_sub (T *p, T value)
{
  T old;
  do { old = *p; }
  while ( !smp_cas (p, old, old - value));
}

template <typename T> inline 
void atomic_and (T *p, T value)
{
  T old;
  do { old = *p; }
  while ( !smp_cas (p, old, old & value));
}

template <typename T> inline 
void atomic_or (T *p, T value)
{
  T old;
  do { old = *p; }
  while ( !smp_cas (p, old, old | value));
}
