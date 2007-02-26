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

	/* push parameters before they get trashed by code in crti.h */
	push	%eax
	push	%ebx
	movl	%ebx, _mbi

	/* Clear the base pointer so that stack backtraces will work.  */
	xorl	%ebp,%ebp

	jmp 3f
ENTRY(__oskit_init)

	/* Call all global constructors.  */
	pushl	%ebx
	pushl	%eax
	lea	ctors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx
	ret
3:

	/*
	 * Similar to the '.init' section,
	 * the '.fini' section contains code to run at exit time.
	 */
	.section .fini
	.type SYMBOL_NAME(__oskit_fini),@function
ENTRY(__oskit_fini)

	/* Call all global destructors.  */
	pushl	%ebx
	pushl	%eax
	lea	dtors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx

	/*
	 * When any global variable needing a constructor is declared in C++,
	 * it writes a longword into the '.ctors' section
	 * which is a pointer to the appropriate constructor routine to call.
	 */
	.section .ctors,"a",@progbits
	.p2align(2)
	.long	0
ctors:

	/*
	 * Similarly, the '.dtors' section is a list of pointers
	 * to destructor routines for global C++ objects.
	 */
	.section .dtors,"a",@progbits
	.p2align(2)
	.long	0
dtors:

	/*
	 * Finally, the '.anno' section is our own little invention;
	 * it allows generic program annotations of many different kinds
	 * to be collected together by the linker into "annotation tables";
	 * see oskit/anno.h for more information.
	 */
	.section .anno,"aw",@progbits
	.p2align(4)
ENTRY(__ANNO_START__)

	/*
	 * Now back to our normal programming...
	 */
	.text

	/* call l4util's _main to parse command line */
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
        .long   _mb_header              /* header_addr */
        .long   SYMBOL_NAME(__crt_dummy__) /* load_addr */
        .long   SYMBOL_NAME(_edata)	/* load_end_addr */
        .long   SYMBOL_NAME(_end)	/* bss_end_addr */
        .long   _start	                /* entry */
	
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

