
#define ASSEMBLER

#include <flux/x86/paging.h>
#include <flux/x86/seg.h>
#include <flux/x86/asm.h>

#include "config_gdt.h"
#include "config_tcbsize.h"
#include "globalconfig.h"
#include "low_level.h"
#include "shortcut.h"
#include "tcboffset.h"
#include "regdefs.h"


#define L4_IPC_RECANCELED		0x40
#define L4_IPC_RETIMEOUT		0x20

#ifdef CONFIG_ABI_X0
#  define RETURN_DOPE 0x6000 // three dwords
#  define TCB_ADDRESS_MASK 0x01fff800
#else
#  define RETURN_DOPE 0x4000 // two dwords
#  define TCB_ADDRESS_MASK 0x1ffff800
#endif

#define REG_ECX
#define REG_EDX	(1*4)
#define REG_ESI	(2*4)
#define REG_EDI	(3*4)
#define REG_EBX	(4*4)
#define REG_EBP	(5*4)
#define REG_EAX	(6*4)
#define REG_EIP (7*4)
#define REG_CS	(8*4)
#define REG_EFL	(9*4)
#define REG_ESP	(10*4)
#define REG_SS	(11*4)

#define OFS__THREAD__SS   (THREAD_BLOCK_SIZE - 1*4)
#define OFS__THREAD__ESP  (THREAD_BLOCK_SIZE - 2*4)
#define OFS__THREAD__EFL  (THREAD_BLOCK_SIZE - 3*4)
#define OFS__THREAD__CS   (THREAD_BLOCK_SIZE - 4*4)
#define OFS__THREAD__EIP  (THREAD_BLOCK_SIZE - 5*4)

// In the SYSENTER path all kernel memory accesses go through 
// stack segment(ss).
// This way we do not need to RESET_KERNEL_SEGMENTS in smas.
// The RESET_KERNEL_SEGMENTS function is executed
// if the shortcut fails or we switch to another thread
// which is not in shortcut.

	//
	// ready_enqueue
	//
	// * precondition: edx = 0
	// * scratches: eax, ecx, edx
#define	READY_ENQUEUE(thread)					\
	movl	KSEG(OFS__THREAD__SCHED (thread)), %ecx		;\
	movzwl	KSEG(OFS__SCHED_CONTEXT__PRIO (%ecx)), %eax	;\
	cmpl	%edx, KSEG(CONTEXT_PRIO_FIRST (, %eax, 4))	;\
	jne	1f						;\
	movl	thread, KSEG(CONTEXT_PRIO_FIRST (, %eax, 4))	;\
1:	cmpl	%edx, KSEG(CONTEXT_PRIO_NEXT (, %eax, 4))	;\
	jne	1f						;\
	movl	thread, KSEG(CONTEXT_PRIO_NEXT (, %eax, 4))	;\
1:	cmpl	KSEG(CONTEXT_PRIO_HIGHEST), %eax		;\
	jbe	1f						;\
	movl	%eax, KSEG(CONTEXT_PRIO_HIGHEST)		;\
1:	/* i = (i+1) % 255 */					;\
	incl	%eax						;\
	andl	$255, %eax					;\
	/* edx = sibling = prio_first[i] */			;\
	movl	KSEG(CONTEXT_PRIO_FIRST (, %eax, 4)), %edx	;\
	testl	%edx, %edx					;\
	jz	1b						;\
	movl	KSEG(OFS__THREAD__READY_PREV (%edx)), %ecx	;\
	/* edx = sibling, ecx = sibling->ready_prev */		;\
	/* ready_next = sibling	*/				;\
	movl	%edx, KSEG(OFS__THREAD__READY_NEXT (thread))	;\
	/* ready_prev = sibling->ready_prev */			;\
	movl	%ecx, KSEG(OFS__THREAD__READY_PREV (thread))	;\
	/* sibling->ready_prev = this */			;\
	movl	thread, KSEG(OFS__THREAD__READY_PREV (%edx))	;\
	/* sibling->ready_prev->ready_next = this */		;\
	movl	thread, KSEG(OFS__THREAD__READY_NEXT (%ecx))	;\


