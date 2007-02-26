/* $Id$ */

#ifndef __L4UTIL_LOCK_H__
#define __L4UTIL_LOCK_H__

#include <l4/sys/syscalls.h>
#include <l4/sys/compiler.h>

typedef int l4_simple_lock_t;

L4_INLINE int l4_simple_try_lock(l4_simple_lock_t *lock);
L4_INLINE void l4_simple_unlock(l4_simple_lock_t *lock);
L4_INLINE int l4_simple_lock_locked(l4_simple_lock_t *lock);
L4_INLINE void l4_simple_lock_solid(register l4_simple_lock_t *p);
L4_INLINE void l4_simple_lock(l4_simple_lock_t * lock);

L4_INLINE int 
l4_simple_try_lock(l4_simple_lock_t *lock)
{
  int tmp;
  __asm__ __volatile__ ("xchg	%1, (%2)\n\t"
			: "=r" (tmp)
			: "0" (1), "r" (lock)
			: "memory"
			);
  return (tmp == 0);
}

 
L4_INLINE void 
l4_simple_unlock(l4_simple_lock_t *lock)
{
  *lock = 0;
#if 0
  __asm__ __volatile__ ("xchg	%1, (%2)\n\t"
			:
			: "r" (0), "r" (lock)
			: "memory"
			);
#endif
}

L4_INLINE int
l4_simple_lock_locked(l4_simple_lock_t *lock)
{
  return (*lock==0) ? 0 : 1; 
}

L4_INLINE void
l4_simple_lock_solid(register l4_simple_lock_t *p)
{
  while (l4_simple_lock_locked(p) || !l4_simple_try_lock(p))
    l4_thread_switch(L4_NIL_ID);
}

L4_INLINE void
l4_simple_lock(l4_simple_lock_t * lock)
{
  if(!l4_simple_try_lock(lock))
    l4_simple_lock_solid(lock);
}

#endif /* __L4UTIL_LOCK_H__ */









