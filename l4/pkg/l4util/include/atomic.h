/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/atomic.h
 * \brief   atomic operations header and generic implementations
 * \ingroup atomic
 *
 * \date    10/20/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>,
 *          Jork Loeser  <jork@os.inf.tu-dresden.de> */

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
#ifndef __L4UTIL__INCLUDE__ATOMIC_H__
#define __L4UTIL__INCLUDE__ATOMIC_H__

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN

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
 * \em new_val
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
 * \em new_val
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
 * \em new_val
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
 * \brief Atomic compare and exchange (machine wide fields)
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, 1 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
               l4_umword_t cmp_val, l4_umword_t new_val);

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

/**
 * \brief Atomic exchange (machine wide fields)
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  val           new value for dest
 *
 * \return old value at destination
 */
L4_INLINE l4_umword_t
l4util_xchg(volatile l4_umword_t * dest, l4_umword_t val);

//!@name Atomic compare and exchange with result
/** @{
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 */

L4_INLINE l4_uint8_t
l4util_cmpxchg8_res(volatile l4_uint8_t *dest,
                    l4_uint8_t cmp_val, l4_uint8_t new_val);
L4_INLINE l4_uint16_t
l4util_cmpxchg16_res(volatile l4_uint16_t *dest,
                     l4_uint16_t cmp_val, l4_uint16_t new_val);
L4_INLINE l4_uint32_t
l4util_cmpxchg32_res(volatile l4_uint32_t *dest,
                     l4_uint32_t cmp_val, l4_uint32_t new_val);
L4_INLINE l4_umword_t
l4util_cmpxchg_res(volatile l4_umword_t *dest,
                   l4_umword_t cmp_val, l4_umword_t new_val);
//@}

//!@name Atomic add/sub/and/or (8,16,32 bit version) without result
/** @{
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  val           value to add/sub/and/or
 */
L4_INLINE void
l4util_add8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_add16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_add32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_sub8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_sub16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_sub32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_and8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_and16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_and32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_or8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_or16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_or32(volatile l4_uint32_t *dest, l4_uint32_t val);
//@}

//!@name Atomic add/sub/and/or operations (8,16,32 bit) with result
/** @{
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \param  val           value to add/sub/and/or
 * \return res
 */
L4_INLINE l4_uint8_t
l4util_add8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_add16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_add32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_sub8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_sub16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_sub32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_and8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_and16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_and32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_or8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_or16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_or32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
//@}

//!@name Atomic inc/dec (8,16,32 bit) without result
/** @{
 * \ingroup atomic
 *
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_inc8(volatile l4_uint8_t *dest);
L4_INLINE void
l4util_inc16(volatile l4_uint16_t *dest);
L4_INLINE void
l4util_inc32(volatile l4_uint32_t *dest);
L4_INLINE void
l4util_dec8(volatile l4_uint8_t *dest);
L4_INLINE void
l4util_dec16(volatile l4_uint16_t *dest);
L4_INLINE void
l4util_dec32(volatile l4_uint32_t *dest);
//@}

//!@name Atomic inc/dec (8,16,32 bit) with result
/** @{
 * \ingroup atomic
 *
 * \param  dest          destination operand
 * \return res
 */
L4_INLINE l4_uint8_t
l4util_inc8_res(volatile l4_uint8_t *dest);
L4_INLINE l4_uint16_t
l4util_inc16_res(volatile l4_uint16_t *dest);
L4_INLINE l4_uint32_t
l4util_inc32_res(volatile l4_uint32_t *dest);
L4_INLINE l4_uint8_t
l4util_dec8_res(volatile l4_uint8_t *dest);
L4_INLINE l4_uint16_t
l4util_dec16_res(volatile l4_uint16_t *dest);
L4_INLINE l4_uint32_t
l4util_dec32_res(volatile l4_uint32_t *dest);
//@}

/**
 * \brief Atomic add
 * \ingroup atomic
 *
 * \param  dest      destination operand
 * \param  val       value to add
 */
L4_INLINE void
l4util_atomic_add(volatile long *dest, long val);

EXTERN_C_END

/*****************************************************************************
 *** Get architecture specific implementations
 *****************************************************************************/
#include <l4/util/atomic_arch.h>

/*****************************************************************************
 *** Generic implementation
 ***    (make sure to prevent pagefaults between critical sections)
 ***  Do not use those functions, go implement a version for your
 ***  architecture!
 *****************************************************************************/

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG16
#include <l4/util/irq.h>

L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
                 l4_uint16_t cmp_val, l4_uint16_t new_val)
{
  int ret = 0;

  l4util_cli();

  if (*dest == cmp_val)
    {
      *dest = new_val;
      ret = 1;
    }

  l4util_sti();

  return ret;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32
#include <l4/util/irq.h>

L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
                 l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  int ret = 0;

  l4util_cli();

  if (*dest == cmp_val)
    {
      *dest = new_val;
      ret = 1;
    }

  l4util_sti();

  return ret;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG
#include <l4/util/irq.h>

L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
               l4_umword_t cmp_val, l4_umword_t new_val)
{
  int ret = 0;

  l4util_cli();

  if (*dest == cmp_val)
    {
      *dest = new_val;
      ret = 1;
    }

  l4util_sti();

  return ret;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32_RES
#include <l4/util/irq.h>

L4_INLINE l4_uint32_t
l4util_cmpxchg32_res(volatile l4_uint32_t * dest,
                     l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  l4_uint32_t old_val;

  l4util_cli();

  old_val = *dest;

  if (*dest == cmp_val)
    *dest = new_val;

  l4util_sti();

  return old_val;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG_RES
#include <l4/util/irq.h>

L4_INLINE l4_umword_t
l4util_cmpxchg_res(volatile l4_umword_t * dest,
                   l4_umword_t cmp_val, l4_umword_t new_val)
{
  l4_umword_t old_val;

  l4util_cli();

  old_val = *dest;

  if (*dest == cmp_val)
    *dest = new_val;

  l4util_sti();

  return old_val;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG32
#include <l4/util/irq.h>

L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val)
{
  l4_uint32_t old_val;

  l4util_cli();

  old_val = *dest;
  *dest = val;

  l4util_sti();

  return old_val;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG
#include <l4/util/irq.h>

L4_INLINE l4_umword_t
l4util_xchg(volatile l4_umword_t * dest, l4_umword_t val)
{
  l4_umword_t old_val;

  l4util_cli();

  old_val = *dest;
  *dest = val;

  l4util_sti();

  return old_val;
}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD
#include <l4/util/irq.h>

L4_INLINE void
l4util_atomic_add(volatile long *dest, long val)
{
  l4util_cli();
  *dest += val;
  l4util_sti();
}
#endif

#endif /* ! __L4UTIL__INCLUDE__ATOMIC_H__ */
