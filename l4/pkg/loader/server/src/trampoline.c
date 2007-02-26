/* $Id$ */
/**
 * \file	loader/server/src/trampoline.c
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Trampoline code for old-style applications
 */
#include <l4/sys/types.h>
#include <l4/sys/linkage.h>

#include "trampoline.h"

#define MULTIBOOT_VALID 0x2BADB002

/** this function is mapped into a new tasks address space to load the
 * registers in a multiboot-compliant way before starting the task's
 * real code */
void
task_trampoline(l4_addr_t entry, struct grub_multiboot_info *mbi)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile("movl %%edx,%%ebx	\n\t\
                call *%%ecx		\n\t\
                .globl " SYMBOL_NAME_STR(_task_trampoline_end) "\n"
	        SYMBOL_NAME_STR(_task_trampoline_end) ":"
	       : "=c" (dummy1), "=d" (dummy2), "=a" (dummy3)
	       : "0"  (entry),  "1"  (mbi),    "2"   (MULTIBOOT_VALID)
	       );
  /* NORETURN */
}