#if defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) && !defined(CONFIG_PROFILE)

// ipc entry point for int 0x30
	.align	16
	.globl	sys_ipc_entry
sys_ipc_entry:
#define	RCV_DESC_ebp %ebp

	pushl	%eax
	SAVE_STATE
#define REGS_esp %esp

	ESP_TO_TCB_AT(%ebx)
#define THIS_ebx %ebx
	
	RESET_THREAD_CANCEL_AT(THIS_ebx)
#ifdef CONFIG_SMALL_SPACES
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES
#endif

	// test if long send or no send at all
#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testl	$~1, %eax
#else
	testl	$~0, %eax
#endif
	jnz	shortcut_failed

	// we need it later
	movl	REG_ECX (REGS_esp), %ecx

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
	orl	$_tcbs_1, %esi			// dest = dst_id.lookup

#define DEST_esi %esi
	
	leal	OFS__THREAD__STATE (DEST_esi), %ecx // addr of dest tcb state

	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets the carry flag to 1 and returns immediatly.
	andl	$0xffffffff, (%ecx)		// can raise pagefault
	jc	shortcut_failed			// tcb is not paged
	movl	(%ecx), %eax

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
	call	sys_ipc_wrapper
in_slow_ipc1:

	addl	$4, %esp			// skip ecx
	RESET_USER_SEGMENTS_FORCE_USER		// scratches ecx, edx
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
#ifdef CONFIG_ABI_X0
	movl	REG_EDI (REGS_esp), %ecx
#else
	movl	OFS__THREAD__ID+4 (THIS_ebx), %ecx
#endif
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

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	movl	%eax, REG_EAX (REGS_esp)
	jne	handle_deceit
#else
	movl	%eax, REG_EAX (REGS_esp)
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	jmp	handle_deceit
#endif

switch_to_however:

	movl	OFS__THREAD__STATE (THIS_ebx), %eax

#ifndef CONFIG_UX
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
#endif	// CONFIG_UX

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

	// *(kmem::kernel_esp()) = reinterpret_cast<Address>(regs() + 1);
	movl	KMEM_TSS, %eax
	leal	THREAD_BLOCK_SIZE (THIS_ebx), %ecx
	movl	%ecx, 4 (%eax)  // x86_tss.esp0

	//
	// _space_context->switchin_context ()
	//
	
	movl	OFS__THREAD__SPACE_CONTEXT (THIS_ebx), %eax

#ifdef CONFIG_SMALL_SPACES
#undef THIS_ebx
#undef DEST_esi
	// ebx = eax::_dir[smas_index]
	movl	((smas_area >> PDESHIFT) & PDEMASK) << 2 (%eax), %ebx
	// Kmem::set_gdt_user
	cmpl	KMEM_CURRENT_GDT, %edi
	cmpl	%edi, %ebx                     // GDT has do be changed?
	je	smas_gdt_up_to_date

	// current_user_gdt = gdtinfo;
	movl	%ebx, KMEM_CURRENT_GDT

	// Space_context::update_smas
	andl	$0xFF0000F0, %edi	       // comming from a big one?
	jnz	smas_region_update_done
	movl	%ebx, %ecx
	andl	$0xFF0000F0, %ecx
	jz	smas_region_update_done        // going to a small one?
	movl	KMEM_KDIR, %esi
	movl	((smas_version >> PDESHIFT) & PDEMASK) << 2 (%esi), %ecx
	movl	PAGE_DIR_ADDR, %edi
	addl	$_physmem_1, %edi
	cmpl	((smas_version >> PDESHIFT) & PDEMASK) << 2 (%edi), %ecx
	je	smas_region_update_done
	// _dir[smas_version] = Kmem::dir()[smas_version]
	movl	%ecx, ((smas_version >> PDESHIFT) & PDEMASK) << 2 (%edi)
	// for (...) _dir[i] = Kmem::dir()[i];
	movl	$((((smas_end - smas_start)  >> PDESHIFT) & PDEMASK) << 2), %ecx
	leal	((smas_start >> PDESHIFT) & PDEMASK) << 2 (%esi), %esi
	leal	((smas_start >> PDESHIFT) & PDEMASK) << 2 (%edi), %edi
	repz movsl

smas_region_update_done:
	movl	KMEM_GDT, %edi
	// gdtptr[gdt_code_user/4] = gdtptr[gdt_data_user/4]
	//                         = (gdtinfo & 0xF000) | 0xFFF ;
	movl	%ebx, %ecx
	andl	$0x0000F000, %ecx
	orl	$0x00000FFF, %ecx
	movl	%ecx, GDT_CODE_USER (%edi)
	movl	%ecx, GDT_DATA_USER (%edi)
	// gdtinfo = ( gdtinfo & 0xFF0F00FF) | 0xC0FB00;
	movl	%ebx, %ecx
	andl	$0xFF0F00FF, %ecx
	orl	$0x00C0FB00, %ecx
	// gdtptr[gdt_code_user/4 + 1] = gdtinfo;
	movl	%ecx, (GDT_CODE_USER + 4)  (%edi)
	// gdtinfo &= ~0x800;
	andl	$0xFFFFF7FF, %ecx
	// gdtptr[gdt_data_user/4 + 1] = gdtinfo;
	movl	%ecx, (GDT_DATA_USER + 4) (%edi)
	// set_fs( gdt_data_user | SEL_PL_U );
	movw	$ (GDT_DATA_USER|SEL_PL_U), %cx
	movl	%ecx, %fs
smas_gdt_up_to_date:
#endif

#ifndef CONFIG_UX
	// edi = index = (kmem::ipc_window(0) >> PDESHIFT) & PDEMASK
	leal	((ipc_window0 >> PDESHIFT) & PDEMASK) << 2 (%eax), %edi

	// pdir = space_context - kmem::mem_phys
	subl	$_physmem_1, %eax

	// _dir[index] || _dir[index+1] || _dir[index+2] || _dir[index+3]
	movl	(%edi), %ecx
	orl	4 (%edi), %ecx
	orl	8 (%edi), %ecx
	orl	12 (%edi), %ecx
	jne	flush_ipcwin_and_pdir		// yes => flush
#else
	// pdir = space_context - kmem::mem_phys
	subl	$_physmem_1, %eax
#endif // CONFIG_UX

#ifdef CONFIG_SMALL_SPACES
	andl	$0xFF0000F0, %ebx        	// is_small_space(_dir)?
	jnz	jump_to_thread           	// yes => no flush necessary
#endif

	movl	PAGE_DIR_ADDR, %ecx		// get_pdir()
	cmpl	%eax, %ecx			// get_pdir == pdir
	jne	flush_pdir			// no => flush
jump_to_thread:
	popl	%eax
	jmp	*%eax

#ifndef CONFIG_UX
flush_ipcwin_and_pdir:
	xorl	%ecx, %ecx
	movl	%ecx, (%edi)
	movl	%ecx, 4(%edi)
	movl	%ecx, 8(%edi)
	movl	%ecx, 12(%edi)
#endif // CONFIG_UX

#ifdef CONFIG_SMALL_SPACE
	andl	$0xFF0000F0, %ebx    		// is_small_space(_dir)?
	jz	flush_pdir
	movl	PAGE_DIR_ADDR, %eax	    // yes => reload old page directory
#endif

flush_pdir:
	movl	%eax, PAGE_DIR_ADDR
	popl	%eax
	jmp	*%eax

no_switch_pop:
	addl	$4, %esp

	.align	16
ret_switch:
	ESP_TO_TCB_AT(%ebx)
	RESET_THREAD_IPC_MASK_AT(%ebx)
	RESET_USER_SEGMENTS_FORCE_USER		// scratches ecx, edx

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
#define THIS_ebx %ebx
#define DEST_esi %esi
	// if (dest->sched()->prio > this->sched()->prio)
	//   switch_to(dest);
	movl	OFS__THREAD__SCHED (THIS_ebx), %ecx
	movzwl	OFS__SCHED_CONTEXT__PRIO (%ecx), %eax
	movl	OFS__THREAD__SCHED (DEST_esi), %ecx
	movzwl	OFS__SCHED_CONTEXT__PRIO (%ecx), %edx
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
	RESET_THREAD_IPC_MASK_AT(THIS_ebx)
	RESET_USER_SEGMENTS_FORCE_USER		// scratches ecx, edx

	addl	$4, %esp			// skip ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebx
	addl	$4, %esp			// skip ebp
	popl	%eax
	iret
#undef THIS_ebx
#undef DEST_esi

#endif


#ifndef CONFIG_UX

// IPC entry point for sysenter. 

	.align	16
	.globl	do_sysenter
do_sysenter:
	pop	%esp
	subl	$48, %esp
	movl	$(GDT_DATA_USER|SEL_PL_U), REG_SS (%esp)
	movl	$0x3200, REG_EFL (%esp)		// fake user eflags
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
	movl	$(GDT_CODE_USER|SEL_PL_U|0x80), REG_CS (%esp)
	movl	%ebx, REG_EBX (%esp)
	movl	KSEG((%ecx)), %ebx
	movl	%eax, REG_EAX (%esp)
	movl	%ebx, REG_EIP (%esp)
	movl	%ebp, REG_EBP (%esp)
	movl	%edi, REG_EDI (%esp)
	movl	KSEG(12(%ecx)), %ebx
	movl	%esi, REG_ESI (%esp)
	movl	%edx, REG_EDX (%esp)
	movl	%ebx, REG_ECX (%esp)

	ESP_TO_TCB_AT(%ebx)
#define THIS_ebx %ebx
#define	RCV_DESC_ebp	%ebp
#define REGS_esp	%esp
#define DWORD1_edx	%edx

	leal	16(%ecx), %ecx			// adapt user esp

	RESET_THREAD_CANCEL_AT(THIS_ebx)

	movl	%ecx, REG_ESP (%esp)		// set user esp

	// test if long send or no send at all
#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testl	$~1, %eax
#else
	testl	$~0, %eax
#endif
	jnz	se_shortcut_failed

	// we need it later
	movl	REG_ECX (REGS_esp), %ecx

	// test if destination is L4_INVALID_ID
	cmpl	$0xffffffff, %esi
	je	se_shortcut_failed

	// test if destination is L4_NIL_ID
	testl	%esi, %esi
	jz	se_shortcut_failed

	leal	(%esi, %esi), %esi
	andl	$TCB_ADDRESS_MASK, %esi
	orl	$_tcbs_1, %esi			// dest = dst_id.lookup
#define DEST_esi %esi

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
	movl	%ecx, %edi			// rcv_to==inf => edi=0
	andl	$0x0f, %edi
	jz	1f				// rcv_to==inf => o.k.
	testl	$0xff000000, %ecx
	jnz	se_shortcut_failed		// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	testl	RCV_DESC_ebp, RCV_DESC_ebp
	jz	se_no_receive			// closed wait

	movl	KSEG(OFS__THREAD__IRQ (THIS_ebx)), %eax
	orl	KSEG(OFS__THREAD__SENDER_FIRST (THIS_ebx)), %eax
	jnz	se_shortcut_failed
	
se_no_receive:

	leal	OFS__THREAD__STATE (DEST_esi), %ecx // addr of dest tcb state

	// clear, we need it later
	xorl	%eax, %eax

	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets the carry flag and returns immediatly.
	andl	$0xffffffff, KSEG((%ecx))		// can raise pagefault
	jc	se_shortcut_failed		// tcb is not paged
	movl	KSEG((%ecx)), %edx

	andl	$(Thread_receiving | Thread_waiting | \
		  Thread_send_in_progress | Thread_ipc_in_progress), %edx

	// dest->thread_lock()->test()
	cmpl	%eax, \
		   KSEG(OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_esi))
	jne	se_shortcut_failed		// dest is locked

	// ipc_state == (Thread_waiting | Thread_ipc_in_progress)?
	cmpl	$(Thread_waiting | Thread_ipc_in_progress), %edx
	je	se_sender_ok			// open wait

	// ipc_state == (Thread_receiving | Thread_ipc_in_progress)?
	cmpl	$(Thread_receiving | Thread_ipc_in_progress), %edx
	jne	se_shortcut_failed		// !dest->sender_ok

	// partner() == sender?
	leal	CAST_Thread_TO_Sender (THIS_ebx), %edx// (Sender*)this
	cmpl	%edx, KSEG(OFS__THREAD__PARTNER (DEST_esi))
	je	se_sender_ok			// !dest->sender_ok

	.align	8
se_shortcut_failed:
	// shortcut failed, execute normal ipc C++ - pass
#ifdef CONFIG_SMALL_SPACES
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES
#endif
	call	sys_ipc_wrapper
in_slow_ipc2:

	SYSEXIT


	.align	16
se_sender_ok:
	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress), KSEG(OFS__THREAD__STATE (DEST_esi))
	orl	$Thread_running, KSEG(OFS__THREAD__STATE (DEST_esi))

	// %eax=0 => default: no receive part => status ok

	// prepare a receive if we have one
	cmpl	$0xffffffff, RCV_DESC_ebp
	je	se_do_switch_to			// no receive part

	// set_receive_regs (regs)
	movl	REGS_esp, KSEG(OFS__THREAD__RECEIVE_REGS (THIS_ebx))

	// default: open wait
	movb	$(Thread_waiting | Thread_ipc_in_progress), %dl
	testl	RCV_DESC_ebp, RCV_DESC_ebp	// open wait?
	jnz	1f				// yes

	// closed wait
	movb	$(Thread_receiving | Thread_ipc_in_progress), %dl

	// set dest's partner
	leal	CAST_Thread_TO_Sender (DEST_esi), %ecx// (Sender*)dest
	movl	%ecx, KSEG(OFS__THREAD__PARTNER (THIS_ebx))

1:	orb	%dl, KSEG(OFS__THREAD__STATE (THIS_ebx))

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	%edi, %edi			// edi==0: timout==inf
	jne	se_do_switch_to			// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_running, KSEG(OFS__THREAD__STATE (THIS_ebx))

	.align	8
se_do_switch_to:

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
	jne	se_handle_deceit
#else
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
	jmp	se_handle_deceit
#endif

	.align	8
se_switch_to_however:
        movl    KSEG(OFS__THREAD__STATE (THIS_ebx)), %eax   
	testl	$Thread_fpu_owner, %eax
	jz	se_not_fpu_owner

	// set ts
	movl	%cr0, %edx
	orl	$CR0_TS, %edx
	movl	%edx, %cr0   
	jmp	se_no_save_fpu

se_not_fpu_owner:
	testl	$Thread_fpu_owner, KSEG(OFS__THREAD__STATE (DEST_esi))
	jz	se_no_save_fpu

	// clear ts
	clts       

se_no_save_fpu:
	// %eax=thread_state (THIS_ebx)
	xorl	%edx, %edx

	// if (state() & Thread_running && ! in_ready_list())
	//   ready_enqueue()
	cmpl	%edx, KSEG(OFS__THREAD__READY_NEXT (THIS_ebx))
	jne	se_no_enqueue
	testb	$Thread_running, %al
	jnz	se_enqueue

	.align	8
se_no_enqueue:
	// push restart address onto old stack
	pushl	$se_ret_switch
	movl	KSEG(OFS__THREAD__KERNEL_SP (DEST_esi)), %ebp
	movl	%esp, KSEG(OFS__THREAD__KERNEL_SP (THIS_ebx))
#undef REGS_esp

	movl	KSEG(KMEM_TSS), %eax

	testl	%ebp, %ebp			// check new stack pointer
	jz	se_no_switch_pop		// fail

	// *(kmem::kernel_esp()) = reinterpret_cast<Address>(regs() + 1);
	leal	THREAD_BLOCK_SIZE (DEST_esi), %ecx
	movl	%ecx, KSEG(4 (%eax))	// x86_tss.esp0

	movl	KSEG(OFS__THREAD__SPACE_CONTEXT (DEST_esi)), %eax

#ifdef CONFIG_SMALL_SPACES
	// edx = eax::_dir[smas_index]
	movl	KSEG(((smas_area >> PDESHIFT) & PDEMASK) << 2 (%eax)), %edx
	// Kmem::set_gdt_user
	movl	KSEG(KMEM_CURRENT_GDT), %edi
	cmpl	%edi, %edx  // GDT has do be changed?
	jne	se_smas_setup_space
se_smas_gdt_up_to_date:
#endif

	// edi = index = (kmem::ipc_window(0) >> PDESHIFT) & PDEMASK
	leal	((ipc_window0 >> PDESHIFT) & PDEMASK) << 2 (%eax), %edi

	// pdir = space_context - kmem::mem_phys
	subl	$_physmem_1, %eax

	// _dir[index] || _dir[index+1] || _dir[index+2] || _dir[index+3]?
	movl	KSEG((%edi)), %ecx
	orl	KSEG(4(%edi)), %ecx
	orl	KSEG(8(%edi)), %ecx
	orl	KSEG(12(%edi)), %ecx
	jne	se_flush_ipcwin_and_pdir	// yes => flush

#ifdef CONFIG_SMALL_SPACES
	andl	$0xFF0000F0, %edx		// is_small_space(_dir)?
	jnz	se_addr_space_switched		// yes => no flush necessary
#endif

	movl	PAGE_DIR_ADDR, %ecx		// get_pdir()
	cmpl	%eax, %ecx			// get_pdir == pdir
	jne	se_flush_pdir			// no => flush

se_addr_space_switched:	
	cmpl	$se_ret_switch, KSEG((%ebp))
	jne	se_slow_switch

	RESET_THREAD_IPC_MASK_AT(DEST_esi)

#ifndef CONFIG_SMALL_SPACES
	movl	KSEG(OFS__THREAD__EIP (DEST_esi)), %edx
#endif
	movl	KSEG(OFS__THREAD__ESP (DEST_esi)), %ecx
	movl	KSEG(OFS__THREAD__ID  (THIS_ebx)), %esi

	// we have to add 4 to each %esp reference since there is the
	// return address pushed on the stack
#ifdef CONFIG_ABI_X0
	movl	4+REG_ESI (%esp), %edi
#else
	movl	KSEG(OFS__THREAD__ID+4 (THIS_ebx)), %edi
#endif
	movl	4+REG_EBX (%esp), %ebx
	movl	4+REG_EDX (%esp), %ebp
	movl	$RETURN_DOPE, %eax
#ifdef CONFIG_SMALL_SPACES
	subl	$16, %ecx
	movl	$smas_trampoline, %edx
#else
	subl	$2, %edx
	sti
#endif
	sysexit

	// The destination thread is not in a shortcut IPC so we cannot
	// throw it directly into user space since it may held a thread
	// lock or does not return via sysexit (int-entered IPC or
	// ex_regs manipulation)
se_slow_switch:
	movl	KSEG(OFS__THREAD__RECEIVE_REGS (DEST_esi)), %eax
	movl	4+REG_EDX (%esp), %edx
	movl	4+REG_EBX (%esp), %ecx
	movl	%edx, KSEG(REG_EDX (%eax))		// dest_regs->edx = dw1
	movl	%ecx, KSEG(REG_EBX (%eax))		// dest_regs->ebx = dw2
	movl	KSEG(OFS__THREAD__ID (THIS_ebx)), %edx
#ifdef CONFIG_ABI_X0
	movl	4+REG_EDI (%esp), %ecx
#else
	movl	KSEG(OFS__THREAD__ID+4 (THIS_ebx)), %ecx
#endif
	movl	$RETURN_DOPE, KSEG(REG_EAX (%eax))
	movl	%edx, KSEG(REG_ESI (%eax))	    // dest_regs->esi = id.low
	movl	%ecx, KSEG(REG_EDI (%eax))	    // dest_regs->edi = id.high
	RESET_KERNEL_SEGMENTS
	movl	%ebp, %esp			    // load new stack pointer
	popl	%eax
	jmp	*%eax


se_flush_ipcwin_and_pdir:
	xorl	%ecx, %ecx
	movl	%ecx, KSEG((%edi))			// _dir[index  ] = 0
	movl	%ecx, KSEG(4(%edi))			// _dir[index+1] = 0
	movl	%ecx, KSEG(8(%edi))			// _dir[index+2] = 0
	movl	%ecx, KSEG(12(%edi))			// _dir[index+3] = 0

#ifdef CONFIG_SMALL_SPACE
	andl	$0xFF0000F0, %edx    // is_small_space(_dir)?
	jz	se_flush_pdir
	movl	PAGE_DIR_ADDR, %eax  // yes => reload old page directory
#endif

se_flush_pdir:
	movl	%eax, PAGE_DIR_ADDR		// set pdir, flush TLBs
	jmp	se_addr_space_switched


se_enqueue:
	READY_ENQUEUE(THIS_ebx)			// scratches eax, ecx, edx
	jmp	se_no_enqueue


se_no_switch_pop:
	addl	$4, %esp			// pop return value

	.align	16
se_ret_switch:
	// shortcut success
	ESP_TO_TCB_AT(%ebx)
	RESET_THREAD_IPC_MASK_AT(%ebx)
	SYSEXIT


#if defined(CONFIG_DECEIT_BIT_DISABLES_SWITCH) \
 || defined(CONFIG_DISABLE_AUTO_SWITCH)
	// handle deceit bit == don't switch to receiver
se_handle_deceit:
#define DEST_esi %esi
#define REGS_esp %esp
	// if (dest->sched()->prio > this->sched()->prio)
	//   switch_to(dest);
	movl	KSEG(OFS__THREAD__SCHED (THIS_ebx)), %ecx
	movzwl	KSEG(OFS__SCHED_CONTEXT__PRIO (%ecx)), %eax
	movl	KSEG(OFS__THREAD__SCHED (DEST_esi)), %ecx
	movzwl	KSEG(OFS__SCHED_CONTEXT__PRIO (%ecx)), %edx
	cmpl	%edx, %eax
	jb	se_switch_to_however

	// else
	//   dest->ready_enqueue();
	xorl	%edx, %edx

	cmpl	%edx, KSEG(OFS__THREAD__READY_NEXT (DEST_esi))
	jne	se_de_no_enqueue
	testb	$Thread_running, KSEG(OFS__THREAD__STATE (DEST_esi))
	jz	se_de_no_enqueue

	READY_ENQUEUE(DEST_esi)			// scratches eax, ecx, edx

se_de_no_enqueue:
	// shortcut success
	RESET_THREAD_IPC_MASK_AT(THIS_ebx)
	movl	KSEG(OFS__THREAD__RECEIVE_REGS (DEST_esi)), %eax
	movl	REG_EDX (REGS_esp), %edx
	movl	REG_EBX (REGS_esp), %ecx
	movl	%edx, KSEG(REG_EDX (%eax))
	movl	%ecx, KSEG(REG_EBX (%eax))
	movl	KSEG(OFS__THREAD__ID (THIS_ebx)), %edx
#ifdef CONFIG_ABI_X0
	movl	REG_EDI (REGS_esp), %ecx
#else
	movl	KSEG(OFS__THREAD__ID+4 (THIS_ebx)), %ecx
#endif
	movl	%edx, KSEG(REG_ESI (%eax))	   // dest_regs->esi = id.low
	movl	%ecx, KSEG(REG_EDI (%eax))	   // dest_regs->edi = id.high
#ifndef CONFIG_SMALL_SPACE
	movl	KSEG(OFS__THREAD__EIP (THIS_ebx)), %edx
#endif
	movl	KSEG(OFS__THREAD__ESP (THIS_ebx)), %ecx
	movl	$RETURN_DOPE, KSEG(REG_EAX (%eax))
	movl	KSEG(REG_EAX (REGS_esp)), %eax
#ifdef CONFIG_SMALL_SPACE
	subl	$16, %ecx
	movl	$smas_trampoline, %edx
#else
	subl	$2, %edx
	sti
#endif
	sysexit

#endif
	.globl	in_slow_ipc2
	.globl	se_ret_switch

#endif // CONFIG_UX

#ifdef CONFIG_SMALL_SPACES
se_smas_setup_space:
	// current_user_gdt = gdtinfo;
	movl	%edx, KSEG(KMEM_CURRENT_GDT)
	// Space_context::update_smas
	andl	$0xFF0000F0, %edi	       // comming from a big one?
	jnz	se_smas_region_update_done
	movl	%edx, %ecx
	andl	$0xFF0000F0, %ecx
	jz	se_smas_region_update_done        // going to a small one?
	// We run out of registers here. Therefore %edx must
	// be reloaded from memory later.
	movl	KSEG(KMEM_KDIR), %edx
	movl	KSEG(((smas_version >> PDESHIFT) & PDEMASK) << 2 (%edx)), %ecx
	movl	PAGE_DIR_ADDR, %edi
	addl	$_physmem_1, %edi
	cmpl	KSEG(((smas_version >> PDESHIFT) & PDEMASK) << 2 (%edi)), %ecx
	je	se_smas_region_current
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES   // needed for copying
	// _dir[smas_version] = Kmem::dir()[smas_version]
	movl	%ecx, ((smas_version >> PDESHIFT) & PDEMASK) << 2 (%edi)
	// for (...) _dir[i] = Kmem::dir()[i];
	movl	$((((smas_end - smas_start)  >> PDESHIFT) & PDEMASK) << 2), %ecx
	xchgl	%esi, %edx
	leal	((smas_start >> PDESHIFT) & PDEMASK) << 2 (%esi), %esi
	leal	((smas_start >> PDESHIFT) & PDEMASK) << 2 (%edi), %edi
	repz movsl
	movl	%edx, %esi
se_smas_region_current:
	movl	KSEG(KMEM_CURRENT_GDT), %edx
se_smas_region_update_done:
	movl	KSEG(KMEM_GDT), %edi
	// gdtptr[gdt_code_user/4] = gdtptr[gdt_data_user/4]
	//                         = (gdtinfo & 0xF000) | 0xFFF ;
	movl	%edx, %ecx
	andl	$0x0000F000, %ecx
	orl	$0x00000FFF, %ecx
	movl	%ecx, KSEG(GDT_CODE_USER  (%edi))
	movl	%ecx, KSEG(GDT_DATA_USER  (%edi))
	// gdtinfo = ( gdtinfo & 0xFF0F00FF) | 0xC0FB00;
	movl	%edx, %ecx
	andl	$0xFF0F00FF, %ecx
	orl	$0x00C0FB00, %ecx
	// gdtptr[gdt_code_user/4 + 1] = gdtinfo;
	movl	%ecx, KSEG((GDT_CODE_USER + 4)  (%edi))
	// gdtinfo &= ~0x800;
	andl	$0xFFFFF7FF, %ecx
	// gdtptr[gdt_data_user/4 + 1] = gdtinfo;
	movl	%ecx, KSEG((GDT_DATA_USER + 4) (%edi))
	// set_fs( gdt_data_user | SEL_PL_U );
	movw	$ (GDT_DATA_USER|SEL_PL_U), %cx
	movl	%ecx, %fs
	jmp	se_smas_gdt_up_to_date
#endif

	.globl	in_slow_ipc1
	.globl	ret_switch



#endif // CONFIG_ASSEMBLER_IPC_SHORTCUT && !CONFIG_PROFILE


#ifndef CONFIG_PROFILE
// fast return from Dirq::hit
	.align	16
	.globl	fast_ret_from_irq
fast_ret_from_irq:
	RESET_USER_SEGMENTS_FORCE_USER		// scratches cx,dx

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

