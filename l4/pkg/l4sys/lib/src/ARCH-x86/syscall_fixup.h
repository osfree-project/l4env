
	.macro	fixup_syscall syscall,offset
	.pushsection .data.l4sys
	.align  8
\syscall:
	.byte 0xe9
	.long \offset
	nop
	nop
	nop
	.hidden \syscall
	.global \syscall
	.endm

