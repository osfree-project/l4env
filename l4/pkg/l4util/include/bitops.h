/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/bitops.h
 * \brief   bit manipulation functions
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
#ifndef __L4UTIL__INCLUDE__BITOPS_H__
#define __L4UTIL__INCLUDE__BITOPS_H__

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

EXTERN_C_BEGIN

/** \defgroup bitops Bit Manipulation Functions */

/**
 * \brief Set bit in memory
 * \ingroup bitops
 *
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_set_bit(int b, volatile l4_umword_t * dest);

L4_INLINE void
l4util_set_bit32(int b, volatile l4_uint32_t * dest);

#if L4_MWORD_BITS >= 64
L4_INLINE void
l4util_set_bit64(int b, volatile l4_uint64_t * dest);
#endif

/**
 * \brief Clear bit in memory
 * \ingroup bitops
 *
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_clear_bit(int b, volatile l4_umword_t * dest);

L4_INLINE void
l4util_clear_bit32(int b, volatile l4_uint32_t * dest);

#if L4_MWORD_BITS >= 64
L4_INLINE void
l4util_clear_bit64(int b, volatile l4_uint64_t * dest);
#endif

/**
 * \brief Complement bit in memory
 * \ingroup bitops
 *
 * \param  b             bit position
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_complement_bit(int b, volatile l4_umword_t * dest);

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
l4util_test_bit(int b, volatile l4_umword_t * dest);

L4_INLINE int
l4util_test_bit32(int b, volatile l4_uint32_t * dest);

#if L4_MWORD_BITS >= 64
L4_INLINE int
l4util_test_bit64(int b, volatile l4_uint64_t * dest);
#endif

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
l4util_bts(int b, volatile l4_umword_t * dest);

/**
 * \brief Bit test and reset
 * \ingroup bitops
 *
 * \param  b             bit position
 * \param  dest          destination operand
 *
 * \return Old value of bit \em b.
 *
 * Reset bit \em b and return old value.
 */
L4_INLINE int
l4util_btr(int b, volatile l4_umword_t * dest);

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
l4util_btc(int b, volatile l4_umword_t * dest);

/**
 * \brief Bit scan reverse
 * \ingroup bitops
 *
 * \param  word          value (machine size)
 *
 * \return index of most significant set bit in word,
 *         -1 if no bit is set (word == 0)
 *
 * "bit scan reverse", find most significant set bit in word (-> LOG2(word))
 */
L4_INLINE int
l4util_bsr(l4_umword_t word);

/**
 * \brief Bit scan forward
 * \ingroup bitops
 *
 * \param  word          value (machine size)
 *
 * \return index of least significant bit set in word,
 *         -1 if no bit is set (word == 0)
 *
 * "bit scan forward", find least significant bit set in word.
 */
L4_INLINE int
l4util_bsf(l4_umword_t word);

/**
 * \brief Find the first set bit in a memory region
 * \ingroup bitops
 *
 * \param  dest          bitstring
 * \param  size          size of string in bits (must be a multiple of 32!)
 *
 * \return number of the first set bit,
 *         >= size if no bit is set
 */
L4_INLINE int
l4util_find_first_set_bit(void * dest, l4_size_t size);

/**
 * \brief Find the first zero bit in a memory region
 * \ingroup bitops
 *
 * \param  dest          bitstring
 * \param  size          size of string in bits (must be a multiple of 32!)
 *
 * \return number of the first zero bit,
 *         >= size if no bit is set
 */
L4_INLINE int
l4util_find_first_zero_bit(void * dest, l4_size_t size);


EXTERN_C_END

/*****************************************************************************
 *** Implementation of specific version
 *****************************************************************************/

#include <l4/util/bitops_arch.h>

/*****************************************************************************
 *** Generic implementations
 *****************************************************************************/

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_SET_BIT
#include <l4/util/atomic.h>
L4_INLINE void
l4util_set_bit(int b, volatile l4_umword_t * dest)
{
  l4_umword_t oldval, newval;

  dest += b / (sizeof(*dest) * 8); /* advance dest to the proper element */
  b    &= sizeof(*dest) * 8 - 1;   /* modulo; cut off all upper bits */

  do
    {
      oldval = *dest;
      newval = oldval | (1 << b);
    }
  while (!l4util_cmpxchg(dest, oldval, newval));
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_CLEAR_BIT
#include <l4/util/atomic.h>
L4_INLINE void
l4util_clear_bit(int b, volatile l4_umword_t * dest)
{
  l4_umword_t oldval, newval;

  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  do
    {
      oldval = *dest;
      newval = oldval & ~(1 << b);
    }
  while (!l4util_cmpxchg(dest, oldval, newval));
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_TEST_BIT
L4_INLINE int
l4util_test_bit(int b, volatile l4_umword_t * dest)
{
  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  return (*dest >> b) & 1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_TEST_BIT32
L4_INLINE int
l4util_test_bit32(int b, volatile l4_uint32_t * dest)
{
  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  return (*dest >> b) & 1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_BIT_TEST_AND_SET
#include <l4/util/atomic.h>
L4_INLINE int
l4util_bts(int b, volatile l4_umword_t * dest)
{
  l4_umword_t oldval, newval;

  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  do
    {
      oldval = *dest;
      newval = oldval | (1 << b);
    }
  while (!l4util_cmpxchg(dest, oldval, newval));

  /* Return old bit */
  return (oldval >> b) & 1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_BIT_TEST_AND_RESET
#include <l4/util/atomic.h>
L4_INLINE int
l4util_btr(int b, volatile l4_umword_t * dest)
{
  l4_umword_t oldval, newval;

  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  do
    {
      oldval = *dest;
      newval = oldval & ~(1 << b);
    }
  while (!l4util_cmpxchg(dest, oldval, newval));

  /* Return old bit */
  return (oldval >> b) & 1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_BIT_SCAN_REVERSE
L4_INLINE int
l4util_bsr(l4_umword_t word)
{
  int i;

  if (!word)
    return -1;

  for (i = 8 * sizeof(word) - 1; i >= 0; i--)
    if ((1 << i) & word)
      return i;

  return -1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_BIT_SCAN_FORWARD
L4_INLINE int
l4util_bsf(l4_umword_t word)
{
  unsigned int i;

  if (!word)
    return -1;

  for (i = 0; i < sizeof(word) * 8; i++)
    if ((1 << i) & word)
      return i;

  return -1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_FIND_FIRST_ZERO_BIT
L4_INLINE int
l4util_find_first_zero_bit(void * dest, l4_size_t size)
{
  l4_size_t i, j;
  unsigned long *v = (unsigned long*)dest;

  if (!size)
    return 0;

  size = (size + 31) & ~0x1f; /* Grmbl: adapt to x86 implementation... */

  for (i = j = 0; i < size; i++, j++)
    {
      if (j >= sizeof(*v) * 8)
	{
	  j = 0;
	  v++;
	}
      if (!((1 << j) & *v))
	return i;
    }
  return size + 1;
}
#endif

#ifndef __L4UTIL_BITOPS_HAVE_ARCH_COMPLEMENT_BIT
L4_INLINE void
l4util_complement_bit(int b, volatile l4_umword_t * dest)
{
  dest += b / (sizeof(*dest) * 8);
  b    &= sizeof(*dest) * 8 - 1;

  *dest ^= 1UL << b;
}
#endif

#endif /* ! __L4UTIL__INCLUDE__BITOPS_H__ */
