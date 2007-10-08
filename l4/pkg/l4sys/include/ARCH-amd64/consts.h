/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-amd64/consts.h
 * \brief   Common L4 constants, amd64 version
 * \ingroup api_types_amd64
 */
/*****************************************************************************/
#ifndef __L4SYS__INCLUDE__ARCH_AMD64__CONSTS_H__
#define __L4SYS__INCLUDE__ARCH_AMD64__CONSTS_H__

/**
 * Sizeof a page in log2
 * \ingroup api_types_common
 */
#define L4_PAGESHIFT		12

/**
 * Sizeof a large page in log2
 * \ingroup api_types_common
 */
#define L4_SUPERPAGESHIFT	21

#include_next <l4/sys/consts.h>

#endif /* ! __L4SYS__INCLUDE__ARCH_AMD64__CONSTS_H__ */
