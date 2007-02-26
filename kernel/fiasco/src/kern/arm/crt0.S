/* -*- c -*- */

#include "asm.h"
.section .text.init,#alloc,#execinstr
.type start,#function
ENTRY(_start)
ENTRY(start)
     
     adr r5, __init_data;
     ldr r6,[r5]
     mov sp,r6
     nop
     nop

     bl	__main

     /* never returns */


.type __init_data, %object
__init_data:
.long SYMBOL_NAME(_stack-4)



.section ".init.data"
.globl _sstack
_sstack:
	.space	2048
.globl _stack
.type _stack,#object
_stack:
