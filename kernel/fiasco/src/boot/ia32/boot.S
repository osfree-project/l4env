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

ENTRY(_mb_header)
        .long   0x1BADB002              /* magic */
        .long   0x00010000              /* flags: AOUT_KLUDGE */
        .long   - 0x00010000 - 0x1BADB002
  .long   _mb_header                    /* header_addr */
        .long   __crt_dummy__           /* load_addr */
        .long   _edata			/* load_end_addr */
        .long   _end			/* bss_end_addr */
        .long   _start	                /* entry */
	
	.bss

	.space	1024
ENTRY(_stack)
