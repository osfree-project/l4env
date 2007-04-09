#ifndef __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__
#define __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__

#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>

L4_INLINE void l4_imb(void);

L4_INLINE void
l4_imb(void)
{
  l4_kdebug_imb();
}

#endif /* ! __L4SYS__INCLUDE__ARCH_ARM__CACHE_H__ */
