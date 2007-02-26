#ifndef TYPES_H__
#define TYPES_H__

#include <stddef.h>
#include "types-arch.h"
#include "std_macros.h"

#ifdef __cplusplus

template< typename a, typename b > inline
a nonull_static_cast( b p )
{
  Address d = reinterpret_cast<Address>
                 (static_cast<a>(reinterpret_cast<b>(10))) - 10;
  return reinterpret_cast<a>( reinterpret_cast<Address>(p) + d);
}

#endif

/// OBSOLETE IA-32 64bit type
typedef struct { Unsigned32 low, high; } l4_low_high_t;

typedef struct { Address start, end; } Mem_region;

/// standard size type
///typedef mword_t size_t;
typedef signed int ssize_t;

/// momentary only used in UX since there the kernel has a different
/// address space than user mode applications
enum Address_type { ADDR_KERNEL = 0, ADDR_USER = 1, ADDR_UNKNOWN = 2 };

#endif // TYPES_H__

