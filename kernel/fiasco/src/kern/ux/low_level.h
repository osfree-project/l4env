#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H

/** Some shared macros and stuff shared by the two different 
 *  assembler kernel entry sources.
 */

#define ASSEMBLER

#define RESET_KERNEL_SEGMENTS			/* not applicable */
#define RESET_USER_SEGMENTS			/* not applicable */

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

#endif //LOW_LEVEL_H

