/* -*- c -*- */
/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Startup code to create MultiBoot-compliant kernel images.
 * You must link this at the beginning of the kernel image.
 * Furthermore, you must link crtn.o at the end of the kernel image,
 * after all of your .o's _and_ after all libraries you link with.
 * (Actually, crtn.o is only strictly required under ELF,
 * but its use is recommended in any case for consistency.)
 *
 * The MultiBoot header defined here assumes none of the specific
 * feature flag bits need to be set
 * (other than the a.out-kludge bit, in the case of a.out format).
 * If you need to use additional features,
 * you will need to define your own MultiBoot header -
 * e.g., just copy this file and modify it as appropriate.
 */
#define __ASSEMBLY__
#include <l4/sys/compiler.h>

/* hohmuth: reference stuff to suck in our (liboskit10_support)
   version (instead of OSKit's) */
        .globl pc_reset

	.text

	/*
	 * See multiboot_dyn.S, where this actually means something different
	 * than _start. 
	 */
	.globl	_start_of_kernel
_start_of_kernel:

	/* 
	 * We are only entered this way when the boot loader doesn't
	 * have the a.out kludge info and assumes the lowest text addr
	 * is the entry point.
	 */
ENTRY(_start)
	jmp	boot_entry

	/* MultiBoot header - see multiboot.h.  */
	.p2align(2)
boot_hdr:
	.long	0x1BADB002		/* magic */
#ifdef __ELF__
	.long	0x00000000		/* feature flags */
	.long	0-0x1BADB002-0x00000000	/* checksum */
#else /* a.out kludge */
	.long	0x00010000		/* feature flags */
	.long	0-0x1BADB002-0x00010000	/* checksum */
	.long	boot_hdr		/* header_addr */
	.long	SYMBOL_NAME(_start)	/* load_addr */
	.long	SYMBOL_NAME(edata)	/* load_end_addr */
	.long	SYMBOL_NAME(end)	/* bss_end_addr */
	.long	boot_entry		/* entry */
#endif

boot_entry:

	/* L4 stuff: the region mapper has to know the position of the
	 * RMGR trampoline page */
	movl	%esp, _tramppage
	andl	$0xFFFFF000, _tramppage

	/* Switch to our own stack.  */
	movl	$SYMBOL_NAME(base_stack_end),%esp

	/*
	 * Reset EFLAGS to a known state.
	 * This will also clear the DF flag for the string-clear below.
	 */
	pushl	$0
	popf

	/* Clear uninitialized data.  */
	/* XXX this shouldn't be necessary, as the multiboot spec now says
	   that the bootloader does this rather than the OS kernel.
	   Remove when we're sure all boot loaders have been updated. */
	lea	SYMBOL_NAME(edata),%edi
	lea	SYMBOL_NAME(end),%ecx
	subl	%edi,%ecx
	xorl	%eax,%eax
	rep
	stosb

	/* Push the boot_info parameter for multiboot_main()
	   before ebx gets trashed by the initialization code in crti.h.  */
	pushl	%ebx

	/* mehnert: moved crti.h from oskit10 here so we don't need the
	 * oskit for building this package */

	/* Clear the base pointer so that stack backtraces will work.  */
	xorl	%ebp,%ebp

	jmp	3f
ENTRY(__oskit_init)

	/* Call all global constructors.  */
	pushl	%ebx
	pushl	%eax
	lea	ctors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx
	ret
3:

	/*
	 * Similar to the '.init' section,
	 * the '.fini' section contains code to run at exit time.
	 */
	.section .fini
	.type SYMBOL_NAME(__oskit_fini),@function
ENTRY(__oskit_fini)

	/* Call all global destructors.  */
	pushl	%ebx
	pushl	%eax
	lea	dtors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx

	/*
	 * When any global variable needing a constructor is declared in C++,
	 * it writes a longword into the '.ctors' section
	 * which is a pointer to the appropriate constructor routine to call.
	 */
	.section .ctors,"a",@progbits
	.p2align(2)
	.long	0
ctors:

	/*
	 * Similarly, the '.dtors' section is a list of pointers
	 * to destructor routines for global C++ objects.
	 */
	.section .dtors,"a",@progbits
	.p2align(2)
	.long	0
dtors:

	/*
	 * Finally, the '.anno' section is our own little invention;
	 * it allows generic program annotations of many different kinds
	 * to be collected together by the linker into "annotation tables";
	 * see oskit/anno.h for more information.
	 */
	.section .anno,"aw",@progbits
	.p2align(4)
ENTRY(__ANNO_START__)

	/*
	 * Now back to our normal programming...
	 */
	.text

	/* Jump into C code.  */
	call	l4env_startup

1:	int	$3
	jmp	2f
	.ascii	"Done"

2:	jmp	1b

	/*
	 * Create and align an initial data segment.
	 */

	.data
	.p2align(12)	/* mehnert: hardcoded to L4 page bits */
	.globl	SYMBOL_NAME(start_of_data)
ENTRY(start_of_data)
	.long	0

	/*
	 * Drag this symbol in so that the version is always included
	 * in the linked image.
	 */
.L69:
	.long SYMBOL_NAME(oskit_version_string)

	/*
	 * This symbol matches the one in unix/crt0.c. Since usermode
	 * simulation is "important," this symbol is allowed to be
	 * here so it can be referenced in oskit libraries. Jeez. 
	 */
	.globl	SYMBOL_NAME(oskit_usermode_simulation)
SYMBOL_NAME_LABEL(oskit_usermode_simulation)
	.long	0

	.globl	SYMBOL_NAME(_tramppage)
SYMBOL_NAME_LABEL(_tramppage)
	.long	0

	.section .eh_frame,"aw"
	.globl	SYMBOL_NAME(__EH_FRAME_BEGIN__)
SYMBOL_NAME_LABEL(__EH_FRAME_BEGIN__)

