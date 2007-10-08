/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-arm/consts.h
 * \brief   Common L4 constants, arm version
 * \ingroup api_types_arm
 */
/*****************************************************************************/
#ifndef _L4_SYS_CONSTS_H
#define _L4_SYS_CONSTS_H

/* L4 includes */
#include <l4/sys/l4int.h>

/**
 * Sizeof a page in log2
 * \ingroup api_types_common
 */
#define L4_PAGESHIFT		12

/**
 * Sizeof a large page in log2
 * \ingroup api_types_common
 */
#define L4_SUPERPAGESHIFT	20

#include_next <l4/sys/consts.h>

#endif /* !_L4_SYS_CONSTS_H */
