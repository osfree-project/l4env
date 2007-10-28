/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/syscalls.h
 * \brief   L4 System calls (except for IPC)
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4SYS__INCLUDE__ARCH_X86__L4API_L4V2__SYSCALLS_H__
#define __L4SYS__INCLUDE__ARCH_X86__L4API_L4V2__SYSCALLS_H__

#include <l4/sys/syscalls_gen.h>

#include <l4/sys/syscall-invoke.h>

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)  ///< GCC version as one figure (e.g. 402 for gcc-4.2)

#ifdef PROFILE
#include "syscalls-l42-profile.h"
#else
#  if GCC_VERSION < 302
#    error gcc >= 3.0.2 required
#  else
#    ifdef __PIC__
#      include "syscalls-l42-gcc3-pic.h"
#    else
#      include "syscalls-l42-gcc3-nopic.h"
#    endif
#  endif

/* ============================================================= */

#endif /* ! PROFILE */

L4_INLINE void *
l4_kernel_interface(void)
{
  void *ret;
  asm (" lock; nop " : "=a"(ret) );
  return ret;
}

#include <l4/sys/syscalls-impl.h>

#endif /* ! __L4SYS__INCLUDE__ARCH_X86__L4API_L4V2__SYSCALLS_H__ */
