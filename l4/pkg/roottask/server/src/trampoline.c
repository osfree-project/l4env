/**
 * \file	roottask/server/src/trampoline.c
 * \brief	holds entry function for new task
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * provides _task_trampoline_end
 **/
#include <l4/sys/compiler.h>
#include <l4/util/mb_info.h>

#include "trampoline.h"

/**
 * This function is mapped into a new tasks address space to load the
 * registers in a multiboot-compliant way before starting the task's
 * real code.
 **/

#if defined(ARCH_x86)

void
task_trampoline(l4_addr_t entry, void *mbi)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile("movl %%edx,%%ebx        \n\t"
               "call *%%ecx             \n\t"
               ".globl _task_trampoline_end\n"
	       "_task_trampoline_end:"
	       : "=c" (dummy1), "=d" (dummy2), "=a" (dummy3)
	       : "0" (entry), "1" (mbi), "2" (L4UTIL_MB_VALID)
	       );

  /* NORETURN */
}

#elif defined(ARCH_arm)

/*
 * Stacklayout:
 *       mbi
 *       entry
 * sp -> 0
 */

asm (".globl task_trampoline         \n"
     "task_trampoline:               \n"
     "  ldr r3, [sp, #4]!            \n" // inc sp, load entry address to r3 sp
     "  ldr r1, [sp, #4]!            \n" // inc sp, load mbi pointer to r1
     "  ldr r0, .LC_l4util_mb_valid  \n" // load MB-Magic to r0
     "  mov pc, r3                   \n" // jump to entry
     ".LC_l4util_mb_valid:           \n"
     "  .word " L4_stringify(L4UTIL_MB_VALID_ASM) "\n"
     "                               \n"
     ".globl _task_trampoline_end    \n"
     "_task_trampoline_end:          \n");

#elif defined(ARCH_amd64)
void
task_trampoline(l4_addr_t entry, void* mbi)
{
  unsigned long dummy1, dummy2;

  asm volatile("pop %%rcx		\n\t" // remove dummy
	       "pop %%rcx		\n\t" // get entry point
	       "pop %%rbx		\n\t" // get mbi
               "call *%%rcx		\n\t" // jump to entry
               ".globl _task_trampoline_end\n"
	       "_task_trampoline_end:"
	       : "=rax" (dummy1), "=rcx" (dummy2)
	       : "0" (L4UTIL_MB_VALID)
	       );

  /* NORETURN */
}

#elif

#error Unknown architecture!

#endif
