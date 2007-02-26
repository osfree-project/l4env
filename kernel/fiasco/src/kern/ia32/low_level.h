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
 *  mendatory.
 */
#define RESET_KERNEL_SEGMENTS(csseg) \
        movl    csseg, %ecx                     ;\
        andb    $3, %cl                         ;\
        jz      2f                              ;\
        movw    $ (GDT_DATA_KERNEL), %cx        ;\
        movl    %ecx,%ds                        ;\
        movl    %ecx,%es                        ;\
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        movl    %fs,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      1f                              ;\
        movl    %ecx,%fs                        ;\
1:      movl    %gs,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      2f                              ;\
        movl    %ecx,%gs                        ;\
2:

/** Set up segments when leaving the kernel.
 * If we return to user mode we must always reload
   the segment registers as the GDT might have changed.
   But if we return to kernel mode we trust ourselfs
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

#else //CONFIG_SMALL_SPACES

/** Set up segments when entering the kernel
 *  the Traditional Way. That is only to correct
 *  things if they have been messed up.
 * csseg can be ignored here.
 */
#define RESET_KERNEL_SEGMENTS(csseg) \
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        movl    %ds,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      1f                              ;\
        movl    %ecx,%ds                        ;\
1:      movl    %es,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      1f                              ;\
        movl    %ecx,%es                        ;\
1:

/** Set up segments when leaving the kernel.
 *  csseg can be ignored here.
 */
#define RESET_USER_SEGMENTS(csseg) \
        movw    $ (GDT_DATA_USER|SEL_PL_U), %cx ;\
        movl    %fs,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      1f                              ;\
        movl    %ecx,%fs                        ;\
1:      movl    %gs,%edx                        ;\
        cmpw    %cx,%dx                         ;\
        jz      1f                              ;\
        movl    %ecx,%gs                        ;\
1:

#endif //CONFIG_SMALL_SPACES

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

#define SYSEXIT_AFTER_DISPATCH_SYSCALL \
	addl	$16, %esp	/* skip ecx */	;\
	popl	%ebp				;\
	popl	%esi				;\
	popl	%edi				;\
	popl	%ebx				;\
	movl	4(%esp), %eax			;\
	movl	8(%esp), %edx	/* user eip */	;\
	movl	20(%esp), %ecx	/* user esp */	;\
	subl	$2, %edx	/* adj. eip */	;\
	sti					;\
	sysexit					;\

/** ebx is still loaded with current */
#define SYSEXIT_AFTER_SHORTCUT \
	popl	%ebx				;\
	addl	$12, %esp	/* skip ecx */	;\
	andl	$~Thread_ipc_mask, OFS__THREAD__STATE (%ebx) ;\
	popl	%ebp				;\
	popl	%esi				;\
	popl	%edi				;\
	popl	%ebx				;\
	movl	4(%esp), %eax			;\
	movl	8(%esp), %edx	/* user eip */	;\
	movl	20(%esp), %ecx	/* user esp */	;\
	subl	$2, %edx	/* adj. eip */	;\
	sti					;\
	sysexit					;\

#endif //LOW_LEVEL_H

