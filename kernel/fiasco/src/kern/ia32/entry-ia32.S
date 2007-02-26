#define ASSEMBLER

#include <flux/x86/asm.h>
#include <flux/x86/trap.h>
#include <flux/x86/seg.h>
#include <flux/x86/gate_init.h>
#include <flux/x86/base_trap.h>

#include "config_tcbsize.h"
#include "config_gdt.h"
#include "globalconfig.h"
#include "low_level.h"
#include "shortcut.h"
#include "tcboffset.h"

	P2ALIGN(4)
	.globl	debug_exception
debug_exception:
	pushl	%eax
	movl	CURRENT_SYSENTER,%eax
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
	movl	20(%esp),%ecx	// x86_tss.esp0
	movl	 8(%esp),%eax	// 3rd dword == EIP
	incl	%eax		// skip ``pop %esp''
	movl	%eax,-12(%ecx)
	movl	12(%esp),%eax	// 2nd dword == CS
	movl	%eax, -8(%ecx)
	movl	16(%esp),%eax	// 1st dword == EFLAGS
	movl	%eax, -4(%ecx)
	popl	%ecx
	popl	%eax
	movl	12(%esp),%esp	// x86_tss.esp0
	subl	$12, %esp	// skip entry frame
	pushl	$0
	pushl	$1
	pusha
	jmp	slowtraps


	.globl	invalid_tss_entry
invalid_tss_entry:
	andl	$0xffffbfff, 12(%esp)
	addl	$4, %esp	/* skip error code */
	iret


	.globl	dbf_entry
dbf_entry:
	call	thread_handle_double_fault
	jmp	dbf_entry


/* PPro spurious interrupt bug: 
 * See "Pentium Pro Processor Specification Update / January 1999"
 * Erratum "Virtual Wire mode through local APIC may cause int 15"
 * This exception can be silently ignored */
	P2ALIGN(4)
	.globl	apic_spurious_interrupt_bug_entry
apic_spurious_interrupt_bug_entry:
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS	/* scratches cx, dx */
	incl	apic_spurious_interrupt_bug_cnt
	RESET_USER_SEGMENTS(12(%esp))
	popl	%edx
	popl	%ecx
	iret

/* APIC error interrupt */
	P2ALIGN(4)
	.globl	apic_error_interrupt_entry
apic_error_interrupt_entry:
	pushl	%eax		/* save registers modifiable by C functions */
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS	/* scratches cx, dx */
	leal	12(%esp), %ecx
	pushl	%ecx
	call	apic_error_interrupt
	addl	$4, %esp
	RESET_USER_SEGMENTS(16(%esp))
	popl	%edx
	popl	%ecx
	popl	%eax
	iret

/* Intel Architecture Software Developer's Manual Volume 3,
 * Advanced Programmable Interrupt Controller (APIC):
 * Spurious Interrupt: "If at the time the INTA cycle is issued, the
 * interupt that was to be dispensed has become masked (programmed by
 * software), the local APIC will deliver a spurious-interrupt vector." */
	P2ALIGN(4)
	.globl	apic_spurious_interrupt_entry
apic_spurious_interrupt_entry:
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS	/* scratches cx, dx */
	incl	apic_spurious_interrupt_cnt
	RESET_USER_SEGMENTS(12(%esp))
	popl	%edx
	popl	%ecx
	iret

#if defined(CONFIG_JDB)

	P2ALIGN(4)
	.global do_sysenter_log
do_sysenter_log:
	pop	%esp
	pushl	$(GDT_DATA_USER|SEL_PL_U)	/* user ss */
	subl	$4, %esp			/* room for user esp */
	pushl	$0x3200				/* fake user eflags */
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
	pushl	$(GDT_CODE_USER|SEL_PL_U|0x80)	/* user cs */
	pushl	(%ecx)				/* user eip */

	pushl	$0x00003000
	popf

	pushl	%eax			/* SAVE_STATE */
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	12(%ecx)		/* save ecx */

	leal	16(%ecx), %edx		/* adapt user esp */
	movl	%edx, 40(%esp)		/* store user esp */

	ESP_TO_TCB_AT(%ebx)
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	RESET_THREAD_CANCEL_AT(%ebx)

	call	*syscall_table
in_slow_ipc5:

	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx,dx */
	SYSEXIT

#endif // CONFIG_JDB


#if defined (CONFIG_JDB_LOGGING) \
    || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
	P2ALIGN(4)
	.global do_sysenter_c
do_sysenter_c:
	pop	%esp
	pushl	$(GDT_DATA_USER|SEL_PL_U)	/* user ss */
	subl	$4, %esp			/* room for user esp */
	pushl	$0x3200				/* fake user eflags */
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
	pushl	$(GDT_CODE_USER|SEL_PL_U|0x80)	/* user cs */
	pushl	(%ecx)				/* user eip */

	pushl	%eax				/* SAVE_STATE */
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	12(%ecx)			/* save ecx */

	pushl	$0x00003000
	popf

	leal	16(%ecx), %edx		/* adapt user esp */
	movl	%edx,40(%esp)		/* store user esp */

	ESP_TO_TCB_AT(%ebx)
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	RESET_THREAD_CANCEL_AT(%ebx)
	call    ipc_short_cut_wrapper
in_sc_ipc2:
	ESP_TO_TCB_AT(%ebx)
	RESET_THREAD_IPC_MASK_AT(%ebx)
	cmpb	$0,%al
	jz	sysenter_shortcut_failed
	/* ipc_short_cut returned true -> ipc finished */
	SYSEXIT

sysenter_shortcut_failed:
	call	sys_ipc_wrapper
in_slow_ipc6:
	SYSEXIT
#endif


	P2ALIGN(4)
	.globl	leave_from_sysenter_by_iret
leave_from_sysenter_by_iret:
	ESP_TO_TCB_AT(%ebx)
	RESET_THREAD_IPC_MASK_AT(%ebx)
	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx, dx */
	RESTORE_STATE
	popl    %eax
	iret


	// This code is global visible at the trampoline page for
	// small address spaces. We need to explicit reload the stack
	// segment selector and to implicit reload the code segment
	// selector by the lret instruction. The "$8" parameter of
	// lret cleans up the stack (see the user-level-ipc bindings
	// for sysenter)
	P2ALIGN(4)
	.globl	tramp_small_after_sysexit_func
tramp_small_after_sysexit_func:
	movl	%ebp, %edx
	movl	$ (GDT_DATA_USER|SEL_PL_U), %ebp
	movl	%ebp, %ss
	sti
	lret	$8

	.globl	tramp_small_end
tramp_small_end:


	// these labels help show_tcb to guess the thread state
	.globl	in_sc_ipc2
	.globl	in_slow_ipc5
	.globl	in_slow_ipc6

