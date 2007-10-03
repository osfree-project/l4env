#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.section .init

	.globl	start
	.globl	_start
start:
_start:
#if defined(ARCH_x86)
	leal	_stack, %esp
	jmp	init
#elif defined (ARCH_amd64)
	lea	_stack, %rsp
	jmp	init
#elif defined (ARCH_arm)
	ldr	sp, .LCstack_top
	b	init
.LCstack_top: .word _stack
#else

#error Unknown architecture!

#endif

	.bss
	.align	8
	.space	12288
ENTRY(_stack)
