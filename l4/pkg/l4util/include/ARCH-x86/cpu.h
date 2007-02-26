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

static inline void
l4util_cpu_pause(void)
{
  __asm__ __volatile__ ("rep; nop");
}

L4_INLINE int
l4util_cpu_has_cpuid(void)
{
  unsigned long eax;

  asm volatile("pushfl			\t\n"
               "popl %%eax		\t\n" /* get eflags */
               "movl %%eax, %%ebx	\t\n" /* save it */
               "xorl $0x200000, %%eax	\t\n" /* toggle ID bit */
               "pushl %%eax		\t\n" 
               "popfl			\t\n" /* set again */
               "pushfl			\t\n"
               "popl %%eax		\t\n" /* get it again */
               "xorl %%eax, %%ebx	\t\n"
               "pushl %%ebx		\t\n"
               "popfl			\t\n" /* restore saved flags */
               : "=a" (eax)
               : /* no input */
               : "ebx");

  return eax & 0x200000;
}

L4_INLINE unsigned int
l4util_cpu_capabilities_nocheck(void)
{
  unsigned int dummy;
  unsigned int capability;

  /* get CPU capabilities */
  asm volatile("pushl %%ebx ; cpuid ; popl %%ebx"
               : "=a" (dummy),
                 "=c" (dummy),
                 "=d" (capability)
               : "a" (0x00000001)
               : "cc");

  return capability;
}

L4_INLINE unsigned int
l4util_cpu_capabilities(void)
{
  if (!l4util_cpu_has_cpuid())
    return 0; /* CPU has not cpuid instruction */

  return l4util_cpu_capabilities_nocheck();
}

#endif

