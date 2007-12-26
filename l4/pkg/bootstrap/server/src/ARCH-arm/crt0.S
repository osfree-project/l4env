/* -*- c -*- */

/*#include <l4/sys/compiler.h>*/

.macro enter_kdebug
    	mov	lr, pc	
    	mov	pc, #-0x00000020-8
    	b	1f
    	.ascii	" crtx "
    	.byte	0	
    	.align	4
    1:
.endm

.section .text.init,#alloc,#execinstr
.type _start,#function
.globl _start
_start:
	ldr 	r3, .LCcrt0_tramppage      /* Load address of tramppage var */
     	str 	sp, [r3]                   /* Store SP in variable          */
	ldr 	sp, .LCstack
     	ldr 	r3, .LCcrt0_multiboot_flag /* Load address of flag variable */
	str 	r0, [r3]                   /* Store value in flag variable  */
	ldr 	r3, .LCcrt0_multiboot_info /* Load address of mbi variable  */
	str 	r1, [r3]                   /* Store pointer in mbi variable */
	ldr 	r3, .LCcrt0_l4env_infopage /* Load address of l4env_infopage variable */
	str 	r2, [r3]                   /* Store pointer in l4env_infopage variable */

     bl	__main
1:     
     b 1b

.LCcrt0_tramppage:      .word crt0_tramppage
.LCcrt0_multiboot_info: .word crt0_multiboot_info
.LCcrt0_multiboot_flag: .word crt0_multiboot_flag
.LCcrt0_l4env_infopage: .word crt0_l4env_infopage
.LCstack: .word crt0_stack_high

.section ".bss"

	.global	crt0_tramppage
crt0_tramppage:
	.space 4

	.global	crt0_multiboot_flag
crt0_multiboot_flag:
	.space 4

	.global	crt0_multiboot_info
crt0_multiboot_info:
	.space	4

	.global	crt0_l4env_infopage
crt0_l4env_infopage:
	.space	4

	.global crt0_stack_low
crt0_stack_low:
	.space	8192
	.global crt0_stack_high
crt0_stack_high:
