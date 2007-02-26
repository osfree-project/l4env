
/*
 * Signal Handler Trampoline for traced tasks.
 *
 * This code is copied onto the task's trampoline page and runs in the task's
 * address space. Therefore all addressing must be done with virtual addresses
 * of the task.
 *
 * The whole purpose of this exercise is to get a pointer to the signal
 * context of the task from inside the kernel process. The signal handler
 * runs at the bottom of the trampoline page with the top of the trampoline
 * page as signal stack. The handler receives the following 3 parameters:
 * int signal, siginfo_t *, void *context
 * We want the third parameter, relative to the beginning of the trampoline
 * page and store it at offset 256 on the trampoline page so that code and data
 * don't cacheline collide.
 */

#define ASSEMBLER

.align	16
.globl	_task_sighandler_start
.globl	_task_sighandler_end

_task_sighandler_start:
	movl	12(%esp), %eax		// get context pointer from sighandler
	andl	$0x0fff, %eax		// calculate offset into trampolinepage
	movl	$_trampoline_page, %ecx	// get trampoline page start address
	movl	%eax, 0x100(%ecx)	// save offset at trampoline page + 256
	ret
_task_sighandler_end:
