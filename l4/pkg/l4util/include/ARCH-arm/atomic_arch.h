/*
 * ARM specific implementation of atomic functions
 */
#ifndef __L4UTIL__INCLUDE__ARCH_ARM__ATOMIC_ARCH_H__
#define __L4UTIL__INCLUDE__ARCH_ARM__ATOMIC_ARCH_H__

#ifdef __GNUC__

#include <l4/sys/kdebug.h>

EXTERN_C_BEGIN

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32
L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
                 l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  return l4_atomic_cmpxchg((volatile long int *)dest, cmp_val, new_val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG
L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
               l4_umword_t cmp_val, l4_umword_t new_val)
{
  return l4_atomic_cmpxchg((volatile long int *)dest, cmp_val, new_val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32_RES
L4_INLINE l4_uint32_t
l4util_cmpxchg32_res(volatile l4_uint32_t * dest,
                     l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  return l4_atomic_cmpxchg_res((volatile long int *)dest, cmp_val, new_val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG_RES
L4_INLINE l4_umword_t
l4util_cmpxchg_res(volatile l4_umword_t * dest,
                   l4_umword_t cmp_val, l4_umword_t new_val)
{
  return l4_atomic_cmpxchg_res((volatile long int *)dest, cmp_val, new_val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD
L4_INLINE void
l4util_atomic_add(volatile long *dest, long val)
{
  l4_atomic_add(dest, val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_INC
L4_INLINE void
l4util_atomic_inc(volatile long *dest)
{
  l4_atomic_add(dest, 1);
}

L4_INLINE void
l4util_inc32(volatile l4_uint32_t *dest)
{
  l4_atomic_add((volatile long int *)dest, 1);
}

L4_INLINE void
l4util_dec32(volatile l4_uint32_t *dest)
{
  l4_atomic_add((volatile long int *)dest, -1);
}

EXTERN_C_END

#endif //__GNUC__

#endif /* ! __L4UTIL__INCLUDE__ARCH_ARM__ATOMIC_ARCH_H__ */
