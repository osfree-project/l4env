/*
 * This file contains a 'gate_init' initialization table
 * to initialize the x86 processor trap vectors to default entrypoints.
 * These entrypoints simply push a standard trap_state frame
 * and jump to the 'trap_handler' routine.
 */

#include "config_tcbsize.h"
#include "config_gdt.h"
#include "globalconfig.h"
#include "idt_init.h"
#include "low_level.h"
#include "shortcut.h"
#include "tcboffset.h"

#include "asm.h"

	.macro save_scratch_regs
	push %rax
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	.endm

	.macro restore_scratch_regs
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rax
	.endm

	.macro save_all_regs
	push	%rax
	push	%rcx
	push	%rdx
	push	%rbx
	mov	%cr2,%rax
	push	%rax
	push	%rbp
	push	%rsi
	push	%rdi
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	.endm

	.macro restore_all_regs
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	add	$0x08,%rsp		/* pop cr2 */
	pop	%rbx
	pop	%rdx
	pop	%rcx
	pop	%rax
	.endm



/* We make the trap handler an interrupt gate, because for debugging
   purposes, we don't want any interrupts to occur until they're
   explicitly enabled in the base_trap_handler (usually
   Thread::handle_slow_trap). */

/*
 * No error code.  Clear error code and push trap number.
 */
#define	EXCEPTION(n,name)					\
	GATE_ENTRY(n,entry_##name,ACC_PL_K | ACC_INTR_GATE)	;\
	.p2align 3			      	 		;\
entry_##name:							;\
	push	$(0)						;\
	push	$(n)						;\
	save_all_regs						;\
	jmp	_slowtraps

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name)					\
	GATE_ENTRY(n,entry_##name,ACC_PL_U | ACC_INTR_GATE)	;\
	.p2align 3			      	 		;\
entry_##name:							;\
	push	$(0)						;\
	push	$(n)						;\
	save_all_regs						;\
	jmp	_slowtraps

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name)					\
	GATE_ENTRY(n,entry_##name,ACC_PL_K | ACC_INTR_GATE)	;\
	.p2align 3			      	 		;\
entry_##name:							;\
	push	$(n)						;\
	save_all_regs						;\
	jmp	_slowtraps

GATE_INITTAB_BEGIN(idt_init_table)

EXCEPTION(0x00,vec00_zero_div)
/* IA32 has to handle breakpoint exceptions if occured exactly at 
   entry_sys_fast_ipc -- see ia32/entry-ia32.S */
GATE_ENTRY(0x01,entry_vec01_debug,ACC_PL_K | ACC_INTR_GATE)
/* XXX IA32 has to handle NMI occured exactly at entry_sys_fast_ipc */
EXCEP_USR(0x02,vec02_nmi)
EXCEP_USR(0x03,vec03_breakpoint)
EXCEP_USR(0x04,vec04_into)
EXCEP_USR(0x05,vec05_bounds)
EXCEPTION(0x06,vec06_invop)
/* EXCEPTION(0x07,nofpu) */
// XXX we can't use a task gate, instead we must use IST
GATE_ENTRY(0x08,entry_vec08_dbf, ACC_PL_K | ACC_INTR_GATE)
EXCEPTION(0x09,vec09_fpu_ovfl)
/* EXCEP_ERR(0x0a,vec0a_inv_tss) */
EXCEP_ERR(0x0b,vec0b_segnp)
EXCEP_ERR(0x0c,vec0c_stack_fault)
EXCEP_ERR(0x0d,vec0d_gen_prot)
/* EXCEP_ERR(0x0e,vec0e_page_fault) */
/* EXCEPTION(0x0f,vec0f_trap_0f) */
EXCEPTION(0x10,vec10_fpu_err)
EXCEP_ERR(0x11,vec11_align)
EXCEPTION(0x12,vec12_mcheck)
EXCEPTION(0x13,vec13_simd_err)


	.p2align 4
	.type	slowtraps,@function
	.globl	slowtraps
	/* We have to introduce the label _slowtraps besides the label
	   slowtraps to achive that jmps from exception entry points
	   are optimized to two-byte jmps. The label slowtraps is visible
	   from outside. */
_slowtraps:
slowtraps:
	mov	%rsp,%rdi		/* 1st arg: address of trap_state */
#ifndef CONFIG_NO_FRAME_PTR
	mov     0x90(%rsp),%rax		/* create artificial stack frame */
	push    %rax			/* push rip */
	push	%rbp
# ifndef CONFIG_PROFILE
	lea	(%rsp),%rbp
# endif
#endif

        /* Call the C handler function if one has been installed.  */
	mov	BASE_TRAP_HANDLER, %rax
	or	%rax,%rax
	jz	unexpected_trap_pop
	call	*%rax
in_slowtrap:

#ifndef CONFIG_NO_FRAME_PTR
	lea	0x10(%rsp),%rsp		/* remove frame pointer	*/
#endif

	/* If the handler function returned zero (success),
	   then resume execution as if the trap never happened.
	   Otherwise, just panic.  */
	or	%rax,%rax
	jnz	unexpected_trap

	restore_all_regs
	add	$0x10,%rsp		/* pop trap number and error code */
	iretq

unexpected_trap_pop:
#ifndef CONFIG_NO_FRAME_PTR
	lea	0x10(%rsp), %rsp	/* remove stack frame */
#endif

unexpected_trap:
	mov	%rsp,%rdi		/* 1st arg: trap state */
	call	trap_dump_panic

GATE_ENTRY(0x0e,entry_vec0e_page_fault,ACC_PL_K | ACC_INTR_GATE)

/* we must save %cr2 before we can be preempted -- therefore we're an
   interrupt gate (invoked with interrupts turned off).  Also, we
   don't turn them on again here, but only after checking for
   page-ins from the global page directory in thread_page_fault().
   XXX: If you make changes to stack layout here, fix thread_page_fault */

/* XXX slow version - sets up nice stack frame for debugger */

	.p2align 4
	.type	entry_vec0e_page_fault,@function
.globl entry_vec0e_page_fault
entry_vec0e_page_fault:
	save_scratch_regs

/* We must reset the cancel flag here atomically 
   if we are entering fresh from user mode and an IPC might occur. 
   NOTE: We cannot test the user-mode bit in the error code because
   it will flag "kernel" in case an I/O-bitmap page is not mapped
   during an I/O access. */

	mov     0x58(%rsp),%rcx   /* get CS from stack */
        andb    $3,%cl		/* retrieve current privilege level (CPL) */
	jz	1f		/* CPL == 0 -> kernel, skip resetting state */
	ESP_TO_TCB_AT %rcx
	RESET_THREAD_CANCEL_AT %rcx
1:
	leaq	0x50(%rsp),%r8		/* arg5: ptr to return frame */
	mov     PAGE_FAULT_ADDR,%rdi	/* arg1: page fault address */
	mov     0x48(%rsp),%rsi		/* arg2: error code */
	mov	0x50(%rsp),%rdx		/* arg3: rip */
	mov	0x60(%rsp),%rcx		/* arg4: rflags */
	call	thread_page_fault

in_page_fault:
	or	%rax,%rax
	jz	bad_page_fault
	restore_scratch_regs
	add	$8,%rsp				/* remove error code */
	iretq

/* If code or stack from a small address space are not yet mapped in the
   current page directory we might get a page fault on return from the
   trampoline page. In this case we cannot return to the trampoline page
   after handling the fault because we are already in user mode (with
   segment limits below kernel space) while the trampoline code is located
   in kernel data space. So instead we change ESP and EIP to point to the
   address the trampoline wanted to return to and do the normal IRET. */

/* recover from a bad page fault by invoking the slow_trap handler */
	.p2align 4
bad_page_fault:
	cli
	/* we have on stack: r8, rdi, rsi, rdx, rcx, rax, error code
	   move registers down to make room for trap number 
	   and build complete trap state before  jumping to trap handler */
	restore_scratch_regs
	pushq	$0x0e
	save_all_regs
	jmp	slowtraps


/* FPU not available in this context. */
GATE_ENTRY(0x07,entry_vec07_fpu_unavail, ACC_PL_K | ACC_INTR_GATE)

/* do all of this with disabled interrupts */
	.p2align 4
	.type	entry_vec07_fpu_unavail,@function
entry_vec07_fpu_unavail:
	save_scratch_regs
	mov 	72(%rsp), %rdi
	call	thread_handle_fputrap
in_handle_fputrap:
	test 	%eax,%eax
	jz	real_fpu_exception
	restore_scratch_regs
	iretq
real_fpu_exception:
	restore_scratch_regs
	pushq	$(0)
	pushq	$(7)
	save_all_regs
	jmp	_slowtraps


/* timer interrupt */
#ifdef CONFIG_SCHED_PIT
GATE_ENTRY(0x20,entry_int_timer,ACC_PL_K | ACC_INTR_GATE)
#endif
#ifdef CONFIG_SCHED_RTC
GATE_ENTRY(0x28,entry_int_timer,ACC_PL_K | ACC_INTR_GATE)
#endif
#ifdef CONFIG_SCHED_APIC
GATE_ENTRY(0x3d,entry_int_timer,ACC_PL_K | ACC_INTR_GATE)
#endif

	.p2align 4
	.globl	entry_int_timer
entry_int_timer:
	save_scratch_regs
do_timer_interrupt:
	mov	72(%rsp),%rdi			/* pass rip for logging */
	call	thread_timer_interrupt		/* enter with disabled irqs */
in_timer_interrupt:
	restore_scratch_regs
	iretq

	.p2align 4
	.globl	entry_int_timer_slow
entry_int_timer_slow:
	save_scratch_regs
	call	thread_timer_interrupt_slow	/* enter with disabled irqs */
in_timer_interrupt_slow:
	jmp	do_timer_interrupt

	
	.p2align 4
	.globl	entry_int_timer_stop
entry_int_timer_stop:
	save_scratch_regs
	call	thread_timer_interrupt_stop
	restore_scratch_regs
	iretq

/* other interrupts */

#define INTERRUPT(int,name)					\
	GATE_ENTRY(int,entry_##name,ACC_PL_K | ACC_INTR_GATE) 	;\
	.p2align 3			      	 		;\
entry_##name:				      	 		;\
	save_scratch_regs					;\
	mov	0x28(%rsp),%rsi					;\
	mov	$ (int - 0x20),%rdi				;\
	jmp	all_irqs

	.p2align 4
	.type	all_irqs,@function
all_irqs:
	call	irq_interrupt			/* enter with disabled irqs */
in_interrupt:
	restore_scratch_regs

entry_int_pic_ignore:
	iretq

	.global	entry_int_pic_ignore
	.global	entry_int7
	.global	entry_intf

#ifndef CONFIG_SCHED_PIT
INTERRUPT(0x20,int0)
#endif
INTERRUPT(0x21,int1)
INTERRUPT(0x22,int2)
INTERRUPT(0x23,int3)
INTERRUPT(0x24,int4)
INTERRUPT(0x25,int5)
INTERRUPT(0x26,int6)
INTERRUPT(0x27,int7)
#ifndef CONFIG_SCHED_RTC
INTERRUPT(0x28,int8)
#endif
INTERRUPT(0x29,int9)
INTERRUPT(0x2a,inta)
INTERRUPT(0x2b,intb)
INTERRUPT(0x2c,intc)
INTERRUPT(0x2d,intd)
INTERRUPT(0x2e,inte)
INTERRUPT(0x2f,intf)


/* system calls */
#define SYSTEM_CALL(int,name)					\
	GATE_ENTRY(int,entry_##name,ACC_PL_U | ACC_INTR_GATE) 	;\
	.p2align 4		 	     	 		;\
entry_##name:				      	 		;\
	push	%rax						;\
	mov	$(syscall_table+8*(int-0x30)), %rax		;\
	jmp	all_syscalls

	.p2align 4
	.type	all_syscalls,@function
all_syscalls:
	SAVE_STATE
	ESP_TO_TCB_AT %rbx
	RESET_THREAD_CANCEL_AT %rbx

	call	*(%rax)
in_syscall:
ret_from_syscall:
	RESTORE_STATE
	pop	%rax
	iretq

#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
GATE_ENTRY(0x30,entry_sys_ipc,ACC_PL_U | ACC_INTR_GATE);
#else
GATE_ENTRY(0x30,entry_sys_ipc_c,ACC_PL_U | ACC_INTR_GATE);
#endif

#if defined (CONFIG_JDB_LOGGING) || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT)
        .p2align 4
	.globl	entry_sys_ipc_c
entry_sys_ipc_c:
	push	%rax
	SAVE_STATE
	ESP_TO_TCB_AT %rbx
	RESET_THREAD_CANCEL_AT %rbx


	call	ipc_short_cut_wrapper
in_sc_ipc1:
	RESTORE_STATE
	pop	%rax
	iretq

	.globl	in_sc_ipc1
#endif

#ifdef CONFIG_JDB
	/* The slow variant of sys_ipc_entry is used when logging IPC */
        .p2align 4
	.globl	entry_sys_ipc_log

entry_sys_ipc_log:
	push	%rax
	SAVE_STATE
	ESP_TO_TCB_AT %rbx
	RESET_THREAD_CANCEL_AT %rbx
	call	*syscall_table

in_slow_ipc4:
	RESTORE_STATE
	pop	%rax
	iretq

	.globl	in_slow_ipc4
#endif // CONFIG_JDB

        // these labels help show_tcb to guess the thread state
	.globl	in_syscall
	.globl	in_slowtrap
	.globl	in_page_fault
	.globl	in_handle_fputrap
	.globl	in_interrupt  
	.globl	in_timer_interrupt
	.globl	in_timer_interrupt_slow



/* SYSTEM_CALL(0x30,sys_ipc) */
SYSTEM_CALL(0x31,sys_id_nearest)
SYSTEM_CALL(0x32,sys_fpage_unmap)
SYSTEM_CALL(0x33,sys_thread_switch)
SYSTEM_CALL(0x34,sys_thread_schedule)
SYSTEM_CALL(0x35,sys_thread_ex_regs)
SYSTEM_CALL(0x36,sys_task_new)
SYSTEM_CALL(0x39,sys_u_lock)


/* these functions are implemented in entry-ia32.S */
GATE_ENTRY(0x0a,entry_vec0a_invalid_tss,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x0f,entry_vec0f_apic_spurious_interrupt_bug,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x3e,entry_vec3e_apic_error_interrupt,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x3f,entry_vec3f_apic_spurious_interrupt,ACC_PL_K | ACC_INTR_GATE)

GATE_INITTAB_END

	.p2align
	.globl	leave_by_trigger_exception
leave_by_trigger_exception:
        sub $40, %rsp
	pushq	$0x00
	pushq	$0xff
	save_all_regs
	pushq	$_slowtraps
	jmp	thread_restore_exc_state
