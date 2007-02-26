#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.section .init

	.globl	start
	.globl	_start
start:
_start:
#ifdef ARCH_x86
	leal	_stack,%esp
	movl	%ebx,mb_ptr	/* for FIASCO_UX */
	jmp	init
#else
	ldr	sp, .LCstack_top
	b	init
.LCstack_top: .word _stack
#endif


	.bss
	.space	8192
ENTRY(_stack)
