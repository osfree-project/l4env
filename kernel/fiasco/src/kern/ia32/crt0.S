/* -*- c -*- */

#include <flux/x86/asm.h>

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)
ENTRY(crt0_start)
	addl	$4,%esp		/* throw away return address */
	popl    %edx		/* pop args from bootstrap.c's stack */
	popl	%ecx
	popl	%ebx
	popl	%eax

        /* Clear the base pointer so that stack backtraces will work.  */
        xorl    %ebp,%ebp

	leal	EXT(_stack_end),%esp
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	call	__main

        /* never returns */

	.bss
	.space	2048
ENTRY(_stack_end)
