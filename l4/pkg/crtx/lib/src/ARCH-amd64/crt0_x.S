/*!
 * \file	crt0_l4env_freebsd.S
 * \brief	Startup code for L4env applications for l4env_freebsd mode
 *
 * \date	06/03/2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#define __ASSEMBLY__
#include <l4/sys/compiler.h>

#ifndef __PIC__

/*--- .text (program code) -------------------------*/
	.text

	/* reference printf to suck in log's version of printf
	 * (instead of OSKit's) */
	.globl printf

	/* needed by base_multiboot_init_mem */
	.globl	_start_of_kernel
_start_of_kernel:

	.globl	_start
_start:
	/* Save the address of our trampoline page. */
	mov	%rsp, crt0_tramppage
	//and	$0xFFFFF000, crt0_tramppage

	/* Save the multiboot flag. */
	mov	%rax, crt0_multiboot_flag

	/* Save the pointer to our multiboot info structure. Normally
	 * it will point into the trampoline page. */
	mov	%rbx, crt0_multiboot_info

	/* Save the pointer to the L4env infopage. Only valid if
	 * crt0_multiboot_flag is ~(L4UTIL_MB_VALID) */
	mov	%rsi, crt0_l4env_infopage

	/* Switch to our own stack.  */
	mov	$crt0_stack_high,%rsp

	/* Clear the base pointer so that stack backtraces will work.  */
	xor	%rbp,%rbp

	/* Jump into C code (init backends, call constructors, call main). */
	call	__main

	/* We should never get here since l4env_startup() does not return */
1:	int	$3
	jmp	2f
	.ascii	"__main() returned."
2:	jmp	1b


	/* The multiBoot header (must be located in the first 8192 bytes. */
	.p2align(2), 0x90
	.long	0x1BADB002		/* magic */
	.long	0x00000000		/* feature flags */
	.long	0 - 0x1BADB002		/* checksum */


/*--- .ctors (array of pointers to constructors) ---*/
	.section .ctors,"a",@progbits
	.p2align(2)
ctors:


/*--- .dtors (array of pointers to destructors) ----*/
	.section .dtors,"a",@progbits
	.p2align(2)
dtors:

#endif /* __PIC__ */

/*--- .data (initialized data) ---------------------*/
	.data
	.p2align(12)
	.globl	start_of_data
start_of_data:
	.long	0

	.globl	__dso_handle
__dso_handle:
	.long	0

#ifndef __PIC__

/*--- .eh_frame (exception frames) -----------------*/
	.section .eh_frame,"aw"
	.globl	__EH_FRAME_BEGIN__
__EH_FRAME_BEGIN__:


/*--- .bss (non-initialized data) ------------------*/
	.bss
	.p2align 8
	.globl	crt0_stack_low
crt0_stack_low:
	.space	8*1024
	.globl	crt0_stack_high
crt0_stack_high:

	.globl	crt0_tramppage
crt0_tramppage:
	.space	8

	.globl	crt0_multiboot_flag
crt0_multiboot_flag:
	.space	8

	.globl	crt0_multiboot_info
crt0_multiboot_info:
	.space	8

	.globl	crt0_l4env_infopage
crt0_l4env_infopage:
	.space	8

#endif /* __PIC__ */
