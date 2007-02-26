#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H

/** Some shared macros and stuff shared by the two different 
 *  assembler kernel entry sources.
 */

#define ASSEMBLER

#define RESET_KERNEL_SEGMENTS	/* n/a */
#define RESET_KERNEL_SEGMENTS_FORCE_DS_ES /* n/a */
#define RESET_USER_SEGMENTS(csseg)	/* n/a */
#define RESET_USER_SEGMENTS_FORCE_USER  /* n/a */

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

#define RESET_THREAD_CANCEL_AT(reg)		\
	andl	$~Thread_cancel, OFS__THREAD__STATE(reg)

#define RESET_THREAD_IPC_MASK_AT(reg)		\
	andl	$~Thread_ipc_mask, OFS__THREAD__STATE(reg)
        
#define ESP_TO_TCB_AT(reg)			\
	movl	%esp,reg			;\
	andl	$~(THREAD_BLOCK_SIZE - 1),reg

/** Defines access to kernel memory when in 
 *  shortcut path: small address spaces need the 
 *  stack segment here, all others use the 
 *  standard access segment
 */
#define KSEG(mem) mem

#endif //LOW_LEVEL_H
