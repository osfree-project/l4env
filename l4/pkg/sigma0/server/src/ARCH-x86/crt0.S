#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.section .init

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)

#ifndef FIASCO_UX
	popl	%ecx		/* ptr to kernel info page is on init stack */
#endif
	leal	_stack,%esp	/* reset stack */
	pushl	%ecx		/* push args to init() */
	pushl	%eax
	pushl	%ebx
	pushl	$_exit
	jmp	init

ENTRY(__main)
	ret

        /* MultiBoot header */
        .p2align(2)

ENTRY(_mb_header)
        .long   0x1BADB002              /* magic */
        .long   0x00010000              /* flags: AOUT_KLUDGE */
        .long   0 - 0x1BADB002		/* checksum */
	
	.bss

	.space	2048
ENTRY(_stack)
