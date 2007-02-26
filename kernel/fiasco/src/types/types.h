#ifndef TYPES_H__
#define TYPES_H__

#if(__GNUC__<3)
# define MARK_AS_DEPRECATED /*empty*/
#else
# define MARK_AS_DEPRECATED __attribute__((deprecated))
#endif


#include <stddef.h>
#include "types-arch.h"

#ifdef __cplusplus

template< typename a, typename b > inline
a nonull_static_cast( b p )
{
  int d = reinterpret_cast<int>(static_cast<a>(reinterpret_cast<b>(10))) - 10;
  return reinterpret_cast<a>( reinterpret_cast<Mword>(p) + d );
}

#endif

/// OBSOLETE IA-32 64bit type
typedef struct { Unsigned32 low, high; } l4_low_high_t; 

/// type for address ranges
typedef struct { Address low, high; } l4_addr_range_t;

/// standard size type
///typedef mword_t size_t;
typedef Smword ssize_t;

#endif // TYPES_H__
