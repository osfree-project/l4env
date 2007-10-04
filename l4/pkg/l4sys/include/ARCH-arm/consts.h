/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-arm/consts.h
 * \brief   Common L4 constants, arm version
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_CONSTS_H
#define _L4_SYS_CONSTS_H

/* L4 includes */
#include <l4/sys/l4int.h>

// what's this? // #define L4_ROOT_TASKNO 4

/**
 * \ingroup api_types_common
 */
#define L4_PAGESHIFT		12

/**
 * \ingroup api_types_common
 */
#define L4_SUPERPAGESHIFT	20

#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
					/* Page is flushed in own address */
					/* space too */

#define L4_NC_SAME_CLAN		0x00	/* destination resides within the */
					/* same clan */
#define L4_NC_INNER_CLAN	0x0C	/* destination is in an inner clan */
#define L4_NC_OUTER_CLAN	0x04	/* destination is outside the */
					/* invoker's clan */

#include_next <l4/sys/consts.h>

#endif /* !_L4_SYS_CONSTS_H */
