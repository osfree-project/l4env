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

/// standard size type
///typedef mword_t size_t;
typedef Smword ssize_t;

/// momentary only used in UX since there the kernel has a different
/// address space than user mode applications
enum Address_type { KERNEL = 0, USER = 1, UNKNOWN = 2 };

#endif // TYPES_H__

