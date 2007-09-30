/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-amd64/consts.h
 * \brief   Common L4 constants, amd64 version
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_CONSTS_H
#define _L4_SYS_CONSTS_H

/* L4 includes */
#include <l4/sys/l4int.h>

/**
 * \ingroup api_types_common
 */
#define L4_PAGESHIFT		12

/**
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_PAGESIZE		(1UL << L4_PAGESHIFT)

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
#define L4_SUPERPAGESHIFT	21

/**
 * \ingroup api_types_common
 * \hideinitializer
 */
#define L4_SUPERPAGESIZE	(1UL << L4_SUPERPAGESHIFT)

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

#endif /* !_L4_SYS_CONSTS_H */
