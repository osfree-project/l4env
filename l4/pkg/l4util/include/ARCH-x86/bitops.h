/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/ARCH-x86/bitops.h
 * \brief   x86 bit manipulation functions
 * \ingroup bitops
 *
 * \date    07/03/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
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
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN;

/*****************************************************************************/
/**
 * \brief Set bit
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 */
/*****************************************************************************/ 
L4_INLINE void
set_bit(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Clear bit
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 */
/*****************************************************************************/ 
L4_INLINE void
clear_bit(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Complement bit
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 */
/*****************************************************************************/ 
L4_INLINE void
complement_bit(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Test bit (return value of bit)
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 *	
 * \return Value of bit \em b.
 */
/*****************************************************************************/ 
L4_INLINE int
test_bit(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Bit test and set
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 *	
 * \return Old value of bit \em b.
 *
 * Set the \em b bit of \em dest to 1 and return the old value.
 */
/*****************************************************************************/ 
L4_INLINE int 
bts(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Bit test and reset
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 *	
 * \return Old value of bit \em b.
 *
 * Rest bit \em b and return old value.
 */
/*****************************************************************************/ 
L4_INLINE int
btr(l4_uint32_t * dest, int b);

/*****************************************************************************/
/**
 * \brief Bit test and complement
 * \ingroup bitops
 * 
 * \param  dest          destination operand
 * \param  b             bit position
 *	
 * \return Old value of bit \em b.
 *
 * Complement bit \em b and return old value.
 */
/*****************************************************************************/ 
L4_INLINE int
btc(l4_uint32_t * dest, int b);


/*****************************************************************************/
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
/*****************************************************************************/ 
L4_INLINE int
bsr(l4_uint32_t word);

/*****************************************************************************/
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
/*****************************************************************************/ 
L4_INLINE int
bsf(l4_uint32_t word);

EXTERN_C_END;

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

/* set bit */
L4_INLINE void
set_bit(l4_uint32_t * dest, int b)
{
  __asm__ __volatile__
    (
     "btsl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "r"   (b)        /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* clear bit */
L4_INLINE void
clear_bit(l4_uint32_t * dest, int b)
{
  __asm__ __volatile__
    (
     "btrl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "r"   (b)        /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* change bit */
L4_INLINE void
complement_bit(l4_uint32_t * dest, int b)
{
  __asm__ __volatile__
    (
     "btcl  %1,%0   \n\t"
     :
     :
     "m"   (*dest),   /* 0 mem, destination operand */ 
     "r"   (b)        /* 1,     bit number */
     :
     "memory", "cc"
     );
}

/* test bit */
L4_INLINE int
test_bit(l4_uint32_t * dest, int b)
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
     "r"   (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and set */
L4_INLINE int 
bts(l4_uint32_t * dest, int b)
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
     "r"   (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and reset */
L4_INLINE int
btr(l4_uint32_t * dest, int b)
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
     "r"   (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit test and complement */
L4_INLINE int
btc(l4_uint32_t * dest, int b)
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
     "r"   (b)        /* 2,     bit number */
     :
     "memory", "cc"
     );

  return (int)bit;
}

/* bit scan reverse */
L4_INLINE int
bsr(l4_uint32_t word)
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
bsf(l4_uint32_t word)
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

#endif /* !_L4UTIL_BITOPS_H */
