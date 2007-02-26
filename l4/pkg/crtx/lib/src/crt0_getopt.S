/* $Id$ */
#define __ASSEMBLY__
#include <l4/sys/compiler.h>
	
	.text

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)
	/* set up initial stack */
	leal	stack,%esp

	/* call l4util's _main */
	pushl   %eax
        pushl   %ebx
	movl	%ebx, _mbi
        call    _main
	addl	$8,%esp
	
	/* call program's main */
        pushl   $_argv
        pushl   _argc
        call    main
	addl	$8,%esp

1:	/* fall asleep */
	xorl	%ecx,%ecx
	xorl	%eax,%eax
	movl	%eax,%ebp
	movl	%eax,%esi
	movl	%eax,%edi
	decl	%eax
	int	$0x30
        jmp     1b


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
	
	.bss

ENTRY(_mbi)
	.space	4

	.space	(1024*64)
stack:

