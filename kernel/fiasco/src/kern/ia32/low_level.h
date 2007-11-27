#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H

#include "asm.h"
#include "regdefs.h"
#include "shortcut.h"
#include "tcboffset.h"

/* be sure that we do not enable the interrupts here! */
	.macro	RESTORE_IOPL
	pushl	16(%esp)
	andl	$~EFLAGS_IF,(%esp)
	popf
	.endm

/** Some shared macros and stuff shared by the two different 
 *  assembler kernel entry sources.
 */

#ifdef CONFIG_SMALL_SPACES

/** Set up segments when entering the kernel.
 *  If interrupted in kernel mode everything should be
 *  fine already, otherwise segment setting is
 *  mandatory.
 *  We expect GS and FS to be correct, though. 
 *  If not the exception handler corrects them.
 *  (Avoids problems with V4 as well.)
 */
	.macro	RESET_KERNEL_SEGMENTS_FORCE_DS_ES
        movw    $ (GDT_DATA_KERNEL), %cx
        movl    %ecx,%ds
        movl    %ecx,%es
	.endm

/** Set up segments when entering the kernel.
 *  If interrupted in kernel mode everything should be
 *  fine already, otherwise segment setting is
 *  mandatory.
 */
	.macro	RESET_KERNEL_SEGMENTS
        movw    $ (GDT_DATA_KERNEL), %cx
        movl    %ds, %edx
        cmpw    %dx, %cx
        jz      1f
        movl    %ecx,%ds
1:      movl    %es, %edx
        cmpw    %dx, %cx
        jz      2f
        movl    %ecx,%es
2:
	.endm

/** Set up segments when leaving the kernel.
 * If we return to user mode we must always reload
 * the segment registers as the GDT might have changed.
 * But if we return to kernel mode we trust ourselves
 * that the segments are still right.
 */
	.macro	RESET_USER_SEGMENTS csseg,incli
	.ifnc	\csseg,$3
	movl	\csseg, %ecx
	andb	$3, %cl
	jz	2f
	.endif
	movw	$ (GDT_DATA_USER|SEL_PL_U), %cx
	.ifnc	\incli,in_cli
	cli
	.endif
	movl	%ecx,%ds
	movl	%ecx,%es
2:
	.endm

/** With small address spaces, the segment selectors
 * has to be reloaded to go the changes in the gdt
 * into effect. Here we reload ds and es and restore
 * most of the general registers. Then we return to a 
 * trampoline page where we reload ss. Last we do a
 * far return reloading cs. */
	.macro	DO_SYSEXIT
	movw	$ (GDT_DATA_USER|SEL_PL_U), %cx
	movl	%ecx,%ds
	movl	%ecx,%es
	addl	$4, %esp	/* skip ecx */
#ifdef CONFIG_IO_PROT
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	CHECK_SANITY $3		/* scratches ecx */
	addl	$4, %esp	/* skip ebp */
	popl	%eax
	orl	$EFLAGS_IF, 8(%esp)
	iret
#else
	popl	%ebp		/* => edx */
	popl	%esi
	popl	%edi
	popl	%ebx
	CHECK_SANITY $3		/* scratches ecx */
	RESTORE_IOPL
	movl	4(%esp), %eax
	movl	20(%esp), %ecx	/* user esp */
	subl	$16, %ecx
	movl	$VAL__MEM_LAYOUT__SMAS_TRAMPOLINE, %edx
	sysexit
#endif
	.endm

/** Defines access to kernel memory when in 
 *  shortcut path: small address spaces need the 
 *  stack segment here, all others use the 
 *  standard access segment.
 */
#define KSEG	%ss:

#else // !CONFIG_SMALL_SPACES

/** Setting up ds/es resp. fs/gs when entering/
 * leaving the kernel is not neccessary anymore
 * since the user can only load the null selector
 * without exception. But then, the first access
 * to code/data with the wrong selector loaded
 * raises an exception 13 which is handled properly.
 */
	.macro	RESET_KERNEL_SEGMENTS
	.endm


	.macro	RESET_KERNEL_SEGMENTS_FORCE_DS_ES
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx
        movl    %ecx,%ds
        movl    %ecx,%es
	.endm

	.macro	RESET_USER_SEGMENTS csseg,incli
	.endm

	.macro	DO_SYSEXIT
	addl	$4, %esp	/* skip ecx */
	popl	%ebp		/* => edx */
	popl	%esi
	popl	%edi
	popl	%ebx
	CHECK_SANITY $3		/* scratches ecx */
	RESTORE_IOPL
	movl	4(%esp), %eax
	movl	8(%esp), %edx	/* user eip */
	movl	20(%esp), %ecx	/* user esp */
	subl	$2, %edx	/* adj. eip */
	sti			/* the interrupts are enabled _after_ the
				 * next instruction (see Intel Ref-Manual) */
	sysexit
	.endm

/** Defines access to kernel memory when in 
 *  shortcut path: small address spaces need the 
 *  stack segment here, all others use the 
 *  standard access segment.
 */
#define KSEG

#endif //CONFIG_SMALL_SPACES

	.macro	RESET_THREAD_CANCEL_AT reg
	andl	$~(Thread_cancel), KSEG OFS__THREAD__STATE (\reg)
	.endm

	.macro	RESET_THREAD_IPC_MASK_AT reg
	andl	$~Thread_ipc_mask, KSEG OFS__THREAD__STATE (\reg)
	.endm

	.macro	ESP_TO_TCB_AT reg
	movl	%esp, \reg
	andl	$~(THREAD_BLOCK_SIZE - 1), \reg
	.endm

	.macro	SAVE_STATE
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	.endm

	.macro	RESTORE_STATE
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	.endm

	.macro	RESTORE_STATE_AFTER_IPC
	addl	$4, %esp
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4, %esp
	.endm

#define PAGE_FAULT_ADDR	%cr2
#define PAGE_DIR_ADDR	%cr3

#endif //LOW_LEVEL_H
