#ifndef __ROOTTASK__KINFO_H__
#define __ROOTTASK__KINFO_H__

#include <l4/sys/compiler.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

EXTERN_C_BEGIN

/* return min. memory address */
l4_addr_t
root_kinfo_mem_low(void);

/* return max. memory address */
l4_addr_t
root_kinfo_mem_high(void);

EXTERN_C_END

#endif /* ! __ROOTTASK__KINFO_H__ */
