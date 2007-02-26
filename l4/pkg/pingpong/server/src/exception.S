
	.globl	dummy_exception13_handler
	.align	16
dummy_exception13_handler:
	add	$4, %esp		// pop error code
	add	$2, (%esp)		// go to next instruction after int 13
	iret

#ifndef FIASCO_UX
	.globl	exception6_handler
	.align	16
exception6_handler:
	pusha
	call	exception6_c_handler
	popa
	iret
#endif

