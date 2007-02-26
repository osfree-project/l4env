#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.section .init

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)

	test	%rdi, %rdi
	jne	1f
	pop	%rdi		/* ptr to kernel info page is on init stack */
1:
	lea	_stack,%rsp	/* reset stack */
	push	$_exit
	jmp	init

ENTRY(__main)
	ret
	
	.bss

	.space	4096
ENTRY(_stack)
