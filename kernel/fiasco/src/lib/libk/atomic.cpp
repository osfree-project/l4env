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
  return up_cas_unsave(reinterpret_cast<Mword*>(ptr), 
		       *reinterpret_cast<Mword*>(&oldval), 
		       *reinterpret_cast<Mword*>(&newval));
}



template< typename Type >
inline 
bool smp_cas( Type *ptr, Type oldval, Type newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),sizeof(Mword));
  return smp_cas_unsave(reinterpret_cast<Mword*>(ptr), 
			*reinterpret_cast<Mword*>(&oldval), 
			*reinterpret_cast<Mword*>(&newval));
}



template< typename Type >
inline 
bool up_cas2( Type *ptr, Type *oldval, Type *newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),(sizeof(Mword)*2));
  return up_cas2_unsave(reinterpret_cast<Mword*>(ptr), 
			reinterpret_cast<Mword*>(oldval), 
			reinterpret_cast<Mword*>(newval));
}



template< typename Type >
inline 
bool smp_cas2( Type *ptr, Type *oldval, Type *newval )
{
  MACRO_CAS_ASSERT(sizeof(Type),(sizeof(Mword)*2));
  return smp_cas2_unsave(reinterpret_cast<Mword*>(ptr), 
			 reinterpret_cast<Mword*>(oldval), 
			 reinterpret_cast<Mword*>(newval));
}
