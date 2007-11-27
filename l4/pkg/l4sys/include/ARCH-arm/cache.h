/*!
 * \file   l4sys/include/ARCH-arm/cache.h
 * \brief  Cache functions
 *
 * \date   2007-11
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__
#define __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__

#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>

EXTERN_C_BEGIN

L4_INLINE void l4_imb(void);
L4_INLINE void l4_sys_cache_clean_range(unsigned long start, unsigned long end);
L4_INLINE void l4_sys_cache_clean(void);
L4_INLINE void l4_sys_cache_flush_range(unsigned long start, unsigned long end);
L4_INLINE void l4_sys_cache_flush(void);
L4_INLINE void l4_sys_cache_inv_range(unsigned long start, unsigned long end);
L4_INLINE void l4_sys_cache_inv(void);

/** Implemenations ***/

L4_INLINE void
l4_sys_cache_clean_range(unsigned long start, unsigned long end)
{ l4_kdebug_cache(1, start, end); }

L4_INLINE void
l4_sys_cache_clean(void)
{ l4_sys_cache_clean_range(0, ~0UL); }


L4_INLINE void
l4_sys_cache_flush_range(unsigned long start, unsigned long end)
{ l4_kdebug_cache(2, start, end); }

L4_INLINE void
l4_sys_cache_flush(void)
{ l4_sys_cache_flush_range(0, ~0UL); }


L4_INLINE void
l4_sys_cache_inv_range(unsigned long start, unsigned long end)
{ l4_kdebug_cache(3, start, end); }

L4_INLINE void
l4_sys_cache_inv(void)
{ l4_sys_cache_inv_range(0, ~0UL); }



L4_INLINE void
l4_imb(void)
{ l4_sys_cache_clean(); }

EXTERN_C_END

#endif /* ! __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__ */
