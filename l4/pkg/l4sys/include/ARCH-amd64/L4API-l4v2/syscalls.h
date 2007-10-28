/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-amd64/L4API-l4v2/syscalls.h
 * \brief   L4 System calls (except for IPC)
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4V2__SYSCALLS_H__
#define __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4V2__SYSCALLS_H__

#include <l4/sys/syscalls_gen.h>

#if 1
//#ifndef CONFIG_L4_CALL_SYSCALLS

# define L4_SYSCALL_id_nearest            "int $0x31 \n\t"     ///< id_nearest syscall entry
# define L4_SYSCALL_fpage_unmap           "int $0x32 \n\t"     ///< fpage_unmap syscall entry
# define L4_SYSCALL_thread_switch         "int $0x33 \n\t"     ///< thread_switch syscall entry
# define L4_SYSCALL_thread_schedule       "int $0x34 \n\t"     ///< thread_schedule syscall entry
# define L4_SYSCALL_lthread_ex_regs       "int $0x35 \n\t"     ///< lthread_ex_regs syscall entry
# define L4_SYSCALL_task_new              "int $0x36 \n\t"     ///< task_new syscall entry
# define L4_SYSCALL_privctrl              "int $0x37 \n\t"     ///< privctrl syscall entry
# define L4_SYSCALL(name)                 L4_SYSCALL_ ## name  ///< syscall entry

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#  define L4_SYSCALL(s) "call __l4sys_"#s"_direct@plt  \n\t"
# else
#  define L4_SYSCALL(s) "call *__l4sys_"#s"  \n\t"
# endif

#endif

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)  ///< GCC version as one figure (e.g. 402 for gcc-4.2)

#ifdef PROFILE
#include "syscalls-l42-profile.h"
#else
#  if GCC_VERSION < 303
#    error gcc >= 3.3 required
#  else
#    include "syscalls-l42-gcc3.h"
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

#endif /* ! __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4V2__SYSCALLS_H__ */
