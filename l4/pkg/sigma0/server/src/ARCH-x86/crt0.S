#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.section .init

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)

	test	%ecx, %ecx
	jne 1f
	popl	%ecx		/* ptr to kernel info page is on init stack */
1:
	leal	_stack,%esp	/* reset stack */
	pushl	%ecx		/* push args to init() */
	pushl	$_exit
	jmp	init

ENTRY(__main)
	ret
	
	.bss

	.space	2048
ENTRY(_stack)
