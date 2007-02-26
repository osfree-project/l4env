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

#include "trampoline.h"

#define MULTIBOOT_VALID 0x2BADB002

/** this function is mapped into a new tasks address space to load the
 * registers in a multiboot-compliant way before starting the task's
 * real code */
void
task_trampoline(l4_addr_t entry, void *mbi)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile("movl %%edx,%%ebx	\n\t"
               "call *%%ecx		\n\t"
               ".globl _task_trampoline_end\n"
	       "_task_trampoline_end:\n\t"
	       : "=c" (dummy1), "=d" (dummy2), "=a" (dummy3)
	       : "0"  (entry),  "1"  (mbi),    "2"   (MULTIBOOT_VALID)
	       );
  /* NORETURN */
}

