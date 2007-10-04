/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/consts.h
 * \brief   Common L4 constants, x86 version
 * \ingroup api_types_x86
 */
/*****************************************************************************/
#ifndef __L4SYS__INCLUDE__ARCH_X86__CONSTS_H__
#define __L4SYS__INCLUDE__ARCH_X86__CONSTS_H__

/**
 * Sizeof a page in log2
 * \ingroup api_types_x86
 */
#define L4_PAGESHIFT		12

/**
 * Sizeof a large page in log2
 * \ingroup api_types_x86
 */
#define L4_SUPERPAGESHIFT	22

#include_next <l4/sys/consts.h>

#endif /* ! __L4SYS__INCLUDE__ARCH_X86__CONSTS_H__ */
