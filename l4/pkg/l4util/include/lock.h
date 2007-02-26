/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/lock.h
 * \brief   Simple lock implementation. 
 *          Does only work if all thread have the same priority!
 *
 * \date    02/1997
 * \author  Michael Hohmuth <hohmuth@os.inf.tu-dresden.de> */

/*
 * Copyright (C) 2000-2004
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
#ifndef __L4UTIL_LOCK_H__
#define __L4UTIL_LOCK_H__

#include <l4/sys/syscalls.h>
#include <l4/sys/compiler.h>
#include <l4/util/atomic.h>

typedef l4_uint32_t l4util_simple_lock_t;

L4_INLINE int  l4_simple_try_lock(l4util_simple_lock_t *lock);
L4_INLINE void l4_simple_unlock(l4util_simple_lock_t *lock);
L4_INLINE int  l4_simple_lock_locked(l4util_simple_lock_t *lock);
L4_INLINE void l4_simple_lock_solid(register l4util_simple_lock_t *p);
L4_INLINE void l4_simple_lock(l4util_simple_lock_t * lock);

L4_INLINE int 
l4_simple_try_lock(l4util_simple_lock_t *lock)
{
  return l4util_xchg32(lock, 1) == 0;
}
 
L4_INLINE void 
l4_simple_unlock(l4util_simple_lock_t *lock)
{
  *lock = 0;
}

L4_INLINE int
l4_simple_lock_locked(l4util_simple_lock_t *lock)
{
  return (*lock == 0) ? 0 : 1;
}

L4_INLINE void
l4_simple_lock_solid(register l4util_simple_lock_t *p)
{
  while (l4_simple_lock_locked(p) || !l4_simple_try_lock(p))
    l4_thread_switch(L4_NIL_ID);
}

L4_INLINE void
l4_simple_lock(l4util_simple_lock_t * lock)
{
  if (!l4_simple_try_lock(lock))
    l4_simple_lock_solid(lock);
}

#endif
