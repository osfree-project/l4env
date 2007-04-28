
#include "config_tcbsize.h"
#include "config_gdt.h"
#include "globalconfig.h"
#include "idt_init.h"
#include "low_level.h"
#include "regdefs.h"
#include "shortcut.h"
#include "tcboffset.h"

.macro  PRE_FAST_ALIEN_IPC
	btrl	$17, OFS__THREAD__STATE (%ebx)	/* Thread_dis_alien */
	jc	1f
	RESTORE_STATE
	popl	%eax
	sub	$6,(%esp)
	andl	$0x7f,4(%esp)
	orl	$EFLAGS_IF,8(%esp)
	sub	$16,12(%esp)
	pushl	$(0x30 << 3 | 2)
	pushl	$(0xd)
	pusha
	jmp	slowtraps

1:	/* do alien IPC and raise a trap afterwards */
	RESET_THREAD_CANCEL_AT %ebx
.endm

.macro  POST_FAST_ALIEN_IPC
	RESTORE_STATE_AFTER_IPC
	popl	%eax
	andl	$0x7f,4(%esp)
	orl	$EFLAGS_IF, 8(%esp)
	pushl	$(0x30 << 3 | 6)
	pushl	$(0xd)
	pusha
	jmp	slowtraps
.endm

	.p2align 4
	.globl	entry_vec01_debug
entry_vec01_debug:
	pushl	%eax
	movl	KSEG CURRENT_SYSENTER,%eax
	cmpl	%eax,4(%esp)
	popl	%eax
	je	2f
1:	pushl	$0
	pushl	$1
	pusha
	jmp	slowtraps

2:	/* We came from an address equal to one of the sysenter entry
	 * points. Did we also come from kernel? */
	testl	$3,4(%esp)
	jne	1b

	/* copy last three dwords to the current kernel stack */
	pushl	%eax
	pushl	%ecx
	movl	20(%esp),%ecx			// x86_tss.esp0
	movl	 8(%esp),%eax			// 3rd dword == EIP
	incl	%eax				// skip ``pop %esp''
	movl	%eax,-12(%ecx)
	movl	12(%esp),%eax			// 2nd dword == CS
	movl	%eax, -8(%ecx)
	movl	16(%esp),%eax			// 1st dword == EFLAGS
	movl	%eax, -4(%ecx)
	popl	%ecx
	popl	%eax
	movl	12(%esp),%esp			// x86_tss.esp0
	subl	$12, %esp			// skip entry frame
	pushl	$0
	pushl	$1
	pusha
	jmp	slowtraps


	.globl	entry_vec0a_invalid_tss
entry_vec0a_invalid_tss:
	andl	$0xffffbfff, 12(%esp)
	addl	$4, %esp			/* skip error code */
	iret


	.globl	entry_vec08_dbf
entry_vec08_dbf:
	testl	$0xffffffff,CPU_DEBUGCTL_BUSY
	jnz	thread_handle_double_fault
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	CPU_DEBUGCTL_RESET, %eax
	xorl	%edx, %edx
	movl	$0x1d9,%ecx
	wrmsr
	popl	%edx
	popl	%ecx
	popl	%eax
	jmp	thread_handle_double_fault


/* PPro spurious interrupt bug: 
 * See "Pentium Pro Processor Specification Update / January 1999"
 * Erratum "Virtual Wire mode through local APIC may cause int 15"
 * This exception can be silently ignored */
	.p2align(4)
	.globl	entry_vec0f_apic_spurious_interrupt_bug
entry_vec0f_apic_spurious_interrupt_bug:
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS			/* scratches ecx, edx */
	incl	apic_spurious_interrupt_bug_cnt
	RESET_USER_SEGMENTS 12(%esp),in_cli	/* scratches ecx */
	popl	%edx
	popl	%ecx
	iret

/* APIC error interrupt */
	.p2align(4)
	.globl	entry_vec3e_apic_error_interrupt
entry_vec3e_apic_error_interrupt:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS			/* scratches ecx, edx */
	leal	12(%esp), %eax			/* &Return_frame */
	call	apic_error_interrupt
	RESET_USER_SEGMENTS 16(%esp),in_cli	/* scratches ecx */
	popl	%edx
	popl	%ecx
	popl	%eax
	iret

/* Intel Architecture Software Developer's Manual Volume 3,
 * Advanced Programmable Interrupt Controller (APIC):
 * Spurious Interrupt: "If at the time the INTA cycle is issued, the
 * interupt that was to be dispensed has become masked (programmed by
 * software), the local APIC will deliver a spurious-interrupt vector." */
	.p2align(4)
	.globl	entry_vec3f_apic_spurious_interrupt
entry_vec3f_apic_spurious_interrupt:
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS			/* scratches ecx, edx */
	incl	apic_spurious_interrupt_cnt
	RESET_USER_SEGMENTS 12(%esp),in_cli	/* scratches ecx */
	popl	%edx
	popl	%ecx
	iret

#if defined(CONFIG_JDB)

	.p2align(4)
	.global entry_sys_fast_ipc_log
entry_sys_fast_ipc_log:
	pop	%esp
	pushl	$(GDT_DATA_USER|SEL_PL_U) /* user ss */
	subl	$4, %esp		/* room for user esp */
#ifdef CONFIG_IO_PROT
	// We must not fake the interrupt flags since we must not
	// loose the current IOPL of the user task
	pushf
#else
	// Fake user eflags, set IOPL to 3
	pushl	$EFLAGS_IOPL_U
#endif
	// Fake user cs. This cs value is never used with exception
	// that the thread is ex_regs'd before we leave with sysexit.
	// lthread_ex_regs has to check user cs for that value. If
	// it is faked, the thread would leave the kernel by sysexit
	// and the thread is in the slow ipc path. Sysexit would
	// adapt the user eip (by subtracting 2) to ensure the user
	// executes the "mov %ebp,%edx" sequence. This is wrong if
	// the thread is ex_regs'd. In that case, we modify the return
	// value from "call dispatch_syscall" to an alternate exit
	// path using "iret".
#if defined(CONFIG_SMALL_SPACES) && defined(CONFIG_IO_PROT)
	pushl	$(GDT_CODE_USER|SEL_PL_U)	/* user cs */
#else
	pushl	$(GDT_CODE_USER|SEL_PL_U|0x80)	/* user cs */
#endif
	pushl	(%ecx)				/* user eip */

	pushl	%eax
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	12(%ecx)			/* save ecx */

	leal	16(%ecx), %edx			/* adapt user esp */
	movl	%edx, 40(%esp)			/* store user esp */

	ESP_TO_TCB_AT %ebx
	RESET_KERNEL_SEGMENTS			/* scratches ecx, edx */
	testl	$Thread_alien, OFS__THREAD__STATE (%ebx)
	jnz	alien_sys_fast_ipc_log
	RESET_THREAD_CANCEL_AT %ebx
	call	*syscall_table
in_slow_ipc5:
	DO_SYSEXIT

alien_sys_fast_ipc_log:
	PRE_FAST_ALIEN_IPC
	push    $ret_from_fast_alien_ipc
	jmp     *syscall_table

	.globl	in_slow_ipc5

#endif // CONFIG_JDB


#if (defined (CONFIG_JDB_LOGGING) || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT))
	.p2align(4)
	.global entry_sys_fast_ipc_c
entry_sys_fast_ipc_c:
	pop	%esp
	pushl	$(GDT_DATA_USER|SEL_PL_U)	/* user ss */
	subl	$4, %esp			/* room for user esp */
#ifdef CONFIG_IO_PROT
	// We must not fake the interrupt flags since we must not
	// loose the current IOPL of the user task
	pushf
#else
	// Fake user eflags, set IOPL to 3
	pushl	$EFLAGS_IOPL_U
#endif
	// Fake user cs. This cs value is never used with exception
	// that the thread is ex_regs'd before we leave with sysexit.
	// lthread_ex_regs has to check user cs for that value. If
	// it is faked, the thread would leave the kernel by sysexit
	// and the thread is in the slow ipc path. Sysexit would
	// adapt the user eip (by subtracting 2) to ensure the user
	// executes the "mov %ebp,%edx" sequence. This is wrong if
	// the thread is ex_regs'd. In that case, we modify the return
	// value from "call dispatch_syscall" to an alternate exit
	// path using "iret".
#if defined(CONFIG_SMALL_SPACES) && defined(CONFIG_IO_PROT)
	pushl	$(GDT_CODE_USER|SEL_PL_U)	/* user cs */
#else
	pushl	$(GDT_CODE_USER|SEL_PL_U|0x80)	/* user cs */
#endif
	pushl	(%ecx)				/* user eip */

	pushl	%eax
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	12(%ecx)			/* save ecx */
	leal	16(%ecx), %edx			/* adapt user esp */
	movl	%edx,40(%esp)			/* store user esp */
	ESP_TO_TCB_AT %ebx
	RESET_KERNEL_SEGMENTS			/* scratches ecx, edx */
	testl	$Thread_alien, OFS__THREAD__STATE (%ebx)
	jnz	alien_sys_fast_ipc_c
	RESET_THREAD_CANCEL_AT %ebx
	call    ipc_short_cut_wrapper
in_sc_ipc2:
	DO_SYSEXIT
#endif

	.globl alien_sys_fast_ipc_c     /* Also used in shortcut */
	.global ret_from_fast_alien_ipc /* used in ex-regs */
/* ex-regs patches the return address of the call to the 'sys_ipc' function
 * in the case of sysenter, because we need to leave the kernel by iret
 * to prevent the need of user-level trampoline code.
 * In the case of alien ipc there must be a special case because otherwise the
 * second alien exception is not generated. Therefore ex-regs uses 
 * 'ret_from_fast_alien_ipc' to determine this case and paches the return
 * address with 'leave_alien_from_sysenter_by_iret' instead of
 * 'leave_from_sysenter_by_iret'. (see Thread::user_ip in thread-ia32-ux.cpp)
 */


alien_sys_fast_ipc_c:
	PRE_FAST_ALIEN_IPC
	call    ipc_short_cut_wrapper
ret_from_fast_alien_ipc:
	POST_FAST_ALIEN_IPC

	.globl	in_sc_ipc2

.macro	LEAVE_SE_BY_IRET
	ESP_TO_TCB_AT %ebx
	RESET_THREAD_IPC_MASK_AT %ebx
	RESET_USER_SEGMENTS $3,in_cli		/* scratches ecx */
	RESTORE_STATE_AFTER_IPC
	popl    %eax
	orl	$EFLAGS_IF, 8(%esp)
.endm

	.p2align(4)
	.globl	leave_from_sysenter_by_iret
leave_from_sysenter_by_iret:
	LEAVE_SE_BY_IRET
	iret

	.globl	leave_alien_from_sysenter_by_iret
leave_alien_from_sysenter_by_iret:
	LEAVE_SE_BY_IRET
	pushl	$(0x30 << 3 | 6)
	pushl	$(0xd)
	pusha
	jmp	slowtraps


	// This code is globally visible at the trampoline page for small
	// address spaces. Because sysexit loads 4GB flat segments, we need
	// to explicit reload the stack segment selector and need to implicit
	// reload the code segment selector using the lret instruction. The
	// "$8" parameter of lret cleans up the stack (see the user-level-ipc
	// bindings for sysenter).
	//
	// It is important that we don't receive interrupts while this code
	// is executed because this code is located beyond the limit of the
	// user-level code segment. If we are interrupted here, the iret
	// instruction of the interrupt handler tries to return using the
	// user-CS which generates a segfault. Therefore sysexit may not be
	// used if both Small Address Spaces and I/O protection are enabled
	// (sti is not allowed for userland tasks if their IOPL is 0).
	//
	// As an alternative we could introduce many special cases before
	// leaving the following handlers using iret: Timer IRQ, device IRQs,
	// pagefault handler, APIC IRQs. Other traps?
	//
	.p2align(4)
	.globl	tramp_small_after_sysexit_func
tramp_small_after_sysexit_func:
	movl	%ebp, %edx
	movl	$(GDT_DATA_USER|SEL_PL_U), %ebp
	movl	%ebp, %ss
	sti
	lret	$8

	.globl	tramp_small_end
tramp_small_end:


	.bss
	.space	4096
	.global	dbf_stack_top
dbf_stack_top:

