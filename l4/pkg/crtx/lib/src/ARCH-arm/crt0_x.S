/* -*- c -*- */

/*#include <l4/sys/compiler.h>*/

.section .text.init,#alloc,#execinstr
.type _start,#function
.globl _start
_start:
     adr r5, __init_data;
     ldr r6,[r5]
     mov sp,r6
     nop
     nop
     bl	__main
1:     
     b 1b

     /* never returns */

.type __init_data, %object
__init_data:
.long _stack-4


.section ".bss"
.globl _sstack
_sstack:
	.space	4096
.globl _stack
.type _stack,#object
_stack:
