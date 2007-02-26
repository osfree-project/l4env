/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/ARCH-x86/bitops.h
 * \brief   x86 bit manipulation functions
 * \ingroup bitops
 *
 * \date    07/03/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de> */

/*
 * Copyright (C) 2000-2002
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
#ifndef _L4UTIL_BITOPS_H
#define _L4UTIL_BITOPS_H

/* L4 includes */
#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>

/** define some more usual names */
#define l4util_test_and_clear_bit(b, dest)	l4util_btr(b, dest)
#define l4util_test_and_set_bit(b, dest)	l4util_bts(b, dest)
#define l4util_test_and_change_bit(b, dest)	l4util_btc(b, dest)
#define l4util_log2(word)			l4util_bsr(word)

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN;

/** \defgroup bitops Bit Manipulation Functions */

/**
 * \brief Set bit in memory
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_set_bit(int b, volatile l4_uint32_t * dest);

/**
 * \brief Clear bit in memory
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_clear_bit(int b, volatile l4_uint32_t * dest);

/**
 * \brief Complement bit in memory
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_complement_bit(int b, volatile l4_uint32_t * dest);

/**
 * \brief Test bit (return value of bit)
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 *	
 * \return Value of bit \em b.
 */
L4_INLINE int
l4util_test_bit(int b, volatile l4_uint32_t * dest);

/**
 * \brief Bit test and set
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 *	
 * \return Old value of bit \em b.
 *
 * Set the \em b bit of \em dest to 1 and return the old value.
 */
L4_INLINE int 
l4util_bts(int b, volatile l4_uint32_t * dest);

/**
 * \brief Bit test and reset
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 *	
 * \return Old value of bit \em b.
 *
 * Rest bit \em b and return old value.
 */
L4_INLINE int
l4util_btr(int b, volatile l4_uint32_t * dest);

/**
 * \brief Bit test and complement
 * \ingroup bitops
 * 
 * \param  b             bit position
 * \param  dest          destination operand
 *	
 * \return Old value of bit \em b.
 *
 * Complement bit \em b and return old value.
 */
L4_INLINE int
l4util_btc(int b, volatile l4_uint32_t * dest);

/**
 * \brief Bit scan reverse
 * \ingroup bitops
 * 
 * \param  word          32 bit value
 *	
 * \return index of most significant set bit in word, 
 *         -1 if no bit is set (word == 0) 
 *
 * "bit scan reverse", find most significant set bit in word (-> LOG2(word))
 */
L4_INLINE int
l4util_bsr(l4_uint32_t word);

/**
 * \brief Bit scan forward
 * \ingroup bitops
 * 
 * \param  word          32 bit value
 *	
 * \return index of least significant bit set in word, 
 *         -1 if no bit is set (word == 0)
 *
 * "bit scan forward", find least significant bit set in word.
 */
L4_INLINE int
l4util_bsf(l4_uint32_t word);

/**
 * \brief Find the first set bit in a memory region
 * \ingroup bitops
 * 
 * \param  dest          bitstring
 * \param  size          size of string in bits
 *
 * \return number of the first set bit,
 *         > size if no bit is set
 */
L4_INLINE int
l4util_find_first_set_bit(void * dest, l4_size_t size);

/**
 * \brief Find the first zero bit in a memory region
 * \ingroup bitops
 * 
 * \param  dest          bitstring
 * \param  size          size of string in bits
 *
 * \return number of the first zero bit, 
 *         > size if no bit is set
 */
L4_INLINE int
l4util_find_first_zero_bit(void * dest, l4_size_t size);


EXTERN_C_END;

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

