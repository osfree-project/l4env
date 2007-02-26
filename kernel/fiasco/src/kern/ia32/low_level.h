#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H

/** Some shared macros and stuff shared by the two different 
 *  assembler kernel entry sources.
 */

#define ASSEMBLER

#ifdef CONFIG_SMALL_SPACES

/** Set up segments when entering the kernel.
 *  If interupted in kernel mode everything should be
 *  fine already, otherwise segment setting is
 *  mandatory.
 *  We expect GS and FS to be correct, though. 
 *  If not the exception handler corrects them.
 *  (Avoids problems with V4 as well.)
 */
#define RESET_KERNEL_SEGMENTS_FORCE_DS_ES \
        movw    $ (GDT_DATA_KERNEL), %cx        ;\
        movl    %ecx,%ds                        ;\
        movl    %ecx,%es                  

/** Set up segments when entering the kernel.
 *  If interupted in kernel mode everything should be
 *  fine already, otherwise segment setting is
 *  mandatory.
 */
#define RESET_KERNEL_SEGMENTS \
        movw    $ (GDT_DATA_KERNEL), %cx        ;\
        movl    %ds, %edx                       ;\
        cmpw    %dx, %cx                        ;\
        jz      1f                              ;\
        movl    %ecx,%ds                        ;\
1:      movl    %es, %edx                       ;\
        cmpw    %dx, %cx                        ;\
        jz      2f                              ;\
        movl    %ecx,%es                        ;\
2:

/** Set up segments when leaving the kernel.
 * If we return to user mode we must always reload
   the segment registers as the GDT might have changed.
   But if we return to kernel mode we trust ourselves
   that the segments are still right.
 */
#define RESET_USER_SEGMENTS(csseg) \
        movl    csseg, %ecx                     ;\
        andb    $3, %cl                         ;\
        jz      2f                              ;\
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        cli                                     ;\
        movl    %ecx,%ds                        ;\
        movl    %ecx,%es                        ;\
2:

/** The version when we are sure to return to user mode.
 */
#define RESET_USER_SEGMENTS_FORCE_USER \
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        cli                                     ;\
        movl    %ecx,%ds                        ;\
        movl    %ecx,%es                        ;\
2:



/** With small address spaces, the segment selectors
 * has to be reloaded to go the changes in the gdt
 * into effect. Here we reload ds and es and restore
 * most of the general registers. Then we return to a 
 * trampoline page where we reload ss. Last we do a
 * far return reloading cs. */
#define SYSEXIT					\
	movw	$ (GDT_DATA_USER|SEL_PL_U), %cx	;\
	movl	%ecx,%ds			;\
	movl	%ecx,%es			;\
	addl	$4, %esp	/* skip ecx */	;\
	popl	%ebp		/* => edx */	;\
	popl	%esi				;\
	popl	%edi				;\
	popl	%ebx				;\
	movl	4(%esp), %eax			;\
	movl	20(%esp), %ecx	/* user esp */	;\
	subl	$16, %ecx			;\
	movl	$(0xeac00000 + 0x3000), %edx	;\
	sysexit					;\

/** Defines access to kernel memory when in 
 *  shortcut path: small address spaces need the 
 *  stack segment here, all others use the 
 *  standard access segment
 */
#define KSEG(mem) %ss:mem

#else //CONFIG_SMALL_SPACES

/** Setting up ds/es resp. fs/gs when entering/
 * leaving the kernel is not neccessary anymore
 * since the user can only load the null selector
 * without exception. But then, the first access
 * to code/data with the wrong selector loaded
 * raises an exception 13 which is handled properly.
 */
#define RESET_KERNEL_SEGMENTS
#define RESET_KERNEL_SEGMENTS_FORCE_DS_ES \
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        movl    %ecx,%ds                        ;\
        movl    %ecx,%es                        ;\

#define RESET_USER_SEGMENTS(csseg)
#define RESET_USER_SEGMENTS_FORCE_USER

#define SYSEXIT					\
	addl	$4, %esp	/* skip ecx */	;\
	popl	%ebp		/* => edx */	;\
	popl	%esi				;\
	popl	%edi				;\
	popl	%ebx				;\
	movl	4(%esp), %eax			;\
	movl	8(%esp), %edx	/* user eip */	;\
	movl	20(%esp), %ecx	/* user esp */	;\
	subl	$2, %edx	/* adj. eip */	;\
	sti					;\
	sysexit					;\
	  
/** Defines access to kernel memory when in 
 *  shortcut path: small address spaces need the 
 *  stack segment here, all others use the 
 *  standard access segment
 */
#define KSEG(mem) mem

#endif //CONFIG_SMALL_SPACES

#define RESET_THREAD_CANCEL_AT(reg)		\
	andl	$~Thread_cancel, KSEG(OFS__THREAD__STATE(reg))

#define RESET_THREAD_IPC_MASK_AT(reg)		\
	andl	$~Thread_ipc_mask, KSEG(OFS__THREAD__STATE (reg))

#define ESP_TO_TCB_AT(reg)			\
	movl	%esp,reg			;\
	andl	$~(THREAD_BLOCK_SIZE - 1),reg

#define SAVE_STATE				\
	pushl	%ebp				;\
	pushl	%ebx				;\
	pushl	%edi				;\
	pushl	%esi				;\
	pushl	%edx				;\
	pushl	%ecx

#define RESTORE_STATE				\
	popl	%ecx				;\
	popl	%edx				;\
	popl	%esi				;\
	popl	%edi				;\
	popl	%ebx				;\
	popl	%ebp				;\

#define PAGE_FAULT_ADDR	%cr2
#define PAGE_DIR_ADDR	%cr3

#endif //LOW_LEVEL_H
