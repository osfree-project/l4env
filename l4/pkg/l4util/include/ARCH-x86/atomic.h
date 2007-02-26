/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/ARCH-x86/atomic.h
 * \brief   i386 atomic operations
 * \ingroup atomic
 *
 * \date    10/20/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de> */

/* Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _L4UTIL_ATOMIC_H
#define _L4UTIL_ATOMIC_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN;

/** \defgroup atomic Atomic Instructions */

/**
 * \brief Atomic compare and exchange (64 bit version) 
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 * 
 * \return 0 if comparison failed, 1 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to 
 * \em_val new
 */
L4_INLINE int
l4util_cmpxchg64(volatile l4_uint64_t * dest, 
		 l4_uint64_t cmp_val, l4_uint64_t new_val);

/**
 * \brief Atomic compare and exchange (32 bit version) 
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 * 
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to 
 * \em_val new
 */
L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
		 l4_uint32_t cmp_val, l4_uint32_t new_val);

/**
 * \brief Atomic compare and exchange (16 bit version) 
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 * 
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to 
 * \em_val new
 */
L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
		 l4_uint16_t cmp_val, l4_uint16_t new_val);

/**
 * \brief Atomic compare and exchange (8 bit version)
 * \ingroup atomic
 * 
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 * 
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to 
 * \em new_val
 */
L4_INLINE int 
l4util_cmpxchg8(volatile l4_uint8_t * dest,
		l4_uint8_t cmp_val, l4_uint8_t new_val);

/**
 * \brief Atomic exchange (32 bit version)
 * \ingroup atomic
 * 
 * \param  dest          destination operand
 * \param  val           new value for dest
 * 
 * \return old value at destination
 */
L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val);

/**
 * \brief Atomic exchange (16 bit version)
 * \ingroup atomic
 * 
 * \param  dest          destination operand
 * \param  val           new value for dest
 * 
 * \return old value at destination
 */
L4_INLINE l4_uint16_t
l4util_xchg16(volatile l4_uint16_t * dest, l4_uint16_t val);

/**
 * \brief Atomic exchange (8 bit version)
 * \ingroup atomic
 * 
 * \param  dest          destination operand
 * \param  val           new value for dest
 * 
 * \return old value at destination
 */
L4_INLINE l4_uint8_t
l4util_xchg8(volatile l4_uint8_t * dest, l4_uint8_t val);

EXTERN_C_END;


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

/* atomic compare and exchange 64 bit value */
L4_INLINE int
l4util_cmpxchg64(volatile l4_uint64_t * dest,
		 l4_uint64_t cmp_val, l4_uint64_t new_val)
{
  unsigned char ret;

  __asm__ __volatile__
    (
     "cmpxchg8b	%4\n\t"
     "sete	%0\n\t"
     :
     "=q" (ret)      /* return val, 0 or 1 */
     :
     "A"  (cmp_val),
     "c"  ((unsigned int)(new_val>>32ULL)),
     "b"  ((unsigned int)new_val),
     "m"  (*dest)    /* 3 mem, destination operand */
     : 
     "memory", "cc"
     );

  return ret;
}

/* atomic compare and exchange 32 bit value */
L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
		 l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  l4_uint32_t tmp;

  __asm__ __volatile__ 
    (
     "cmpxchgl %1, %3 \n\t"
     : 
     "=a" (tmp)      /* 0 EAX, return val */
     : 
     "r"  (new_val), /* 1 reg, new value */
     "0"  (cmp_val), /* 2 EAX, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     : 
     "memory", "cc" 
     );

  return tmp == cmp_val;
}

/* atomic compare and exchange 16 bit value */
L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
		 l4_uint16_t cmp_val, l4_uint16_t new_val)
{
  l4_uint16_t tmp;

  __asm__ __volatile__ 
    (
     "cmpxchgw %1, %3 \n\t"
     :
     "=a" (tmp)      /* 0  AX, return value */
     :
     "c"  (new_val), /* 1  CX, new value */
     "0"  (cmp_val), /* 2  AX, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     :
     "memory", "cc"
     );

  return tmp == cmp_val;
}

/* atomic compare and exchange 8 bit value */
L4_INLINE int 
l4util_cmpxchg8(volatile l4_uint8_t * dest,
		l4_uint8_t cmp_val, l4_uint8_t new_val)
{
  l4_uint8_t tmp;

  __asm__ __volatile__ 
    (
     "cmpxchgb %1, %3 \n\t"
     :
     "=a" (tmp)      /* 0  AL, return value */
     :
     "c"  (new_val), /* 1  CL, new value */
     "0"  (cmp_val), /* 2  AL, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     :
     "memory", "cc"
     );

  return tmp == cmp_val;
}

/* atomic exchange 32 bit value */
L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val)
{
  __asm__ __volatile__
    (
     "xchg %0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange 16 bit value */
L4_INLINE l4_uint16_t
l4util_xchg16(volatile l4_uint16_t * dest, l4_uint16_t val)
{
  __asm__ __volatile__
    (
     "xchg %w0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange 8 bit value */
L4_INLINE l4_uint8_t
l4util_xchg8(volatile l4_uint8_t * dest, l4_uint8_t val)
{
  __asm__ __volatile__
    (
     "xchg %b0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

#endif /* !_L4UTIL_ATOMIC_H */
