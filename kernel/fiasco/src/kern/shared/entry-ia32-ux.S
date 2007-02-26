/*
 * This file contains a 'gate_init' initialization table
 * to initialize the x86 processor trap vectors to default entrypoints.
 * These entrypoints simply push a standard trap_state frame
 * and jump to the 'trap_handler' routine.
 */

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

/* We make the trap handler an interrupt gate, because for debugging
   purposes, we don't want any interrupts to occur until they're
   explicitly enabled in the base_trap_handler (usually
   Thread::handle_slow_trap). */

/*
 * No error code.  Clear error code and push trap number.
 */
#define	EXCEPTION(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_INTR_GATE)		;\
	P2ALIGN(TEXT_ALIGN)		      	 		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	pusha							;\
	jmp	slowtraps

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_U | ACC_INTR_GATE)		;\
	P2ALIGN(TEXT_ALIGN)		      	 		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	pusha							;\
	jmp	slowtraps

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_INTR_GATE)		;\
	P2ALIGN(TEXT_ALIGN)		      	 		;\
name:								;\
	pushl	$(n)						;\
	pusha							;\
	jmp	slowtraps

GATE_INITTAB_BEGIN(idt_init_table)

EXCEPTION(0x00,t_zero_div)
#ifdef CONFIG_UX
EXCEPTION(0x01,t_debug)
#else
/* IA32 has to handle breakpoint exceptions if occured exactly at do_sysenter
   see ia32/entry-ia32.S */
GATE_ENTRY(0x01,debug_exception,ACC_PL_K | ACC_INTR_GATE)
#endif
/* XXX IA32 has to handle NMI occured exactly at do_sysenter */
EXCEP_USR(0x02,t_nmi)
EXCEP_USR(0x03,t_int3)
EXCEP_USR(0x04,t_into)
EXCEP_USR(0x05,t_bounds)
EXCEPTION(0x06,t_invop)
/* EXCEPTION(0x07,t_nofpu) */
#ifdef CONFIG_UX
EXCEP_ERR(0x08,a_dbl_fault)
#else
/* IA32 has a special handler defined in ia32/entry-ia32.S */
GATE_ENTRY(0x08, GDT_TSS_DBF, ACC_PL_K | ACC_TASK_GATE)
#endif
EXCEPTION(0x09,a_fpu_over)
/* EXCEP_ERR(0x0a,a_inv_tss) */
EXCEP_ERR(0x0b,t_segnp)
EXCEP_ERR(0x0c,t_stack_fault)
EXCEP_ERR(0x0d,t_gen_prot)
/* EXCEP_ERR(0x0e,t_page_fault) */
/* EXCEPTION(0x0f,t_trap_0f) */
EXCEPTION(0x10,t_fpu_err)
EXCEP_ERR(0x11,t_align)
EXCEPTION(0x12,t_trap_12)
EXCEPTION(0x13,t_trap_13)
EXCEPTION(0x14,t_trap_14)
EXCEPTION(0x15,t_trap_15)
EXCEPTION(0x16,t_trap_16)
EXCEPTION(0x17,t_trap_17)
EXCEPTION(0x18,t_trap_18)
EXCEPTION(0x19,t_trap_19)
EXCEPTION(0x1a,t_trap_1a)
EXCEPTION(0x1b,t_trap_1b)
EXCEPTION(0x1c,t_trap_1c)
EXCEPTION(0x1d,t_trap_1d)
EXCEPTION(0x1e,t_trap_1e)
EXCEPTION(0x1f,t_trap_1f)


	P2ALIGN(TEXT_ALIGN)
	FUNCSYM(slowtraps)
	.globl	slowtraps
slowtraps:

	pushl	%ds		/* we save the segment regs in the trap   */
	pushl	%es		/* state, but we do not restore them.  We */
	pushl	%fs		/* rather reload them using               */
	pushl	%gs		/* RESET_{KERNEL,USER}_SEGMENTS           */

	/* Load the kernel's segment registers.  */
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES /* scratches ecx, edx */

	/* Note: we do not use RESET_THREAD_CANCEL_* here as that is
	   needed only when an I/O-page-fault IPC is sent and when the
	   thread is killed. Resetting Thread_cancel here could
	   be harmful when using this trap handler in debugging.
	   Instead, we clear this flag in Thread::handle_slow_trap()
	   just before sending the IPC message or before killing the
	   thread.  That's OK, because it is still atomic -- we never
	   enable IRQs (sti) before that point. */

	movl	%esp,%edx		/* save address of trap_state */
#ifndef CONFIG_NO_FRAME_PTR
	movl    56(%esp),%eax		/* create artificial stack frame */
	pushl   %eax
	pushl	%ebp
# ifndef CONFIG_PROFILE
	leal	(%esp),%ebp
# else
	xorl	%ebp,%ebp
# endif
#endif

        /* Call the C handler function if one has been installed.  */
	movl	base_trap_handler,%eax
	orl	%eax,%eax
	jz	unexpected_trap
	pushl	%edx			/* push address of trap state */
	call	*%eax
in_slowtrap:

#ifndef CONFIG_NO_FRAME_PTR
	leal	12(%esp),%esp	
#else
        popl    %edx
#endif

	/* If the handler function returned zero (success),
	   then resume execution as if the trap never happened.
	   Otherwise, just panic.  */
	orl	%eax,%eax
	jnz	unexpected_trap

	RESET_USER_SEGMENTS(60(%esp))	/* scratches cx, dx */

	addl	$4*4,%esp	/* Pop segment registers from stack */
	popa
	addl	$4*2,%esp	/* Pop trap number and error code */
	iret

unexpected_trap:
	/* Dump the register state and panic.  */
	UNEXPECTED_TRAP


/* page fault */
GATE_ENTRY(0x0e,page_fault,ACC_PL_K | ACC_INTR_GATE)
	       
/* we must save %cr2 before we can be preempted -- therefore we're an
   interrupt gate (invoked with interrupts turned off).  Also, we
   don't turn them on again here, but only after checking for
   page-ins from the global page directory in thread_page_fault().
   XXX: If you make changes to stack layout here, fix thread_page_fault */

/* XXX slow version - sets up nice stack frame for debugger */

	P2ALIGN(TEXT_ALIGN)
	FUNCSYM(page_fault)
page_fault:
	pushl	%eax			/* save regs modifiable by C funcs */
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS	/* scratches cx, dx */
	movl    12(%esp),%edx		/* save error code in %edx ... */
	movl    PAGE_FAULT_ADDR,%eax	/* save page fault address in %eax */

/* We must reset the cancel flag here atomically 
   if we are entering fresh from user mode and an IPC might occur. 
   NOTE: We cannot test the user-mode bit in the error code because
   it will flag "kernel" in case an I/O-bitmap page is not mapped
   during an I/O access. */

	movl    20(%esp),%ecx   /* get CS from stack */
        andb    $3,%cl		/* retrieve current privilege level (CPL) */
	jz	1f		/* CPL == 0 -> kernel, skip resetting state */
	ESP_TO_TCB_AT(%ecx)
	RESET_THREAD_CANCEL_AT(%ecx)

1:	movl	%ebp,12(%esp)	/* save frame pointer */
	leal    12(%esp),%ebp	/* load new frame pointer */
#ifdef CONFIG_PROFILE
	call	mcount
#endif
     	pushl	%edx		/* push the error code as 2nd argument */
	pushl	%eax		/* push page fault address as 1st argument */
	pushl	%esp
	call	thread_page_fault
  
#ifdef CONFIG_SMALL_SPACES 
in_smas_trampoline_check:
	movl	28(%esp),%ecx                   /* check if page fault was in */
	andl	$0xFFFFFF00,%ecx                /* sysexit trampoline page */
	cmpl	$(0xeac00000 + 0x3000),%ecx
	jz	page_fault_in_trampoline
#endif
in_page_fault:
	orl	%eax,%eax
	jz	bad_page_fault
	addl	$12,%esp
	RESET_USER_SEGMENTS(20(%esp))  /* scratches ECX */
	popl	%edx
	popl	%ecx
	popl	%eax
	popl	%ebp
	iret

/* If code or stack from a small address space are not yet mapped
   in the current page directory we might get a page fault on return
	from the trampoline page. In this case we cannot return to the 
	trampoline page after handling the fault because we are already in
	user mode while the trampoline code is in kernel data space.
	So instead we change ESP and EIP to point to the address the
	trampoline wanted to return to and do the normal IRET.
	*/
#ifdef CONFIG_SMALL_SPACES
page_fault_in_trampoline:
	movl	40(%esp),%ecx
	movl	(%ecx),%edx
	movl	%edx,28(%esp)
	addl	$16,%ecx
	movl	%ecx,40(%esp)
	jmp	in_page_fault
#endif

/* recover from a bad page fault by invoking the slow_trap handler */
	P2ALIGN(TEXT_ALIGN)
bad_page_fault:
	cli
	addl	$4,%esp
	popl	%edx		/* page fault address */
	popl	%ecx		/* restore error code */
	movl	(%ebp),%eax	/* old %ebp */
	movl	%ecx,(%ebp)
	movl	%eax,%ebp

	/* we have on stack: error code, eax, ecx, edx
	   move registers down to make room for trap number */
	subl	$4,%esp
	movl	4(%esp),%eax
	movl	%eax,(%esp)
	movl	8(%esp),%eax
	movl	%eax,4(%esp)
	movl	12(%esp),%eax
	movl	%eax,8(%esp)

	/* push error code */
	movl	$0x0e,12(%esp)

	/* push rest of struct trap_state */
	pushl	%ebx
	pushl	%edx		/* the page fault address */
	pushl	%ebp
	pushl	%esi
	pushl	%edi

	jmp	slowtraps


/* FPU not available in this context. */
GATE_ENTRY(0x07,fpu_unavail, ACC_PL_K | ACC_INTR_GATE)

/* do all of this with disabled interrupts */
	P2ALIGN(TEXT_ALIGN)
	FUNCSYM(fpu_unavail)
fpu_unavail:
	pushl	%eax		/* save registers modifiable by C functions */
	pushl	%ecx
	pushl	%edx
	RESET_KERNEL_SEGMENTS	/* scratches cx, dx */
	call	thread_handle_fputrap
in_handle_fputrap:
	RESET_USER_SEGMENTS(16(%esp))
	popl	%edx
	popl	%ecx
	popl	%eax
	iret

/* timer interrupt */
#ifdef CONFIG_SCHED_PIT
GATE_ENTRY(0x20,timer_interrupt,ACC_PL_K | ACC_INTR_GATE)
#else
GATE_ENTRY(0x28,timer_interrupt,ACC_PL_K | ACC_INTR_GATE)
#endif

	P2ALIGN(4)
	.globl	timer_interrupt
timer_interrupt:
#ifndef CONFIG_NO_FRAME_PTR
	pushl	%ebp
#ifndef CONFIG_PROFILE
	movl	%esp,%ebp
#else
	xorl	%ebp,%ebp
#endif
#endif
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	call	thread_timer_interrupt	/* enter with disabled irqs */
in_timer_interrupt:
#ifndef CONFIG_NO_FRAME_PTR
	RESET_USER_SEGMENTS(20(%esp))	/* scratches cx,dx */
#else
	RESET_USER_SEGMENTS(16(%esp))	/* scratches cx,dx */
#endif
	popl	%ecx
	popl	%edx
	popl	%eax
#ifndef CONFIG_NO_FRAME_PTR
	popl	%ebp
#endif
	iret

	P2ALIGN(4)
	.globl	timer_interrupt_slow
timer_interrupt_slow:
#ifndef CONFIG_NO_FRAME_PTR
	pushl	%ebp
#ifndef CONFIG_PROFILE
	movl	%esp,%ebp
#else
	xorl	%ebp,%ebp
#endif
#endif
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	call	thread_timer_interrupt_slow /* enter with disabled irqs */
in_timer_interrupt_slow:
#ifndef CONFIG_NO_FRAME_PTR
	RESET_USER_SEGMENTS(20(%esp))	/* scratches cx,dx */
#else
	RESET_USER_SEGMENTS(16(%esp))	/* scratches cx,dx */
#endif
	popl	%ecx
	popl	%edx
	popl	%eax
#ifndef CONFIG_NO_FRAME_PTR
	popl	%ebp
#endif
	iret
	
	
	P2ALIGN(4)
	.globl	timer_interrupt_stop
timer_interrupt_stop:
#ifndef CONFIG_NO_FRAME_PTR
	pushl	%ebp
#ifndef CONFIG_PROFILE
	movl	%esp,%ebp
#else
	xorl	%ebp,%ebp
#endif
#endif
	pushl	%eax
	pushl	%edx
	pushl	%ecx
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	call	thread_timer_interrupt_stop /* enter with disabled irqs */
#ifndef CONFIG_NO_FRAME_PTR
	RESET_USER_SEGMENTS(20(%esp))	/* scratches cx,dx */
#else
	RESET_USER_SEGMENTS(16(%esp))	/* scratches cx,dx */
#endif
	popl	%ecx
	popl	%edx
	popl	%eax
#ifndef CONFIG_NO_FRAME_PTR
	popl	%ebp
#endif
	iret


/* profiling timer interrupt entry point */
#ifdef CONFIG_PROFILE

	P2ALIGN(4)
	.globl profile_interrupt_entry
	FUNCSYM(profile_interrupt_entry)
profile_interrupt_entry:
#ifndef CONFIG_NO_FRAME_PTR
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%eax
	movl    8(%esp),%eax	/* %eax = return address */
#else
	pushl	%eax
	movl    4(%esp),%eax	/* %eax = return address */
#endif
	pushl	%edx
	pushl	%ecx
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	pushl	%eax		/* pass return address as parameter */
	call	profile_interrupt	/* enter with disabled irqs */
	addl	$4,%esp
#ifndef CONFIG_NO_FRAME_PTR
	RESET_USER_SEGMENTS(20(%esp))	/* scratches cx,dx */
#else
	RESET_USER_SEGMENTS(16(%esp))	/* scratches cx,dx */
#endif
	popl	%ecx
	popl	%edx
	popl	%eax
#ifndef CONFIG_NO_FRAME_PTR
	popl	%ebp
#endif
	iret

#endif /* CONFIG_PROFILE */

/* other interrupts */

#define INTERRUPT(int,entrypoint)				\
	GATE_ENTRY(int,entry_##entrypoint,ACC_PL_K | ACC_INTR_GATE) ;\
	P2ALIGN(4)			      	 		;\
entry_##entrypoint:			      	 		;\
	pushl	%eax						;\
	pushl	%edx						;\
	pushl	%ecx						;\
	pushl	$ (int - 0x20)					;\
	jmp	all_irqs

	P2ALIGN(4)
	FUNCSYM(all_irqs)
all_irqs:
#ifdef CONFIG_APIC_MASK
	movl	0(%esp), %eax		// get irq number
	mov	apic_io_base, %ecx
	cmpl	apic_irq_nr, %eax
	jnz	1f

	// get current time
	movl	0x390 (%ecx), %edx
	movl	%edx, apic_timer_entry

apic_nostore:
	// save eax
	movl	%eax, %edx

	// mask all other irqs (except irq3/irq4 for COM1/COM2) to prevent
	// APIC irq service preemption
	inb	$0x21, %al
	movb	%al, apic_irq_mask
	movb	$0xe7, %al
	outb	%al, $0x21

	// restore eax
	movl	%edx, %eax
	1:
#endif /* CONFIG_APIC_MASK */
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	call	irq_interrupt	/* enter with disabled irqs */
in_interrupt:
	addl	$4,%esp
	RESET_USER_SEGMENTS(16(%esp))	/* scratches cx,dx */
	popl	%ecx
	popl	%edx
	popl	%eax
	iret

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
#ifdef CONFIG_SCHED_PIT
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

#define SYSTEM_CALL(int,entrypoint)				\
	GATE_ENTRY(int,entry_##entrypoint,ACC_PL_U | ACC_INTR_GATE) ;\
	P2ALIGN(4)		 	     	 		;\
entry_##entrypoint:			      	 		;\
	pushl	%eax						;\
	movl	$(4*(int-0x30)), %eax				;\
	jmp	all_syscalls

	P2ALIGN(4)
	FUNCSYM(all_syscalls)
all_syscalls:
	SAVE_STATE
	ESP_TO_TCB_AT(%ebx)
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	RESET_THREAD_CANCEL_AT(%ebx)
	sti
	call	*syscall_table(%eax)
	
ret_from_syscall:
	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx,dx */
	RESTORE_STATE
	popl	%eax
	iret

#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
GATE_ENTRY(0x30,sys_ipc_entry_c,ACC_PL_U | ACC_INTR_GATE);
#else
GATE_ENTRY(0x30,sys_ipc_entry,ACC_PL_U | ACC_INTR_GATE);
#endif

#if defined (CONFIG_JDB_LOGGING) \
    || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
        P2ALIGN(4)
	.globl	sys_ipc_entry_c
sys_ipc_entry_c:
	pushl	%eax
	SAVE_STATE
	ESP_TO_TCB_AT(%ebx)
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	RESET_THREAD_CANCEL_AT(%ebx)
	call	ipc_short_cut_wrapper
in_sc_ipc1:
	ESP_TO_TCB_AT(%ebx)
	RESET_THREAD_IPC_MASK_AT(%ebx)
	cmpb    $0,%al
	jz	shortcut_failed
	/* ipc_short_cut returned true -> ipc finished */
	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx,dx */
	RESTORE_STATE
	popl	%eax
	iret

shortcut_failed:
	call	sys_ipc_wrapper
in_slow_ipc3:
	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx,dx */
	RESTORE_STATE
	popl	%eax
	iret

	.globl	in_sc_ipc1
	.globl	in_slow_ipc3
#endif

#ifdef CONFIG_JDB
	/* The slow variant of sys_ipc_entry is used when logging IPC */
        P2ALIGN(4)
	.globl	sys_ipc_entry_log
sys_ipc_entry_log:
	pushl	%eax
	SAVE_STATE
	ESP_TO_TCB_AT(%ebx)
	RESET_KERNEL_SEGMENTS	/* scratches cx,dx */
	RESET_THREAD_CANCEL_AT(%ebx)
	call	*syscall_table
in_slow_ipc4:
	RESET_USER_SEGMENTS(32(%esp))	/* scratches cx,dx */
	RESTORE_STATE
	popl	%eax
	iret

	.globl	in_slow_ipc4
#endif // CONFIG_JDB

        // these labels help show_tcb to guess the thread state
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

#ifndef CONFIG_UX
/* these functions are implemented in entry-ia32.S */
GATE_ENTRY(0x0a,invalid_tss_entry,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x0f,apic_spurious_interrupt_bug_entry,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x3e,apic_error_interrupt_entry,ACC_PL_K | ACC_INTR_GATE)
GATE_ENTRY(0x3f,apic_spurious_interrupt_entry,ACC_PL_K | ACC_INTR_GATE)
#endif

GATE_INITTAB_END

	.bss
	.space	1024
	.global	dbf_stack_top
dbf_stack_top:

#ifdef CONFIG_APIC_MASK
	.bss
	.globl	apic_timer_entry
apic_timer_entry:
	.space	4
	.globl	apic_irq_mask
apic_irq_mask:
	.space	4
	.globl	apic_irq_waiting
apic_irq_waiting:
	.space	4
apic_irq_late_ptr:
	.space	4 
apic_irq_late:    
	.space	0x40

	.data
	.globl	apic_irq_nr
apic_irq_nr:
	.long	-2
#endif /* CONFIG_APIC_MASK */
