/* $Id$ */
/**
 * \file	loader/server/src/trampoline.c
 * \brief	Trampoline code for old-style applications
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/linkage.h>
#include <l4/util/mb_info.h>

#include "trampoline.h"

/** This function is mapped into a new tasks address space to load the
 *  registers in a multiboot-compliant way before starting the task's
 *  real code. Not needed if bootet with libloader.s.so. */
#ifdef ARCH_x86
void
task_trampoline(l4_addr_t entry, void *mbi, void *env, unsigned mb_flag)
{
  unsigned dummy1, dummy2, dummy3, dummy4;

  asm volatile("movl %%edx,%%ebx	\n\t"
               "call *%%ecx		\n\t"
               ".globl _task_trampoline_end\n"
	       "_task_trampoline_end:\n\t"
	       : "=c"(dummy1), "=d"(dummy2), "=a"(dummy3), "=S"(dummy4)
	       :  "c"(entry), "d"(mbi), "a"(mb_flag), "S"(env)
	       );
  /* NORETURN */
}
#endif

/* See also roottask/server/src/trampoline.c */
#ifdef ARCH_arm

asm (
".global task_trampoline            \n"
"task_trampoline:                   \n"
"	ldr r3, [sp, #4]!           \n" // inc sp, load entry address to r3 sp
"	ldr r1, [sp, #4]!           \n" // inc sp, load mbi pointer to r1
"	ldr r2, [sp, #4]!           \n" // inc sp, load env page pointer to r2
"	ldr r0, [sp, #4]!           \n" // load MB-Magic to r0
"	mov pc, r3                  \n" // jump to entry
".global _task_trampoline_end       \n"
"_task_trampoline_end:              \n");

#endif
