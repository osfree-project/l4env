/* -*- c -*- */

#define __ASSEMBLY__

#define ENTRY(name) \
  .globl name; \
  .align 16,0x90; \
  name:


ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)
	leal	_stack,%esp
	pushl	%eax
	pushl	%ebx
	pushl	$0		/* no return address */
	jmp	bootstrap

        /* MultiBoot header - see multiboot.h.  */
#ifdef __ELF__
        .align  4
#else /* ! __ELF__, that mean a.out assembler */
        .align  2
#endif

	.p2align(2), 0x90
	.long   0x1BADB002              /* magic */
	.long   0x00000000              /* feature flags */
	.long   0 - 0x1BADB002

	.bss
	.space	1024
ENTRY(_stack)

