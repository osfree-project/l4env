#include <l4/sys/compiler.h>
	
	.section .init

	.globl	_start
_start:
#if defined(REALMODE_LOADING) && !defined(IMAGE_MODE)
	cld
	cli
	mov	$(3 * 8), %eax
	mov	%eax, %ds
	mov	%eax, %es
	mov	%eax, %fs
	mov	%eax, %gs

	lss	_stack_seg, %esp
#else
	lea	_stack,%esp
#endif

	push	$_exit
	jmp	__main


        /* MultiBoot header - see multiboot.h.  */
	.p2align(2)

_mb_header:
        .long   0x1BADB002		/* magic */
        .long   0			/* flags: AOUT_KLUDGE */
        .long   0 - 0x1BADB002

#if defined(REALMODE_LOADING) && !defined(IMAGE_MODE)
_stack_seg:
	.long	_stack
	.word	(3 * 8)
#endif

	.bss

	.globl  _stack
	.space	8192
_stack:
