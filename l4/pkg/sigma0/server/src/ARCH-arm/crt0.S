
.section .text.init,#alloc,#execinstr
.type _start,#function
.globl _start
_start:
	ldr 	sp, .LCstack
	ldr	lr, .LCexit
	b	init
.LCstack: .word crt0_stack_high
.LCexit:  .word _exit

.section ".bss"

	.global crt0_stack_low
crt0_stack_low:
	.space	2048
	.global crt0_stack_high
crt0_stack_high:
