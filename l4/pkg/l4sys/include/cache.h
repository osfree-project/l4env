/*!
 * \file   l4sys/include/cache.h
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

#ifndef __L4SYS__INCLUDE__CACHE_H__
#define __L4SYS__INCLUDE__CACHE_H__

#include <l4/sys/compiler.h>

L4_INLINE void l4_imb(void);
L4_INLINE void l4_sys_cache_clean_range(unsigned long start, unsigned long end);




/**** Implemenations ****/

L4_INLINE void
l4_imb(void)
{}

L4_INLINE void
l4_sys_cache_clean_range(unsigned long start, unsigned long end)
{}

#endif /* ! __L4SYS__INCLUDE__CACHE_H__ */
