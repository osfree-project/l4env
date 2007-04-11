/**
 * \file   l4util/include/ARCH-x86/cpu.h
 * \brief  CPU related functions
 *
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __L4_UTIL_CPU_H
#define __L4_UTIL_CPU_H

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/** \defgroup cpu CPU related functions */

/**
 * Check whether the CPU supports the "cpuid" instruction.
 * \ingroup cpu
 *
 * \return 1 if it has, 0 if it has not
 */
L4_INLINE int          l4util_cpu_has_cpuid(void);

/**
 * Returns the CPU capabilities if the "cpuid" instruction is available.
 * \ingroup cpu
 *
 * \return CPU capabilities if the "cpuid" instruction is available,
 *         0 if the "cpuid" instruction is not supported.
 */
L4_INLINE unsigned int l4util_cpu_capabilities(void);

/**
 * Returns the CPU capabilities.
 * \ingroup cpu
 *
 * \return CPU capabilities.
 */
L4_INLINE unsigned int l4util_cpu_capabilities_nocheck(void);

/**
 * Generic CPUID access function.
 * \ingroup cpu
 */
L4_INLINE void
l4util_cpu_cpuid(unsigned long mode,
                 unsigned long *eax, unsigned long *ebx,
                 unsigned long *ecx, unsigned long *edx);

static inline void
l4util_cpu_pause(void)
{
  __asm__ __volatile__ ("rep; nop");
}

L4_INLINE int
l4util_cpu_has_cpuid(void)
{
  unsigned long eax;

  asm volatile("pushf			\t\n"
               "pop %%rax		\t\n" /* get eflags */
               "mov %%rax, %%rbx	\t\n" /* save it */
               "xorq $0x200000, %%rax	\t\n" /* toggle ID bit */
               "push %%rax		\t\n" 
               "popf			\t\n" /* set again */
               "pushf			\t\n"
               "pop %%rax		\t\n" /* get it again */
               "xor %%rax, %%rbx	\t\n"
               "push %%rbx		\t\n"
               "popf			\t\n" /* restore saved flags */
               : "=a" (eax)
               : /* no input */
               : "rbx");

  return eax & 0x200000;
}

L4_INLINE void
l4util_cpu_cpuid(unsigned long mode,
                 unsigned long *eax, unsigned long *ebx,
                 unsigned long *ecx, unsigned long *edx)
{
  asm volatile("cpuid"
               : "=a" (*eax),
                 "=b" (*ebx),
                 "=c" (*ecx),
                 "=d" (*edx)
               : "a"  (mode)
               : "cc");
}

L4_INLINE unsigned int
l4util_cpu_capabilities_nocheck(void)
{
  unsigned long dummy, capability;

  /* get CPU capabilities */
  l4util_cpu_cpuid(1, &dummy, &dummy, &dummy, &capability);

  return capability;
}

L4_INLINE unsigned int
l4util_cpu_capabilities(void)
{
  if (!l4util_cpu_has_cpuid())
    return 0; /* CPU has not cpuid instruction */

  return l4util_cpu_capabilities_nocheck();
}

EXTERN_C_END

#endif