/* set bit */
L4_INLINE void
l4util_set_bit(int b, volatile l4_uint32_t * dest)
{
  __asm__ __volatile__
    (
     "btsl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "Ir"  (b)       /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* clear bit */
L4_INLINE void
l4util_clear_bit(int b, volatile l4_uint32_t * dest)
{
  __asm__ __volatile__
    (
     "btrl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "Ir"  (b)        /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* change bit */
L4_INLINE void
l4util_complement_bit(int b, volatile l4_uint32_t * dest)
{
  __asm__ __volatile__
    (
     "btcl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "Ir"  (b)        /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* test bit */
L4_INLINE int
l4util_test_bit(int b, volatile l4_uint32_t * dest)
{
  l4_int8_t bit;

  __asm__ __volatile__
    (
     "btl   %2,%1   \n\t"
     "setc  %0      \n\t"
     :
     "=r"  (bit)      /* 0,     old bit value */
     :
     "m"   (*dest),   /* 1 mem, destination operand */ 
     "Ir"  (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and set */
L4_INLINE int 
l4util_bts(int b, volatile l4_uint32_t * dest)
{
  l4_int8_t bit;

  __asm__ __volatile__
    (
     "btsl  %2,%1   \n\t"
     "setc  %0      \n\t"
     :
     "=r"  (bit)      /* 0,     old bit value */
     :
     "m"   (*dest),   /* 1 mem, destination operand */ 
     "Ir"  (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and reset */
L4_INLINE int
l4util_btr(int b, volatile l4_uint32_t * dest)
{
  l4_int8_t bit;

  __asm__ __volatile__
    (
     "btrl  %2,%1   \n\t"
     "setc  %0      \n\t"
     :
     "=r"  (bit)      /* 0,     old bit value */
     :
     "m"   (*dest),   /* 1 mem, destination operand */ 
     "Ir"  (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and complement */
L4_INLINE int
l4util_btc(int b, volatile l4_uint32_t * dest)
{
  l4_int8_t bit;

  __asm__ __volatile__
    (
     "btcl  %2,%1   \n\t"
     "setc  %0      \n\t"
     : 
     "=r"  (bit)      /* 0,     old bit value */
     :
     "m"   (*dest),   /* 1 mem, destination operand */ 
     "Ir"  (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit scan reverse */
L4_INLINE int
l4util_bsr(l4_uint32_t word)
{
  int tmp;

  if (word == 0)
    return -1;

  __asm__ __volatile__
    (
     "bsrl %1,%0 \n\t"
     :
     "=r" (tmp)       /* 0, index of most significant set bit */
     :
     "r"  (word)      /* 1, argument */
     );

  return tmp;
}

/* bit scan forwad */
L4_INLINE int
l4util_bsf(l4_uint32_t word)
{
  int tmp;

  if (word == 0)
    return -1;

  __asm__ __volatile__
    (
     "bsfl %1,%0 \n\t"
     :
     "=r" (tmp)       /* 0, index of least significant set bit */
     :
     "r"  (word)      /* 1, argument */
     );

  return tmp;
}

L4_INLINE int
l4util_find_first_set_bit(void * dest, l4_size_t size)
{
  l4_mword_t dummy0, dummy1, res;

  __asm__ __volatile__
    (
     "xorl  %%eax,%%eax		\n\t"
     "repe; scasl		\n\t"
     "jz    1f			\n\t"
     "leal  -4(%%edi),%%edi	\n\t"
     "bsfl  (%%edi),%%eax	\n"
     "1:			\n\t"
     "subl  %%ebx,%%edi		\n\t"
     "shll  $3,%%edi		\n\t"
     "addl  %%edi,%%eax		\n\t"
     :
     "=a" (res), "=&c" (dummy0), "=&D" (dummy1)
     :
     "1" ((size + 31) >> 5), "2" (dest), "b" (dest));

  return res;
}

L4_INLINE int
l4util_find_first_zero_bit(void * dest, l4_size_t size)
{
  l4_mword_t dummy0, dummy1, dummy2, res;

  if (!size)
    return 0;

  __asm__ __volatile__
    (
     "movl   $-1,%%eax		\n\t"
     "xorl   %%edx,%%edx	\n\t"
     "repe;  scasl		\n\t"
     "je     1f			\n\t"
     "xorl   -4(%%edi),%%eax	\n\t"
     "subl   $4,%%edi		\n\t"
     "bsfl   %%eax,%%edx	\n"
     "1:			\n\t"
     "subl   %%ebx,%%edi	\n\t"
     "shll   $3,%%edi		\n\t"
     "addl   %%edi,%%edx	\n\t"
     :
     "=d" (res), "=&c" (dummy0), "=&D" (dummy1), "=&a" (dummy2)
     :
     "1" ((size + 31) >> 5), "2" (dest), "b" (dest));

  return res;
}

#endif /* !_L4UTIL_BITOPS_H */
