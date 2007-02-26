/* $Id$ */
#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.text

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)
	/* the region mapper has to know the position of the trampoline page */
	movl	%esp, _tramppage
	andl	$0xFFFFF000, _tramppage
	
	/* set our own stack */
	leal	stack,%esp
	
	/* call l4util's _main to parse command line */
	pushl	%eax
	pushl	%ebx
	movl	%ebx, _mbi
	call	_main
	addl	$8,%esp
	
	/* init region mapper, thread library ... */
        call    l4env_startup

1:      
        int   $3
        jmp   2f
        .ascii "Done."
2:      
        jmp   1b



ENTRY(__main)
	ret

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
        .long   SYMBOL_NAME(_mb_header) /* header_addr */
        .long   SYMBOL_NAME(__crt_dummy__) /* load_addr */
        .long   _edata			/* load_end_addr */
        .long   _end			/* bss_end_addr */
        .long   SYMBOL_NAME(_start)	/* entry */
	
	.data

ENTRY(_tramppage)
	.space	4

ENTRY(_mbi)
	.space	4


	.bss

ENTRY(stack_low)
	.space	(1024*16)
stack:
ENTRY(stack_high)

