
#define ASSEMBLER

#include <flux/x86/paging.h>
#include <flux/x86/seg.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/asm.h>

#include "config_gdt.h"
#include "config_tcbsize.h"
#include "globalconfig.h"
#include "low_level.h"
#include "shortcut.h"
#include "tcboffset.h"


#define L4_IPC_RECANCELED		0x40
#define L4_IPC_RETIMEOUT		0x20

#ifdef CONFIG_ABI_X0
#  define RETURN_DOPE 0x6000 // three dwords
#  define TCB_ADDRESS_MASK 0x01fff800
#else
#  define RETURN_DOPE 0x4000 // two dwords
#  define TCB_ADDRESS_MASK 0x1ffff800
#endif

#define RESET_THREAD_STATE_AT(reg)		\
	andl	$~Thread_cancel, OFS__THREAD__STATE(reg)


#define REG_ECX
#define REG_EDX	(1*4)
#define REG_ESI	(2*4)
#define REG_EDI	(3*4)
#define REG_EBX	(4*4)
#define REG_EBP	(5*4)
#define REG_EAX	(6*4)

	//
	// ready_enqueue
	//
	// * precondition: edx = 0
	// * scratches: eax, ecx, edx
#define	READY_ENQUEUE(thread)					\
	movzwl	OFS__THREAD__SCHED__PRIO (thread), %eax		;\
	cmpl	%edx, CONTEXT_PRIO_FIRST (, %eax, 4)		;\
	jne	1f						;\
	movl	thread, CONTEXT_PRIO_FIRST (, %eax, 4)		;\
1:	cmpl	%edx, CONTEXT_PRIO_NEXT (, %eax, 4)		;\
	jne	1f						;\
	movl	thread, CONTEXT_PRIO_NEXT (, %eax, 4)		;\
1:	cmpl	CONTEXT_PRIO_HIGHEST, %eax			;\
	jbe	1f						;\
	movl	%eax, CONTEXT_PRIO_HIGHEST			;\
1:	/* i = (i+1) % 255 */					;\
	incl	%eax						;\
	andl	$255, %eax					;\
	/* edx = sibling = prio_first[i] */			;\
	movl	CONTEXT_PRIO_FIRST (, %eax, 4), %edx		;\
	testl	%edx, %edx					;\
	jz	1b						;\
	movl	OFS__THREAD__READY_PREV (%edx), %ecx		;\
	/* edx = sibling, ecx = sibling->ready_prev */		;\
	/* ready_next = sibling	*/				;\
	movl	%edx, OFS__THREAD__READY_NEXT (thread)		;\
	/* ready_prev = sibling->ready_prev */			;\
	movl	%ecx, OFS__THREAD__READY_PREV (thread)		;\
	/* sibling->ready_prev = this */			;\
	movl	thread, OFS__THREAD__READY_PREV (%edx)		;\
	/* sibling->ready_prev->ready_next = this */		;\
	movl	thread, OFS__THREAD__READY_NEXT (%ecx)		;\


#if defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) && !defined(CONFIG_PROFILE)

// ipc entry point for int 0x30
	.align	16
	.globl	sys_ipc_entry
sys_ipc_entry:
#define	RCV_DESC_ebp %ebp

	pushl	%eax
	SAVE_STATE
#define REGS_esp %esp

	movl	%esp,%ebx
	andl	$~(THREAD_BLOCK_SIZE -1), %ebx
#define THIS_ebx %ebx
	
	RESET_KERNEL_SEGMENTS(32(%esp))		// scratches ecx, edx
	RESET_THREAD_STATE_AT(%ebx)

	// test if long send or no send at all
#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testl	$~1, %eax
#else
	testl	$~0, %eax
#endif
	jnz	shortcut_failed

	// test if destination is L4_INVALID_ID
	cmpl	$0xffffffff, %esi
	je	shortcut_failed

	// test if destination is L4_NIL_ID
	or	%esi, %esi
	jz	shortcut_failed
	
	// test if have receive operation
	cmpl	$0xffffffff, RCV_DESC_ebp
	je	no_receive			// no

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut when we have a receive operation
	testb	$1, %al
	jnz	shortcut_failed
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut when we have a receive operation
	jmp	shortcut_failed
#endif

	// test if short receive
	testl	$~1, RCV_DESC_ebp
	jnz	shortcut_failed			// more than 2 dwords
	
	// test if simple timeout
	movl	REG_ECX (REGS_esp), %ecx
	movl	%ecx, %edi
	andl	$0x0f, %edi
	jz	1f				// rcv_to==inf => o.k.
	shrl	$24, %ecx
	jnz	shortcut_failed			// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	testl	RCV_DESC_ebp, RCV_DESC_ebp
	jz	no_receive			// closed wait
	
	movl	OFS__THREAD__IRQ (THIS_ebx), %eax
	orl	OFS__THREAD__SENDER_FIRST (THIS_ebx), %eax
	jnz	shortcut_failed
	
	.align	8
no_receive:
	addl	%esi, %esi                  
	andl	$TCB_ADDRESS_MASK, %esi	    
	orl	$_tcbs_1, %esi	// dest = dst_id.lookup

#define DEST_esi %esi
	
	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets eax to 0xffffffff and return immediatly.
	leal	OFS__THREAD__STATE (DEST_esi), %edx // addr of dest tcb state
	.global	pagein_tcb_request1
pagein_tcb_request1:
	movl	(%edx), %eax			// can raise pagefault
	cmpl	$0xffffffff, %eax
	je	shortcut_failed			// tcb is not paged

	andl	$(Thread_receiving | Thread_waiting | \
		  Thread_send_in_progress | Thread_ipc_in_progress), %eax

	// dest->tread_lock()->test()
	cmpl	$0, OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_esi)
	jne	shortcut_failed			// dest is locked

	// ipc_state == (Thread_waiting | Thread_ipc_in_progress)?
	cmpl	$(Thread_waiting | Thread_ipc_in_progress), %eax
	je	sender_ok			// open wait
	
	// receive
	// ipc_state == (Thread_receiving | Thread_ipc_in_progress)?
	cmpl	$(Thread_receiving | Thread_ipc_in_progress), %eax
	jne	shortcut_failed			// !dest->sender_ok
	
	// partner() == sender?
	leal	CAST_Thread_TO_Sender (THIS_ebx), %eax// (Sender*)this
	cmpl	%eax, OFS__THREAD__PARTNER (DEST_esi)
	je	sender_ok			// !dest->sender_ok
	
shortcut_failed:
	// shortcut failed, execute normal ipc C++ - pass
	movl	%esp,%ecx
	pushl	$0x0				// pass syscall number as arg
	pushl	%ecx				// pass pointer to regs
	pushl	%ebx				// pass "this" pointer as arg
	call	EXT(dispatch_syscall)
in_slow_ipc1:

	addl	$16, %esp			// pop args, skip ecx
	RESET_USER_SEGMENTS(28(%esp))		// scratches ecx, edx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4,%esp				// skip ebp
	popl	%eax
	iret

sender_ok:
	// copy the short msg directly into the registers of the receiver
	movl	OFS__THREAD__RECEIVE_REGS (DEST_esi), %eax
	movl	REG_EDX (REGS_esp), %edx
	movl	REG_EBX (REGS_esp), %ecx
	movl	%edx, REG_EDX (%eax)		// dest_regs->edx = edx
	movl	%ecx, REG_EBX (%eax)		// dest_regs->ebx = ebx
	movl	$RETURN_DOPE, REG_EAX (%eax)	// dest_regs->eax = DOPE(3,0)
	movl	OFS__THREAD__ID (THIS_ebx), %edx
	movl	OFS__THREAD__ID+4 (THIS_ebx), %ecx
	movl	%edx, REG_ESI (%eax)		// dest_regs->esi = id.low
	movl	%ecx, REG_EDI (%eax)		// dest_regs->edi = id.high

	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress), OFS__THREAD__STATE (DEST_esi)
	orb	$Thread_running, OFS__THREAD__STATE (DEST_esi)

	// default: no receive part => status ok
	xorl	%eax, %eax

	// prepare a receive if we have one
	cmpl	$0xffffffff, RCV_DESC_ebp
	je	do_switch_to			// no receive part

	// set_receive_regs (regs)
	movl	REGS_esp, OFS__THREAD__RECEIVE_REGS (THIS_ebx)

	// default: open wait
	movb	$(Thread_waiting | Thread_ipc_in_progress), %dl
	testl	RCV_DESC_ebp, RCV_DESC_ebp	// open wait?
	jnz	1f				// yes

	// closed wait
	movb	$(Thread_receiving | Thread_ipc_in_progress), %dl

	// set dest's partner
	leal	CAST_Thread_TO_Sender (DEST_esi), %ecx// (Sender*)dest
	movl	%ecx, OFS__THREAD__PARTNER (THIS_ebx)

1:	orb	%dl, OFS__THREAD__STATE (THIS_ebx)

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	%edi, %edi			// edi==0: timout==inf
	jne	do_switch_to			// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_running, OFS__THREAD__STATE (THIS_ebx)

	.align	8
do_switch_to:
	movl	%eax, REG_EAX (REGS_esp)

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	jne	handle_deceit
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	jmp	handle_deceit
#endif

switch_to_however:
	pushl	%ebx

	movl	OFS__THREAD__STATE (THIS_ebx), %eax
	testl	$Thread_fpu_owner, %eax
	jz	not_fpu_owner

	// set ts
	movl	%cr0, %edx
	orl	$CR0_TS, %edx
	movl	%edx, %cr0
        jmp	no_save_fpu

not_fpu_owner:

	testl	$Thread_fpu_owner, OFS__THREAD__STATE (DEST_esi)
	jz	no_save_fpu

	// clear ts
	clts	

no_save_fpu:
	xorl	%edx, %edx
	
	// if ((state() & Thread_running) && ! in_ready_list())
	//   ready_enqueue()
	cmpl	%edx, OFS__THREAD__READY_NEXT (THIS_ebx)
	jne	no_enqueue
	testb	$Thread_running, %al
	jz	no_enqueue

	READY_ENQUEUE(THIS_ebx)			// scratches eax, ecx, edx

no_enqueue:
	// push restart address onto old stack
	pushl	$ret_switch
	
	movl	%esp, OFS__THREAD__KERNEL_SP (THIS_ebx)
	movl	OFS__THREAD__KERNEL_SP (DEST_esi), %eax

	testl	%eax, %eax			// check new stack pointer
	jz	no_switch_pop			// fail
	
	movl	%eax, %esp			// load new stack pointer
	movl	DEST_esi, THIS_ebx
#undef DEST_esi

	//
	// switchin_context ()
	//

	// *(kmem::kernel_esp()) = reinterpret_cast<vm_offset_t>(regs() + 1);
	movl	KMEM_TSS, %eax
	leal	THREAD_BLOCK_SIZE (THIS_ebx), %edx
	movl	%edx, 4 (%eax)  // x86_tss.esp0

	//
	// _space_context->switchin_context ()
	//
	
	movl	OFS__THREAD__SPACE_CONTEXT (THIS_ebx), %eax
	xorl	%edx, %edx

	// edi = index = (kmem::ipc_window(0) >> PDESHIFT) & PDEMASK
	leal	((ipc_window0 >> PDESHIFT) & PDEMASK) << 2 (%eax), %edi
	leal	((ipc_window1 >> PDESHIFT) & PDEMASK) << 2 (%eax), %ebx
	movl	%edx, %esi

	// if (_dir[index] || _dir[index+1]
	movl	(%edi), %ecx
	orl	4 (%edi), %ecx
	je	1f				// no => skip
	movl	%edx, (%edi)			// _dir[index] = 0
	movl	%edx, 4(%edi)			// _dir[index+1] = 0
	incl	%esi				// need_flush_tlb = true

1:	// _dir[index] || _dir[index+1]?
	movl	(%ebx), %ecx
	orl	4 (%ebx), %ecx
	je	1f				// no => flush
	movl	%edx, (%ebx)			// _dir[index] = 0
	movl	%edx, 4(%ebx)			// _dir[index+1] = 0
	incl	%esi				// need_flush_tlb = true

1:	// pdir = space_context - kmem::mem_phys
	subl	$_physmem_1, %eax
	
	testl	%esi, %esi			// need_flush_tlb?
	jne	flush_pdir			// yes => flush
	
	movl	%cr3, %edx			// get_pdir()
	cmpl	%eax, %edx			// get_pdir == pdir
	jne	flush_pdir			// yes => no_flush
	ret

flush_pdir:
	movl	%eax, %cr3
	ret

no_switch_pop:
	addl	$4, %esp

	.align	16
ret_switch:
	popl	%ebx

	// shortcut success
	RESET_USER_SEGMENTS(32(%esp))		// scratches ecx, edx

	andl	$~Thread_ipc_mask, OFS__THREAD__STATE (THIS_ebx)

	addl	$4, %esp			// skip ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4, %esp			// skip ebp
	popl	%eax
	
	iret


#if defined(CONFIG_DECEIT_BIT_DISABLES_SWITCH) \
 || defined(CONFIG_DISABLE_AUTO_SWITCH)
	// handle deceit bit == don't switch to receiver
handle_deceit:
#define DEST_esi %esi
	// if (dest->sched()->prio > this->sched()->prio)
	//   switch_to(dest);
	movzwl	OFS__THREAD__SCHED__PRIO (THIS_ebx), %eax
	movzwl	OFS__THREAD__SCHED__PRIO (DEST_esi), %edx
	cmpl	%edx, %eax
	jb	switch_to_however

	// else
	//   dest->ready_enqueue();
	xorl	%edx, %edx

	cmpl	%edx, OFS__THREAD__READY_NEXT (DEST_esi)
	jne	de_no_enqueue
	testb	$Thread_running, OFS__THREAD__STATE (DEST_esi)

	jz	de_no_enqueue

	READY_ENQUEUE(DEST_esi)			// scratches eax, ecx, edx

de_no_enqueue:
	// shortcut success
	RESET_USER_SEGMENTS(32(%esp))		// scratches ecx, edx

	// state_change_dirty (~Thread_ipc_mask, 0)
	andl	$~Thread_ipc_mask, OFS__THREAD__STATE (THIS_ebx)

	addl	$4, %esp			// skip ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4, %esp			// skip ebp
	popl	%eax
	
	iret
#endif


// ipc entry point for sysenter
	.align	16
	.globl	do_sysenter
do_sysenter:
	pop	%esp
	pushl	$(GDT_DATA_USER|SEL_PL_U)	// user ss
	subl	$4, %esp			// room for user esp
	pushl	$0x3200				// fake user eflags
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
	pushl	$(GDT_CODE_USER|SEL_PL_U|0x80)	// user cs
	push	(%ecx)				// user eip

	pushl	%eax				// SAVE_STATE
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	12(%ecx)			// save ecx
	
	pushl   $0x00003000
	popf

	leal	16(%ecx), %edx			// adapt user esp
	movl	%edx, 40(%esp)			// set user esp
	
#define	RCV_DESC_ebp %ebp
#define REGS_esp %esp

	movl	%esp,%ebx
	andl	$~(THREAD_BLOCK_SIZE -1), %ebx	// current() => ebx
	
	RESET_KERNEL_SEGMENTS(0)  // XXX smas	// scratches ecx, edx
	RESET_THREAD_STATE_AT(%ebx)
#define THIS_ebx %ebx

	// test if long send or no send at all
#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testl	$~1, %eax
#else
	testl	$~0, %eax
#endif
	jnz	se_shortcut_failed

	// test if destination is L4_INVALID_ID
	cmpl	$0xffffffff, %esi
	je	se_shortcut_failed

	// test if destination is L4_NIL_ID
	or	%esi, %esi
	jz	se_shortcut_failed
	
	// test if have receive operation
	cmpl	$0xffffffff, RCV_DESC_ebp
	je	se_no_receive			// no

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut when we have a receive operation
	testb	$1, %al
	jnz	se_shortcut_failed
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut when we have a receive operation
	jmp	se_shortcut_failed
#endif

	// test if short receive
	testl	$~1, RCV_DESC_ebp
	jnz	se_shortcut_failed		// more than 2 dwords
	
	// test if simple timeout
	movl	REG_ECX (REGS_esp), %ecx
	movl	%ecx, %edi			// rcv_to==inf => edi=0
	andl	$0x0f, %edi
	jz	1f				// rcv_to==inf => o.k.
	shrl	$24, %ecx
	jnz	se_shortcut_failed		// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	testl	RCV_DESC_ebp, RCV_DESC_ebp
	jz	se_no_receive			// closed wait
	
	movl	OFS__THREAD__IRQ (THIS_ebx), %eax
	orl	OFS__THREAD__SENDER_FIRST (THIS_ebx), %eax
	jnz	se_shortcut_failed
	
	.align	8
se_no_receive:
	addl	%esi, %esi                  
	andl	$TCB_ADDRESS_MASK, %esi	    
	orl	$_tcbs_1, %esi	// dest = dst_id.lookup

#define DEST_esi %esi
	
	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets eax to 0xffffffff and return immediatly.
	leal	OFS__THREAD__STATE (DEST_esi), %edx // addr of dest tcb state
	.global	pagein_tcb_request2
pagein_tcb_request2:
	movl	(%edx), %eax			// can raise pagefault
	cmpl	$0xffffffff, %eax
	je	se_shortcut_failed		// tcb is not paged

	andl	$(Thread_receiving | Thread_waiting | \
		  Thread_send_in_progress | Thread_ipc_in_progress), %eax

	// dest->thread_lock()->test()
	cmpl	$0, OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_esi)
	jne	se_shortcut_failed		// dest is locked

	// ipc_state == (Thread_waiting | Thread_ipc_in_progress)?
	cmpl	$(Thread_waiting | Thread_ipc_in_progress), %eax
	je	se_sender_ok			// open wait
	
	// ipc_state == (Thread_receiving | Thread_ipc_in_progress)?
	cmpl	$(Thread_receiving | Thread_ipc_in_progress), %eax
	jne	se_shortcut_failed		// !dest->sender_ok
	
	// partner() == sender?
	leal	CAST_Thread_TO_Sender (THIS_ebx), %eax// (Sender*)this
	cmpl	%eax, OFS__THREAD__PARTNER (DEST_esi)
	je	se_sender_ok			// !dest->sender_ok

	.align	8
se_shortcut_failed:
	// shortcut failed, execute normal ipc C++ - pass
	movl	%esp,%ecx
	pushl	$0x0				// pass syscall number as arg
	pushl	%ecx				// pass pointer to regs
	pushl	%ebx				// pass pointer to "this"
	call	EXT(dispatch_syscall)
in_slow_ipc2:

	RESET_USER_SEGMENTS(0)	// XXX smas	// scratches cx,dx
	SYSEXIT_AFTER_DISPATCH_SYSCALL


