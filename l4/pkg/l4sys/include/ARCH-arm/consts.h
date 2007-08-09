/* $Id$ */
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

#define L4_ROOT_TASKNO 4

/**
 * \ingroup api_types_common 
 */
#define L4_PAGESHIFT		12

/**
 * \ingroup api_types_common 
 * \hideinitializer
 */
#define L4_PAGESIZE 		(1U << L4_PAGESHIFT)

/**
 * \ingroup api_types_common 
 * \hideinitializer
 */
#define L4_PAGEMASK		(~(L4_PAGESIZE - 1))

/**
 * \ingroup api_types_common 
 * \hideinitializer
 */
#define L4_LOG2_PAGESIZE	L4_PAGESHIFT


/**
 * \ingroup api_types_common
 */
#define L4_SUPERPAGESHIFT	20

/**
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_SUPERPAGESIZE	(1U << L4_SUPERPAGESHIFT)

/**
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_SUPERPAGEMASK	(~(L4_SUPERPAGESIZE - 1))

/**
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_LOG2_SUPERPAGESIZE	L4_SUPERPAGESHIFT

/**
 * Maximum address value
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_MAX_ADDRESS          ((l4_addr_t)-1)

/**
 * Get page address of \a x
 * \ingroup api_types_common
 * \hideinitializer
 *
 * \param   x            Address
 */
#define l4_trunc_page(x)        (((l4_addr_t)(x)) & L4_PAGEMASK)

/**
 * Round to next page address behind \a x
 * \ingroup api_types_common
 * \hideinitializer
 *
 * \param   x            Address
 */
#define l4_round_page(x) \
  ((((l4_addr_t)(x)) + L4_PAGESIZE-1) & L4_PAGEMASK)

/**
 * Get superpage address of \a x
 * \ingroup api_types_common
 * \hideinitializer
 *
 * \param   x            Address
 */
#define l4_trunc_superpage(x) \
  (((l4_addr_t)(x)) & L4_SUPERPAGEMASK)

/**
 * Round to next superpage address behind \a x
 * \ingroup api_types_common
 * \hideinitializer
 *
 * \param   x            Address
 */
#define l4_round_superpage(x) \
  ((((l4_addr_t)(x)) + L4_SUPERPAGESIZE-1) & L4_SUPERPAGEMASK)


#ifndef NULL
# define NULL ((void *)0)  /**< \ingroup api_types_common
			    **  \hideinitializer
			    **/
#endif


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

enum {
  L4_TASK_NEW_IPC_MONITOR     = 1UL << 29,
  L4_TASK_NEW_RAISE_EXCEPTION = 1UL << 30,
  L4_TASK_NEW_ALIEN           = 1UL << 31,
  L4_TASK_NEW_NR_OF_FLAGS     = 3,
  L4_TASK_NEW_FLAGS_MASK      = ((1 << L4_TASK_NEW_NR_OF_FLAGS) - 1)
                                  << (32 - L4_TASK_NEW_NR_OF_FLAGS),
};

enum {
  L4_THREAD_EX_REGS_RAISE_EXCEPTION = 1UL << 28,
  L4_THREAD_EX_REGS_ALIEN           = 1UL << 29,
  L4_THREAD_EX_REGS_NO_CANCEL       = 1UL << 30,
};

#endif /* !_L4_SYS_CONSTS_H */
