/*!
 * \file    l4sys/include/consts.h
 * \brief   Common constants
 * \ingroup api_types
 */
#ifndef __L4_SYS__INCLUDE__CONSTS_H__
#define __L4_SYS__INCLUDE__CONSTS_H__


/**
 * \ingroup api_types_common
 * \hideinitializer
 * Page size
 */
#define L4_PAGESIZE		(1UL << L4_PAGESHIFT)

/**
 * \ingroup api_types_common
 * \hideinitializer
 * Page mask (upper bits are 1)
 */
#define L4_PAGEMASK		(~(L4_PAGESIZE - 1))

/**
 * \ingroup api_types_common
 * \hideinitializer
 * Size of page in log2
 */
#define L4_LOG2_PAGESIZE	L4_PAGESHIFT

/**
 * \ingroup api_types_common
 * \hideinitializer
 * Size of large page
 */
#define L4_SUPERPAGESIZE	(1UL << L4_SUPERPAGESHIFT)

/**
 * \ingroup api_types_common
 * \hideinitializer
 * Mask of large page (upper bits are 1)
 */
#define L4_SUPERPAGEMASK	(~(L4_SUPERPAGESIZE - 1))

/**
 * \ingroup api_types_common
 * \hideinitializer
 * Size of large page in log2
 */
#define L4_LOG2_SUPERPAGESIZE	L4_SUPERPAGESHIFT

/**
 * Maximum address value
 * \ingroup api_types_common
 * \hideinitializer
 * Max address
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
#ifndef __cplusplus
# define NULL ((void *)0)  /**< \ingroup api_types_common
			    **  \hideinitializer
			    ** NULL
			    **/
#else
# define NULL 0
#endif
#endif

#endif /* ! __L4_SYS__INCLUDE__CONSTS_H__ */