se_sender_ok:
	// copy the short msg directly into the registers of the receiver
	movl	OFS__THREAD__RECEIVE_REGS (DEST_esi), %eax
	movl	REG_EDX (REGS_esp), %edx
	movl	REG_EBX (REGS_esp), %ecx
	movl	%edx, REG_EDX (%eax)		// dest_regs->edx = edx
	movl	%ecx, REG_EBX (%eax)		// dest_regs->ebx = ebx
	movl	$RETURN_DOPE, REG_EAX (%eax)	// dest_regs->eax = DOPE(3,0)
	movl	OFS__THREAD__ID (THIS_ebx), %edx
	movl	OFS__THREAD__ID+4 (THIS_ebx), %ecx
	movl	%edx, REG_ESI (%eax)		// dest_regs->esi = id.low
	movl	%ecx, REG_EDI (%eax)		// dest_regs->edi = id.high

	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress), OFS__THREAD__STATE (DEST_esi)
	orb	$Thread_running, OFS__THREAD__STATE (DEST_esi)

	// default: no receive part => status ok
	xorl	%eax, %eax

	// prepare a receive if we have one
	cmpl	$0xffffffff, RCV_DESC_ebp
	je	se_do_switch_to			// no receive part

	// set_receive_regs (regs)
	movl	REGS_esp, OFS__THREAD__RECEIVE_REGS (THIS_ebx)

	// default: open wait
	movb	$(Thread_waiting | Thread_ipc_in_progress), %dl
	testl	RCV_DESC_ebp, RCV_DESC_ebp	// open wait?
	jnz	1f				// yes

	// closed wait
	movb	$(Thread_receiving | Thread_ipc_in_progress), %dl

	// set dest's partner
	leal	CAST_Thread_TO_Sender (DEST_esi), %ecx// (Sender*)dest
	movl	%ecx, OFS__THREAD__PARTNER (THIS_ebx)

1:	orb	%dl, OFS__THREAD__STATE (THIS_ebx)

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	%edi, %edi			// edi==0: timout==inf
	jne	se_do_switch_to			// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_running, OFS__THREAD__STATE (THIS_ebx)

	.align	8
se_do_switch_to:
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result

	subl	$8, %esp
	pushl	%ebx

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	jne	se_handle_deceit
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	jmp	se_handle_deceit
#endif

se_switch_to_however:
        movl    OFS__THREAD__STATE (THIS_ebx), %eax   
        testl   $Thread_fpu_owner, %eax
	jz	se_not_fpu_owner

	// set ts
	movl	%cr0, %edx
	orl	$CR0_TS, %edx
	movl	%edx, %cr0   
	jmp	se_no_save_fpu

se_not_fpu_owner:

	testl	$Thread_fpu_owner, OFS__THREAD__STATE (DEST_esi)
	jz	se_no_save_fpu

	// clear ts
	clts       

se_no_save_fpu:
	xorl	%edx, %edx

	// if (state() & Thread_running && ! in_ready_list())
	//   ready_enqueue()
	cmpl	%edx, OFS__THREAD__READY_NEXT (THIS_ebx)
	jne	se_no_enqueue
	testb	$Thread_running, %al
	jz	se_no_enqueue

	READY_ENQUEUE(THIS_ebx)			// scratches eax, ecx, edx

se_no_enqueue:
	// push restart address onto old stack
	pushl	$se_ret_switch
	
	movl	%esp, OFS__THREAD__KERNEL_SP (THIS_ebx)
	movl	OFS__THREAD__KERNEL_SP (DEST_esi), %eax

	testl	%eax, %eax			// check new stack pointer
	jz	se_no_switch_pop		// fail
	
	movl	%eax, %esp			// load new stack pointer
	movl	DEST_esi, THIS_ebx
#undef DEST_esi

	//
	// switchin_context ()
	//

	// *(kmem::kernel_esp()) = reinterpret_cast<vm_offset_t>(regs() + 1);
	movl	KMEM_TSS, %eax
	leal	THREAD_BLOCK_SIZE (THIS_ebx), %edx
	movl	%edx, 4 (%eax)	// x86_tss.esp0

	//
	// _space_context->switchin_context ()
	//
	
	movl	OFS__THREAD__SPACE_CONTEXT (THIS_ebx), %eax
	xorl	%edx, %edx

	// edi = index = (kmem::ipc_window(0) >> PDESHIFT) & PDEMASK
	leal	((ipc_window0 >> PDESHIFT) & PDEMASK) << 2 (%eax), %edi
	leal	((ipc_window1 >> PDESHIFT) & PDEMASK) << 2 (%eax), %ebx
	movl	%edx, %esi

	// _dir[index] || _dir[index+1]?
	movl	(%edi), %ecx
	orl	4 (%edi), %ecx
	je	1f				// no => skip
	movl	%edx, (%edi)			// _dir[index] = 0
	movl	%edx, 4(%edi)			// _dir[index+1] = 0
	incl	%esi				// need_flush_tlb = true

1:	// _dir[index] || _dir[index+1]?
	movl	(%ebx), %ecx
	orl	4 (%ebx), %ecx
	je	1f				// no => flush
	movl	%edx, (%ebx)			// _dir[index] = 0
	movl	%edx, 4(%ebx)			// _dir[index+1] = 0
	incl	%esi				// need_flush_tlb = true

1:	// pdir = space_context - kmem::mem_phys
	subl	$_physmem_1, %eax
	
	testl	%esi, %esi			// need_flush_tlb?
	jne	se_flush_pdir			// yes => flush
	
	movl	%cr3, %edx			// get_pdir()
	cmpl	%eax, %edx			// get_pdir == pdir
	jne	se_flush_pdir			// yes => no_flush
	ret

se_flush_pdir:
	movl	%eax, %cr3
	ret

se_no_switch_pop:
	addl	$4, %esp

	.align	16
se_ret_switch:
	// shortcut success
	RESET_USER_SEGMENTS(0)	// XXX smas	// scratches ecx, edx
	SYSEXIT_AFTER_SHORTCUT


#if defined(CONFIG_DECEIT_BIT_DISABLES_SWITCH) \
 || defined(CONFIG_DISABLE_AUTO_SWITCH)
	// handle deceit bit == don't switch to receiver
se_handle_deceit:
#define DEST_esi %esi
	// if (dest->sched()->prio > this->sched()->prio)
	//   switch_to(dest);
	movzwl	OFS__THREAD__SCHED__PRIO (THIS_ebx), %eax
	movzwl	OFS__THREAD__SCHED__PRIO (DEST_esi), %edx
	cmpl	%edx, %eax
	jb	se_switch_to_however

	// else
	//   dest->ready_enqueue();
	xorl	%edx, %edx

	cmpl	%edx, OFS__THREAD__READY_NEXT (DEST_esi)
	jne	se_de_no_enqueue
	testb	$Thread_running, OFS__THREAD__STATE (DEST_esi)
	jz	se_de_no_enqueue

	READY_ENQUEUE(DEST_esi)			// scratches eax, ecx, edx

se_de_no_enqueue:
	// shortcut success
	RESET_USER_SEGMENTS(0)	// XXX smas	// scratches ecx, edx
	SYSEXIT_AFTER_SHORTCUT

#endif

	.globl	in_slow_ipc1
	.globl	in_slow_ipc2
	.globl	ret_switch
	.globl	se_ret_switch

#endif // CONFIG_ASSEMBLER_IPC_SHORTCUT && !CONFIG_PROFILE


#ifndef CONFIG_PROFILE
// fast return from irq_t::hit
	.align	16
	.globl	fast_ret_from_irq
fast_ret_from_irq:
	RESET_USER_SEGMENTS(32(%esp))		// scratches cx,dx
	
	addl	$4, %esp			// skip ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4, %esp			// skip ebp
	popl	%eax
	andl	$0x0000007f, 4(%esp)		// if entered by sysenter
	iret
#endif
