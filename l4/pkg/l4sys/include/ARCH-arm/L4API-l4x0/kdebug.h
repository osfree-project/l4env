#include_next <l4/sys/kdebug.h>

#ifndef __L4SYS__INCLUDE__ARCH_ARM__L4API_X0__KDEBUG_H__
#define __L4SYS__INCLUDE__ARCH_ARM__L4API_X0__KDEBUG_H__

#include <l4/sys/types.h>

L4_INLINE void
fiasco_register_thread_name(l4_threadid_t tid, const char *name);

/*
 * -------------------------------------------------------------------
 * Implementations
 */

L4_INLINE void
fiasco_register_thread_name(l4_threadid_t tid, const char *name)
{
  __KDEBUG_ARM_PARAM_2(0x10, tid.raw, name);
}

#endif /* ! __L4SYS__INCLUDE__ARCH_ARM__L4API_X0__KDEBUG_H__ */
