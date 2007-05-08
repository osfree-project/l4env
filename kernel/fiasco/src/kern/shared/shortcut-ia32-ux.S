
#include "config_gdt.h"
#include "config_tcbsize.h"
#include "globalconfig.h"
#include "idt_init.h"
#include "low_level.h"
#include "shortcut.h"
#include "tcboffset.h"
#include "regdefs.h"
#include "asm.h"

#define L4_IPC_RECANCELED		0x40
#define L4_IPC_RETIMEOUT		0x20

#define	PDIR_IDX(virt)			(((virt) >> 22) & 0x3ff)


#define OFS__THREAD__SS   (THREAD_BLOCK_SIZE - 1*4)
#define OFS__THREAD__ESP  (THREAD_BLOCK_SIZE - 2*4)
#define OFS__THREAD__EFL  (THREAD_BLOCK_SIZE - 3*4)
#define OFS__THREAD__CS   (THREAD_BLOCK_SIZE - 4*4)
#define OFS__THREAD__EIP  (THREAD_BLOCK_SIZE - 5*4)

// In the SYSENTER path all kernel memory accesses go through stack
// segment ss. This way we do not need to RESET_KERNEL_SEGMENTS in
// SMAS. The RESET_KERNEL_SEGMENTS function is executed if the shortcut
// fails or we switch to another thread which is not in shortcut.

	//
	// ready_enqueue
	//
	// The code outside this macro has to ensure that \thread is still
	// not queued!
	//
	// precondition  : edx = 0
	// postcondition : eax = sched()->prio
	// scratches     : ecx, edx
	.macro	READY_ENQUEUE thread, label, kseg
	//
	// ecx = sched()
	movl	\kseg OFS__THREAD__SCHED (\thread), %ecx
	// if (!sched()->left())
	//   return;
	orl	\kseg OFS__SCHED_CONTEXT__LEFT (%ecx), %edx
	orl	\kseg 4+OFS__SCHED_CONTEXT__LEFT (%ecx), %edx
	jz	\label
	// eax = sched()->prio()
	movzwl	\kseg OFS__SCHED_CONTEXT__PRIO (%ecx), %eax
	// if (prio > prio_highest)
	//   prio_highest = prio
	cmpl	\kseg CONTEXT_PRIO_HIGHEST, %eax
	jbe	1f
	movl	%eax, \kseg CONTEXT_PRIO_HIGHEST
1:	// edx = prio_next[prio]
	movl    \kseg CONTEXT_PRIO_NEXT (, %eax, 4), %edx
	// if (!prio_next[prio])
	orl	%edx, %edx
	jnz	2f
	// prio_next[prio] = this;
	movl	\thread, \kseg CONTEXT_PRIO_NEXT (, %eax, 4)
	// ready_next = this;
	movl	\thread, \kseg OFS__THREAD__READY_NEXT (\thread)
	// ready_prev = this;
	movl	\thread, \kseg OFS__THREAD__READY_PREV (\thread)
	jmp	\label
2:	// ecx = prio_next[prio]->ready_prev
	movl	\kseg OFS__THREAD__READY_PREV (%edx), %ecx
	// ready_next = prio_next[prio]
        movl	%edx, \kseg OFS__THREAD__READY_NEXT (\thread)
	// ready_prev = prio_next[prio]->ready_prev
        movl	%ecx, \kseg OFS__THREAD__READY_PREV (\thread)
	// prio_next[prio]->ready_prev = this
	movl	\thread, \kseg OFS__THREAD__READY_PREV (%edx)
	// ready_prev->ready_next = this
	movl	\thread, \kseg OFS__THREAD__READY_NEXT (%ecx)
	.endm


# define KIP_SWITCH_TIME   0xA8
# define KIP_CONSUMED_TIME 0xB8

	//
	// bookkeeping for the time a thread consumed
	//
	// precondition  : ebx = THIS
	//                 esi = DEST
	// scratches     : eax, ecx, edx, ebp
	.macro	CONSUME_TIME kseg
#ifdef CONFIG_FINE_GRAINED_CPUTIME
	rdtsc
	movl	%eax, %ecx
	movl	%edx, %ebp
	subl	\kseg CONTEXT_SWITCH_TIME, %ecx
	sbbl	\kseg CONTEXT_SWITCH_TIME+4, %ebp
	movl	%eax, \kseg CONTEXT_SWITCH_TIME
	movl	%edx, \kseg CONTEXT_SWITCH_TIME+4
	// update consumed time of previous context
	addl	%ecx, \kseg OFS__THREAD__CONSUMED_TIME (%ebx)
	adcl	%ebp, \kseg OFS__THREAD__CONSUMED_TIME+4 (%ebx)
	movl	\kseg GLOBAL_KIP, %ebp
	// store switch time to kip
	movl	%eax, KIP_SWITCH_TIME (%ebp)
	movl	%edx, KIP_SWITCH_TIME+4 (%ebp)
	// store consumed time of next thread to kip
	movl	\kseg OFS__THREAD__CONSUMED_TIME (%esi), %eax
	movl    \kseg OFS__THREAD__CONSUMED_TIME+4 (%esi), %edx
	movl    %eax, KIP_CONSUMED_TIME (%ebp)
	movl    %edx, KIP_CONSUMED_TIME+4 (%ebp)
#endif
	.endm

	//
	// change gdt entries for GDT_CODE_USER and GDT_DATA_USER
	// (only relevant if CONFIG_SMALL_SPACES is true)
	//
	// precondition  : edx = Kmem::current_user_gdt
	// postcondition :
	// scratches     : ecx, esi
	.macro	SET_GDT_USER kseg
	movl	\kseg CPU_GDT, %esi
	movl	%edx, %ecx
	andl	$0x0000FC00, %ecx
	orl	$0x000003FF, %ecx
	movl	%ecx, \kseg GDT_CODE_USER (%esi)
	movl	%ecx, \kseg GDT_DATA_USER (%esi)
	movl	%edx, %ecx
	andl	$0xFF0F00FC, %ecx
	orl	$0x00C0FB00, %ecx
	movl	%ecx, \kseg (GDT_CODE_USER + 4) (%esi)
	andl	$0xFFFFF7FF, %ecx
	movl	%ecx, \kseg (GDT_DATA_USER + 4) (%esi)
	movl	$(GDT_DATA_USER|SEL_PL_U), %ecx
	movl	%ecx, %fs
	.endm

	//
	// test if destination context can preempt the current context
	// (only relevant if CONFIG_DECEIT_BIT_DISABLES_SWITCH is true)
	//
	// precondition : ebx = This
	//                esi = Dest
	// scratches    : eax, ecx, edx
	.macro	CAN_PREEMPT_CURRENT label_true, this, dest, kseg
	movl	\kseg OFS__THREAD__SCHED (\this), %ecx      // sched()
	movzwl	\kseg OFS__SCHED_CONTEXT__PRIO (%ecx), %eax // sched()->prio
	movl	\kseg OFS__THREAD__SCHED (\dest), %ecx      // dst->sched()
	movzwl	\kseg OFS__SCHED_CONTEXT__PRIO (%ecx), %edx // dst->sched()->pr
	// if (this->sched()->prio >= dst->sched()->prio)
	//   return false;
	cmpl	%edx, %eax
	jae	1f
	movl	\kseg CONTEXT_CURRENT_SCHED, %eax	    // current_sched()
	// if (!current_sched())
	//   return true
	testl	%eax, %eax
	jz	\label_true
	// if (current_sched() == dest->sched())
	//   return true
	cmpl	%ecx, %eax
	je	\label_true
	movzwl	\kseg OFS__SCHED_CONTEXT__PRIO (%eax), %eax // curr_sched()->pr
	// if (current_sched()->prio < dest->sched()->prio)
	//   return true
	cmpl	%edx, %eax
	jb	\label_true
1:
	.endm

#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT

	// ipc entry point for int 0x30
	.align	16
	.globl	entry_sys_ipc
entry_sys_ipc:
#define	RECV_DESC_ebp %ebp

	pushl	%eax
	SAVE_STATE
#define REGS_esp %esp

	ESP_TO_TCB_AT %ebx
#define THIS_ebx %ebx

	testl   $Thread_alien, KSEG OFS__THREAD__STATE (%ebx)
	jnz     alien_sys_ipc_c

	RESET_THREAD_CANCEL_AT THIS_ebx
#ifdef CONFIG_SMALL_SPACES
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES		// scratches ecx
#endif

	// test if long send or no send at all
#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testl	$~1, %eax
#else
	testl	$~0, %eax
#endif
	jnz	i30_shortcut_failed

	// we need it later
	movl	REG_ECX (REGS_esp), %ecx

	// test if destination is L4_INVALID_ID
	cmpl	$0xffffffff, %esi
	je	i30_shortcut_failed

#ifdef CONFIG_ABI_X0
	// test if destination is L4_NIL_ID
	testl	$0x00ffffff, %esi
	jz	i30_shortcut_failed

	// test if destination has ``next_period'' bit set
	testl	$0x10000000, %esi
	jnz	i30_shortcut_failed
#else
	// test if destination is L4_NIL_ID
	or	%esi, %esi
	jz	i30_shortcut_failed

	// test if destination has ''next_period'' bit set
	testl	$0x00200000, %edi
	jnz	i30_shortcut_failed
#endif

	// test if have receive operation
	cmpl	$0xffffffff, RECV_DESC_ebp
	je	i30_test_tcb_mapped			// no

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut if we have a receive operation
	testb	$1, %al
	jnz	i30_shortcut_failed
#endif

	// test if short receive
	cmpl	$1, RECV_DESC_ebp
	ja	i30_shortcut_failed			// more than 2 dwords
	
	// test if simple timeout
	movl	%ecx, %edi
	andl	$0x0f, %edi
	jz	1f				// rcv_to==inf => o.k.
	shrl	$24, %ecx
	jnz	i30_shortcut_failed		// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	testl	RECV_DESC_ebp, RECV_DESC_ebp
	jz	i30_test_tcb_mapped
	
	movl	OFS__THREAD__SENDER_FIRST (THIS_ebx), %eax
	testl	%eax, %eax
	jnz	i30_shortcut_failed
	orl	OFS__THREAD__IRQ (THIS_ebx), %eax
	jnz	i30_shortcut_failed
	jmp	i30_test_tcb_mapped

i30_shortcut_failed:
	// shortcut failed, execute normal ipc C++ - pass
	CNT_SHORTCUT_FAILED
	call	sys_ipc_wrapper
in_slow_ipc1:
        CHECK_SANITY $3                         // scratches ecx
        RESET_USER_SEGMENTS $3,in_cli           // scratches ecx
	RESTORE_STATE_AFTER_IPC
	popl	%eax
	iret

	.align	8
i30_test_tcb_mapped:
	addl	%esi, %esi
	andl	$TCB_ADDRESS_MASK, %esi
#ifdef CONFIG_CONTEXT_4K
	addl	%esi, %esi
#endif
	orl	$VAL__MEM_LAYOUT__TCBS, %esi	// dst = dst_id.lookup

#define DEST_esi %esi
	
	leal	OFS__THREAD__STATE (DEST_esi), %ecx // addr of dst tcb state

	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets the carry flag to 1 and returns immediatly.
	// The idea behind this, is to enable interrupts early as
	// possible. The update of the pagedirectory in the pagefault
	// handler are too expensive for cli mode.
	// So the pagefault handler will enable interrupts.

	testb	$0xff, %ss:(%ecx)		// can raise pagefault
	jc	i30_shortcut_failed_1		// tcb is not paged

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
        // This is necessary to make sure the TCB is not zero,
        // Context::_sched is valid too, and 
        // CAN_PREEMPT_CURRENT dont raise an pagefault.
        // Scenario: We first touch thread 7.00 and the TCB
        // gets mapped and filled. The TCB for 7.01 is than
        // mapped AND zero, because it resides in the same page
        // as 7.00. So the assumption, that mapped TCB are always valid, is wrong.
                        
	testl	$(Thread_receiving), KSEG (%ecx)        // TCB not zero.
	jz	i30_shortcut_failed_1

	testb	$1, %al				// eax still contains the
						// send descriptor -- no recv!
	jz	1f
	// Don't take the shortcut if we have to switch_to (dst) since
	// set_current_sched() is too complicated to implement it in Assembler
	CAN_PREEMPT_CURRENT i30_shortcut_failed, THIS_ebx, DEST_esi, 0+
1:	movl	OFS__THREAD__STATE (DEST_esi), %eax
#else
	movl	(%ecx), %eax
#endif

	andl	$(Thread_receiving | Thread_send_in_progress | \
                | Thread_delayed_deadline | Thread_delayed_ipc | \
		Thread_ipc_in_progress), %eax

	movl	OFS__THREAD__PARTNER (DEST_esi), %edx

	// dst->tread_lock()->test()
	cmpl	$0, OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_esi)
	jne	i30_shortcut_failed_1			// dst is locked

	leal	CAST__Thread_TO_Sender (THIS_ebx), %ecx // (Sender*)this

	//    (ipc_state == (Thread_receiving | Thread_ipc_in_progress)
	cmpl	$(Thread_ipc_in_progress | Thread_receiving), %eax
	jne	i30_shortcut_failed_1

	// see Receiver::sender_ok
//	movl    OFS__THREAD__SENDER_FIRST (DEST_esi), %eax

	// if DEST_esi->partner() == 0, openwait
	testl	%edx, %edx
	jnz	1f

	// sender_queue empty?
//	testl	%eax, %eax
//	jnz	1f
	jmp	i30_sender_ok
1:
	// if DEST_esi->partner() == this, wait for me
	cmpl	%ecx, %edx
	jne	i30_shortcut_failed_1
	jmp	i30_sender_ok

i30_shortcut_failed_1:
	// shortcut failed, execute normal ipc C++ - pass
	jmp	i30_shortcut_failed

i30_sender_ok:
	CNT_SHORTCUT_SUCCESS

	// copy the short msg directly into the registers of the receiver
	movl	OFS__THREAD__RCV_REGS (DEST_esi), %eax
	movl	REG_EDX (REGS_esp), %edx
	movl	REG_EBX (REGS_esp), %ecx
	movl	%edx, REG_EDX (%eax)		// dst_regs->edx = edx
	movl	%ecx, REG_EBX (%eax)		// dst_regs->ebx = ebx
	movl	$RETURN_DOPE, REG_EAX (%eax)	// dst_regs->eax = DOPE(3,0)
	movl	OFS__THREAD__ID (THIS_ebx), %edx
#ifdef CONFIG_ABI_X0
	movl	REG_EDI (REGS_esp), %ecx
#else
	movl	OFS__THREAD__ID+4 (THIS_ebx), %ecx
#endif
	movl	%edx, REG_ESI (%eax)		// dst_regs->esi = id.low
	movl	%ecx, REG_EDI (%eax)		// dst_regs->edi = id.high

	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress | Thread_lipc_ready),\
		OFS__THREAD__STATE (DEST_esi)
	orb	$Thread_ready, OFS__THREAD__STATE (DEST_esi)

	// default: no receive part => status ok
	xorl	%eax, %eax

	// prepare a receive if we have one
	cmpl	$0xffffffff, RECV_DESC_ebp
	je	i30_do_switch_exec			// no receive part

	// set_rcv_regs (regs)
	movl	REGS_esp, OFS__THREAD__RCV_REGS (THIS_ebx)

        orb	$(Thread_receiving | Thread_ipc_in_progress),\
		OFS__THREAD__STATE (THIS_ebx)

        // default: open wait
	xorl	%ecx, %ecx
	
        testl   RECV_DESC_ebp, RECV_DESC_ebp      // open wait?
	jnz	1f				// openwait cmp yes

        // set dst's partner
        leal    CAST__Thread_TO_Sender (DEST_esi), %ecx // (Sender*)dst

1:	movl	%ecx, OFS__THREAD__PARTNER (THIS_ebx)

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	%edi, %edi			// edi==0: timout==inf
	jne	i30_do_switch_exec		// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_ready, OFS__THREAD__STATE (THIS_ebx)

	.align	8
i30_do_switch_exec:

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
	jnz	i30_handle_deceit
#else
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
#endif

	CNT_CONTEXT_SWITCH

#if defined(CONFIG_EXCEPTION_IPC) || defined(CONFIG_HANDLE_SEGMENTS)
	movl THIS_ebx, %eax
	movl DEST_esi, %edx
	movl $1, %ecx
	call switch_exception_context_wrapper
#endif

	movl	OFS__THREAD__STATE (THIS_ebx), %eax

#ifndef CONFIG_PF_UX
	testl	$Thread_fpu_owner, %eax
	jz	1f
	// set ts
	movl	%cr0, %edx
	orl	$CR0_TS, %edx
	movl	%edx, %cr0
        jmp	2f

1:	testl	$Thread_fpu_owner, OFS__THREAD__STATE (DEST_esi)
	jz	2f
	// clear ts
	clts	
2:
#endif	// CONFIG_PF_UX

	xorl	%edx, %edx
	
	// if ((state() & Thread_ready) && ! in_ready_list())
	//   ready_enqueue()
	cmpl	%edx, OFS__THREAD__READY_NEXT (THIS_ebx)
	jne	i30_no_enqueue
	testb	$Thread_ready, %al
	jnz	i30_enqueue_this

i30_no_enqueue:
	// not for performance kernels!
	CONSUME_TIME 0+				// scratches eax, ecx, edx, ebp

	// push restart address onto old stack
	pushl	$i30_ret_switch
	
	// context switch
	movl	%esp, OFS__THREAD__KERNEL_SP (THIS_ebx)
	movl	OFS__THREAD__KERNEL_SP (DEST_esi), %esp

	// we don't need to test for dst->kernel_sp==0 here because this
	// case is only relevant for two cases:
	// - if the thread is about to be created, the thread state is
	//   invalid until Thread::initialize()
	// - if the thread is about to be destroyed, the thread is locked
	//   and therefore we don't take the shortcut

	movl	DEST_esi, THIS_ebx
#undef DEST_esi

	// *(kmem::kernel_esp()) = reinterpret_cast<Address>(regs() + 1);
	movl	CPU_TSS, %eax
	leal	THREAD_BLOCK_SIZE (THIS_ebx), %ecx
	movl	%ecx, 4 (%eax)  // x86_tss.esp0

	// eax = this->space()
	movl	OFS__THREAD__SPACE (THIS_ebx), %eax
	leal	OFS__SPACE__MEM_SPACE (%eax), %eax
	movl	%eax, CURRENT_MEM_SPACE
	movl	OFS__MEM_SPACE__PGTABLE (%eax), %eax

#ifdef CONFIG_SMALL_SPACES
	// current_user_gdt != this->smas_area()?
	// (small ==> small, or big ==> small, or small ==> big)
	movl	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_AREA) << 2 (%eax), %edx
	movl	KMEM_CURRENT_GDT, %edi

	// here it is guaranteed that the lowest two bits of edx are zero
	// which means need_flush_iobitmap_tlb = need_flush_whole_tlb = 0

	cmpl	%edi, %edx
	jne	i30_smas_setup_space		// yes

	// big ==> big or no space switch at all
	movl	PAGE_DIR_ADDR, %ecx
	addl	PHYSMEM_OFFS, %ecx		// Space_context::current()
	cmpl	%eax, %ecx			// == this->space_context()?
	je	i30_smas_setup_space_done	// yes => no switch

	// big ==> big
	movb	$2, %dl				// need_flush_whole_tlb = 1

i30_smas_setup_space_done_to_big:
#ifdef CONFIG_IO_PROT
	movl	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_IO_BMAP_BAK) << 2 (%eax), %ecx
	movl	%ecx, PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%eax)
#endif

i30_smas_setup_space_done:
	// eax = the new big Space_context
	// edx & 1 = 1: need_flush_iobitmap_tlb = 1
	// edx & 2 = 1: need_flush_whole_tlb = 1
	// ecx, esi, edi scratched
#endif


	// pdir = space_context - kmem::mem_phys (needed later)
        subl    PHYSMEM_OFFS, %eax
        
	// eax = page table register of new big Space_context

#ifdef CONFIG_SMALL_SPACES
	testb	$2, %dl				// need_flush_whole_tlb?
	jnz	i30_flush_pdir			// yes => load pdir
	testb	$1, %dl				// need_flush_iobitmap_tlb?
	jnz	i30_flush_iobitmap		// yes => flush IO bitmap
#else
	movl	PAGE_DIR_ADDR, %ecx		// Space_context::current
	cmpl	%eax, %ecx			// == the new Space_context?
	jne	i30_flush_pdir			// no => flush
#endif
i30_jump_to_thread:
	popl	%eax
	jmp	*%eax

#ifdef CONFIG_SMALL_SPACES
i30_flush_iobitmap:
	CNT_IOBMAP_TLB_FLUSH
	invlpg	VAL__MEM_LAYOUT__IO_BITMAP
	invlpg	VAL__MEM_LAYOUT__IO_BITMAP + 0x1000
	popl	%eax
	jmp	*%eax
#endif


i30_flush_pdir:
	CNT_ADDR_SPACE_SWITCH
	movl	%eax, PAGE_DIR_ADDR
	popl	%eax
	jmp	*%eax

i30_ret_switch:
	ESP_TO_TCB_AT %ebx
	RESET_THREAD_IPC_MASK_AT %ebx
	CHECK_SANITY $3				// scratches ecx
	RESET_USER_SEGMENTS $3,in_cli		// scratches ecx
	RESTORE_STATE_AFTER_IPC
	popl	%eax
	iret

i30_enqueue_this:
#define THIS_ebx %ebx
	// ((state() & Thread_ready) && !in_ready_list()) is true here!
	READY_ENQUEUE THIS_ebx, i30_no_enqueue, 0+ // scratches ecx, edx
						   // eax = sched()->prio
	// if (_prio_next[prio] != 0)   # see READY_ENQUEUE macro
	//   if (this == current())
	//     _prio_next[prio] = this
	movl	THIS_ebx, CONTEXT_PRIO_NEXT (, %eax, 4)
	jmp	i30_no_enqueue


#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// Handle deceit bit == don't switch to receiver. As we got here there
	// is no receive operation since it would not make sense to handle that
	// case in shortcut. If we _have_ a receive phase that would induce a
	// normal schedule() after the send phase.
	// Here we are sure that we don't need to switch to the destination
	// (in this case we wouldn't take the shortcut).
i30_handle_deceit:
#define THIS_ebx %ebx
#define DEST_esi %esi

#if defined(CONFIG_EXCEPTION_IPC) || defined(CONFIG_HANDLE_SEGMENTS)
	movl THIS_ebx, %eax
	movl DEST_esi, %edx
	movl $0, %ecx
	call switch_exception_context_wrapper
#endif
	
	//   dst->ready_enqueue();
	xorl	%edx, %edx

	// (dst->state() & Thread_ready) is true here!
	cmpl	%edx, OFS__THREAD__READY_NEXT (DEST_esi)
	je	i30_de_enqueue

i30_de_no_enqueue:
	// short IPC send shortcut success
	RESET_THREAD_IPC_MASK_AT THIS_ebx
	CHECK_SANITY $3				// scratches ecx
	RESET_USER_SEGMENTS $3,in_cli		// scratches ecx
	addl	$28, %esp			// skip all registers
	subl	%eax, %eax			// status ok
	iret

i30_de_enqueue:
	// The deceiting bit was set which means that we don't switch to
	// the receiver of the IPC. To ensure that we don't forget the
	// IPC partner we have to enqueue it into the ready queue.
	READY_ENQUEUE DEST_esi, i30_de_no_enqueue, 0+ // scratches eax,ecx,edx
	jmp	i30_de_no_enqueue
#undef THIS_ebx
#undef DEST_esi

#endif

#ifdef CONFIG_SMALL_SPACES
	// current_user_gdt != this->small_space_area()
	// (small ==> small, or big ==> small, or small ==> big)
	//
	// precondition  : eax = this->space_context()
	//                 edx = this->small_space_area()
	//                 edi = current_user_gdt
	// postcondition : edx & 1 = 1: need_flush_iobitmap_tlb = true
	//                 edx & 2 = 1: need_flush_whole_tlb = true
	.align	16
i30_smas_setup_space:
	SET_GDT_USER 0+				// scratches ecx, esi
	movl	%edx, KMEM_CURRENT_GDT
	movl	PAGE_DIR_ADDR, %esi
	addl	PHYSMEM_OFFS, %esi		// Space_context::current
	testl	$0xFF0000FC, %edx		// small ==> big?
	jnz	1f				// no

	// small ==> big
	cmpl	%eax, %esi			// == this->space_context()?
	setne	%dl
	shlb	$1, %dl
	jmp	i30_smas_setup_space_done_to_big

1:	// small ==> small or big ==> small
#ifdef CONFIG_IO_PROT
	movl	PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%esi), %ecx
	andl	$1, %ecx
	orl	%ecx, %edx			// need_flush_iobitmap_tlb = 1
	movl	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_IO_BMAP_BAK) << 2 (%eax), %ecx
	movl	%ecx, PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%esi)
#endif
	movl	%esi, %eax

	testl	$0xFF0000FC, %edi		// small ==> small?
	jnz	1f				// yes => no smas_update

	// big ==> small: Copy complete SMAS area from Kmem::dir() to
	// current_space::dir().
	movl	%esi, %edi
	movl	KMEM_KDIR, %esi
	movl	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%esi), %ecx
	cmpl	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%edi), %ecx
	jne	2f
1:	jmp	i30_smas_setup_space_done

	.align	8
2:	// Only occurs in very rare cases. Perhaps we should fail the
	// shortcut if we would call this code.
	movl	%ecx, PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%edi)
	int	$3
	leal	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_START) << 2 (%esi), %esi
	leal	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_START) << 2 (%edi), %edi
	movl	$ (PDIR_IDX(VAL__MEM_LAYOUT__SMAS_END - \
			    VAL__MEM_LAYOUT__SMAS_START) - 4), %ecx
3:	movl	(%esi, %ecx, 4), %edx
	movl	%edx, (%edi, %ecx, 4)
	decl	%ecx
	jns	3b

	movb	$2, %dl				// need_flush_whole_tlb = 1
	jmp	i30_smas_setup_space_done
#endif

#ifndef CONFIG_PF_UX

// IPC entry point for sysenter. 

	.align	16
	.globl	entry_sys_fast_ipc
entry_sys_fast_ipc:
	pop	%esp
	subl	$48, %esp
	movl	$(GDT_DATA_USER|SEL_PL_U), REG_SS (%esp)

#ifdef CONFIG_IO_PROT
	// We must not fake the interrupt flags since we must not
	// loose the current IOPL of the user task
	pushf
	pop	REG_EFL (%esp)
#else
	// Fake user eflags, set IOPL to 3
	movl	$EFLAGS_IOPL_U, REG_EFL (%esp)
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
	// no sysexit if Small Spaces and I/O protection is enabled!
	movl	$(GDT_CODE_USER|SEL_PL_U), REG_CS (%esp)
#else
	movl	$(GDT_CODE_USER|SEL_PL_U|0x80), REG_CS (%esp)
#endif
	movl	%ebx, REG_EBX (%esp)
	movl	(%ecx), %ebx
	movl	%eax, REG_EAX (%esp)
	movl	%ebx, REG_EIP (%esp)
	movl	%ebp, REG_EBP (%esp)
	movl	%edi, REG_EDI (%esp)
	movl	12(%ecx), %ebx
	movl	%esi, REG_ESI (%esp)
	movl	%edx, REG_EDX (%esp)
	movl	%ebx, REG_ECX (%esp)

	ESP_TO_TCB_AT %ebx
#define THIS_ebx %ebx
#define	RECV_DESC_ebp	%ebp
#define REGS_esp	%esp
#define DWORD1_edx	%edx

	leal	16(%ecx), %ecx			// adapt user esp

	movl	%ecx, REG_ESP (%esp)		// set user esp

	testl   $Thread_alien, KSEG OFS__THREAD__STATE (%ebx)
	jnz     alien_sys_fast_ipc_c

	RESET_THREAD_CANCEL_AT THIS_ebx

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

#ifdef CONFIG_ABI_X0
	// test if destination is L4_NIL_ID
	testl	$0x00ffffff, %esi
	jz	se_shortcut_failed
	// test if destination has ``next_period'' bit set
	testl	$0x10000000, %esi
	jnz	se_shortcut_failed
#else
	// test if destination is L4_NIL_ID
	testl	%esi, %esi
	jz	se_shortcut_failed
	// test if destination has ``next_period'' bit set
	testl	$0x002000000, %edi
	jnz	se_shortcut_failed
#endif

	leal	(%esi, %esi), %esi
#ifdef CONFIG_CONTEXT_4K
	addl	%esi, %esi
#endif
	andl	$TCB_ADDRESS_MASK, %esi
	orl	$VAL__MEM_LAYOUT__TCBS, %esi	// dst = dst_id.lookup
#define DEST_esi %esi

	// test if have receive operation
	cmpl	$0xffffffff, RECV_DESC_ebp
	je	se_test_tcb_mapped		// no

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// don't automatically switch to receiver requested, not possible
	// to handle this in shortcut if we have a receive operation
	testb	$1, %al
	jnz	se_shortcut_failed
#endif

	// test if short receive
	cmpl	$1, RECV_DESC_ebp
	ja	se_shortcut_failed		// more than 2 dwords
	
	// test if simple timeout
	movl	%ecx, %edi			// rcv_to==inf => edi=0
	andl	$0x0f, %edi
	jz	1f				// rcv_to==inf => o.k.
	testl	$0xff000000, %ecx
	jnz	se_shortcut_failed		// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	testl	RECV_DESC_ebp, RECV_DESC_ebp
	jz	se_test_tcb_mapped		// closed wait

	movl	KSEG OFS__THREAD__SENDER_FIRST (THIS_ebx), %eax
	testl	%eax, %eax
	jnz	se_shortcut_failed
	orl	KSEG OFS__THREAD__IRQ (THIS_ebx), %eax
	jnz	se_shortcut_failed
	jmp	se_test_tcb_mapped

	.align	8
se_shortcut_failed:
	// shortcut failed, execute normal ipc C++ pass
#ifdef CONFIG_SMALL_SPACES
	RESET_KERNEL_SEGMENTS_FORCE_DS_ES	// scratches ecx
#endif
	CNT_SHORTCUT_FAILED
	call	sys_ipc_wrapper
in_slow_ipc2:

	DO_SYSEXIT

//	.align	16
se_test_tcb_mapped:
	leal	OFS__THREAD__STATE (DEST_esi), %ecx // addr of dst tcb state

	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets the carry flag and returns immediatly.
	testb	$0xff, %ss:(%ecx)		// can raise pagefault
	jc	se_shortcut_failed_1		// tcb is not paged

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
        // This is necessary to make sure the TCB is not zero,
        // Context::_sched is valid too, and 
        // CAN_PREEMPT_CURRENT dont raise an pagefault.
        // Scenario: We first touch thread 7.00 and the TCB
        // gets mapped and filled. The TCB for 7.01 is than
        // mapped AND zero, because it resides in the same page
        // as 7.00. So the assumption, that mapped TCB are always valid, is wrong.
        
	testl	$(Thread_receiving), KSEG (%ecx)
	jz	se_shortcut_failed_1            // TC not zero

	testb	$1, %al				// eax still contains the
						// send descriptor -- no recv!
	jz	1f
	// Don't take the shortcut if we have to switch_to (dst) since
	// set_current_sched() is too complicated to implement it in Assembler
	CAN_PREEMPT_CURRENT se_shortcut_failed, THIS_ebx, DEST_esi, KSEG 0+
1:	movl	KSEG OFS__THREAD__STATE (DEST_esi), %eax
#else
	movl	KSEG (%ecx), %eax
#endif

	andl	$(Thread_receiving | Thread_send_in_progress | \
		Thread_ipc_in_progress), %eax

	movl	KSEG OFS__THREAD__PARTNER (DEST_esi), %edx
	
	// dst->thread_lock()->test()
	cmpl	$0, \
		KSEG OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_esi)
	jne	se_shortcut_failed_1		// dst is locked

	leal	CAST__Thread_TO_Sender (THIS_ebx), %ecx // (Sender*)this

	//    (ipc_state == (Thread_receiving | Thread_ipc_in_progress)
	cmpb	$(Thread_ipc_in_progress | Thread_receiving), %al
	jne	se_shortcut_failed_1

	// see Receiver::sender_ok
//	movl    KSEG OFS__THREAD__SENDER_FIRST (DEST_esi), %eax

	// if DEST_esi->partner() == 0, openwait
	testl	%edx, %edx
	jne	1f

	// sender_queue empty?
//	testl	%eax, %eax
//	jnz	1f
	jmp	se_sender_ok

1:	// if DEST_esi->partner() == this, wait for me
	cmpl	%ecx, %edx
	jne	se_shortcut_failed_1
	jmp	se_sender_ok

se_shortcut_failed_1:
	jmp	se_shortcut_failed


	.align	16
se_sender_ok:
	CNT_SHORTCUT_SUCCESS

	// clear, we need it later
	xorl	%eax, %eax

	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress), KSEG OFS__THREAD__STATE (DEST_esi)
	orb	$Thread_ready, KSEG OFS__THREAD__STATE (DEST_esi)

	// %eax=0 => default: no receive part => status ok

	// prepare a receive if we have one
	cmpl	$0xffffffff, RECV_DESC_ebp
	je	se_do_switch_exec		// no receive part

	// set_rcv_regs (regs)
	movl	REGS_esp, KSEG OFS__THREAD__RCV_REGS (THIS_ebx)

        orb	$(Thread_receiving | Thread_ipc_in_progress),\
		 KSEG OFS__THREAD__STATE (THIS_ebx)

        // default: open wait
	xorl	%ecx, %ecx

        testl   RECV_DESC_ebp, RECV_DESC_ebp    // open wait?
	jnz	1f				// openwait cmp yes

        // set dst's partner
	leal	CAST__Thread_TO_Sender (DEST_esi), %ecx // (Sender*)dst

1:	movl	%ecx, KSEG OFS__THREAD__PARTNER (THIS_ebx)

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	%edi, %edi			// edi==0: timout==inf
	jne	se_do_switch_exec		// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_ready, KSEG OFS__THREAD__STATE (THIS_ebx)

	.align	8
se_do_switch_exec:

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	testb	$1, REG_EAX (REGS_esp)
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
	jnz	se_handle_deceit
#else
	movl	%eax, REG_EAX (REGS_esp)	// store ipc result
#endif

	CNT_CONTEXT_SWITCH

#if defined(CONFIG_EXCEPTION_IPC) || defined(CONFIG_HANDLE_SEGMENTS)
	movl THIS_ebx, %eax
	movl DEST_esi, %edx
	movl $1, %ecx
	call switch_exception_context_wrapper
#endif
	
        movl    KSEG OFS__THREAD__STATE (THIS_ebx), %eax
	testl	$Thread_fpu_owner, %eax
	jz	1f
	// set ts
	movl	%cr0, %edx
	orl	$CR0_TS, %edx
	movl	%edx, %cr0   
	jmp	2f

1:	testl	$Thread_fpu_owner, KSEG OFS__THREAD__STATE (DEST_esi)
	jz	2f
	// clear ts
	clts       

2:	// %eax=thread_state (THIS_ebx)
	xorl	%edx, %edx

	// if (state() & Thread_ready && ! in_ready_list())
	//   ready_enqueue()
	cmpl	%edx, KSEG OFS__THREAD__READY_NEXT (THIS_ebx)
	jne	se_no_enqueue
	testb	$Thread_ready, %al
	jnz	se_enqueue_this

	.align	8
se_no_enqueue:
	// not for performance kernels!
	CONSUME_TIME KSEG 0+			// scratches eax, ecx, edx, ebp

	// push restart address onto old stack
	pushl	$se_ret_switch
	movl	%esp, KSEG OFS__THREAD__KERNEL_SP (THIS_ebx)
#undef REGS_esp
	movl	KSEG OFS__THREAD__KERNEL_SP (DEST_esi), %ebp

	// we don't need to test for dst->kernel_sp==0 here because this
	// case is only relevant for two cases:
	// - if the thread is about to be created, the thread state is
	//   invalid until Thread::initialize()
	// - if the thread is about to be destroyed, the thread is locked
	//   and therefore we don't take the shortcut

	// switch esp0 on TSS
	movl	KSEG CPU_TSS, %eax
	leal	THREAD_BLOCK_SIZE (DEST_esi), %ecx
	movl	%ecx, KSEG 4 (%eax)	// x86_tss.esp0

	movl	KSEG OFS__THREAD__SPACE (DEST_esi), %eax
	leal	OFS__SPACE__MEM_SPACE (%eax), %eax
	movl	%eax, CURRENT_MEM_SPACE
	movl	OFS__MEM_SPACE__PGTABLE (%eax), %eax

#ifdef CONFIG_SMALL_SPACES
	// current_user_gdt != this->smas_area()?
	// (small ==> small, or big ==> small, or small ==> big)
	movl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_AREA) << 2 (%eax), %edx
	movl	KSEG KMEM_CURRENT_GDT, %edi

	// here it is guaranteed that the lowest two bits of edx are zero
	// which means need_flush_iobitmap_tlb = need_flush_whole_tlb = 0

	cmpl	%edi, %edx
	jne	se_smas_setup_space

	// big ==> big or no space switch at all
	movl	PAGE_DIR_ADDR, %ecx
	addl	KSEG PHYSMEM_OFFS, %ecx		// Space_context::current()
	cmpl	%eax, %ecx			// == this->space_context()?
	je	se_smas_setup_space_done	// yes => no switch

	// big ==> big
	movb	$2, %dl				// need_flush_whole_tlb = 1

se_smas_setup_space_done_to_big:
#ifdef CONFIG_IO_PROT
	movl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_IO_BMAP_BAK) << 2 (%eax),\
		%ecx
	movl	%ecx, KSEG PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%eax)
#endif

se_smas_setup_space_done:
	// eax = the new big Space context
	// edx & 1 = 1: need_flush_iobitmap_tlb = 1
	// edx & 2 = 1: need_flush_whole_tlb = 1
	// ecx, edi scratched
#endif

	// pdir = space_context - kmem::mem_phys (needed later)
        subl    KSEG PHYSMEM_OFFS, %eax
         

#ifdef CONFIG_SMALL_SPACES
	testb	$2, %dl				// need_flush_whole_tlb?
	jnz	se_flush_pdir			// yes => load pdir
	testb	$1, %dl				// need_flush_iobitmap_tlb?
	jnz	se_flush_iobitmap		// yes => flush IO bitmap
#else
	movl	PAGE_DIR_ADDR, %ecx		// get_pdir()
	cmpl	%eax, %ecx			// get_pdir == pdir
	jne	se_flush_pdir			// no => flush
#endif

se_addr_space_switched:	
	cmpl	$se_ret_switch, (%ebp)
	jne	se_slow_switch

	RESET_THREAD_IPC_MASK_AT DEST_esi

	// Setup return registers. We have to add 4 to each %esp reference
	// since there is the return address pushed on the stack.

#if defined(CONFIG_SMALL_SPACES) && defined(CONFIG_IO_PROT)
	movl	KSEG OFS__THREAD__ID  (THIS_ebx), %esi
#ifdef CONFIG_ABI_X0
	movl	4+REG_ESI (%esp), %edi
#else
	movl	KSEG OFS__THREAD__ID+4 (THIS_ebx), %edi
#endif
	movl	4+REG_EBX (%esp), %ebx
	movl	4+REG_EDX (%esp), %edx
	leal	32(%ebp), %esp
	orl	$EFLAGS_IF, 8(%esp)
	movl	$RETURN_DOPE, %eax
	iret
#endif

#if defined(CONFIG_SMALL_SPACES) && !defined(CONFIG_IO_PROT)
	movl	KSEG OFS__THREAD__ESP (DEST_esi), %ecx
	subl	$16, %ecx
	movl	KSEG OFS__THREAD__ID  (THIS_ebx), %esi
#ifdef CONFIG_ABI_X0
	movl	4+REG_ESI (%esp), %edi
#else
	movl	KSEG OFS__THREAD__ID+4 (THIS_ebx), %edi
#endif
	movl	4+REG_EBX (%esp), %ebx
	movl	4+REG_EDX (%esp), %ebp
	movl	$VAL__MEM_LAYOUT__SMAS_TRAMPOLINE, %edx
	movl	$RETURN_DOPE, %eax
	sysexit
#endif

#if !defined(CONFIG_SMALL_SPACES)
#ifdef CONFIG_IO_PROT
	pushl	KSEG OFS__THREAD__EFL (DEST_esi)	// restore user IOPL
	popf
#endif
	movl	KSEG OFS__THREAD__EIP (DEST_esi), %edx
	subl	$2, %edx
	movl	KSEG OFS__THREAD__ESP (DEST_esi), %ecx
	movl	KSEG OFS__THREAD__ID  (THIS_ebx), %esi
#ifdef CONFIG_ABI_X0
	movl	4+REG_ESI (%esp), %edi
#else
	movl	KSEG OFS__THREAD__ID+4 (THIS_ebx), %edi
#endif
	movl	4+REG_EBX (%esp), %ebx
	movl	4+REG_EDX (%esp), %ebp
	movl	$RETURN_DOPE, %eax
	sti
	sysexit
#endif


#ifdef CONFIG_SMALL_SPACES
se_flush_iobitmap:
	CNT_IOBMAP_TLB_FLUSH
	invlpg	VAL__MEM_LAYOUT__IO_BITMAP
	invlpg	VAL__MEM_LAYOUT__IO_BITMAP + 0x1000
	jmp	se_addr_space_switched
#endif


se_flush_pdir:
	CNT_ADDR_SPACE_SWITCH
	movl	%eax, PAGE_DIR_ADDR			// set pdir, flush TLBs
	jmp	se_addr_space_switched

se_enqueue_this:
	// ((state() & Thread_ready) && !in_ready_list()) is true here!
	READY_ENQUEUE THIS_ebx, se_no_enqueue, KSEG 0+ // scratches ecx, edx
						       // eax = sched()->prio()
	// if (_prio_next[prio] != 0)   # see READY_ENQUEUE macro
	//   if (this == current())
	//     _prio_next[prio] = this
	movl	THIS_ebx, KSEG CONTEXT_PRIO_NEXT (, %eax, 4)
	jmp	se_no_enqueue

	.align	16
se_ret_switch:
	// shortcut success
	ESP_TO_TCB_AT %ebx
	RESET_THREAD_IPC_MASK_AT %ebx
	DO_SYSEXIT

	// The destination thread is not in a shortcut IPC so we cannot
	// throw it directly into user space since it may held a thread
	// lock or does not return via sysexit (int-entered IPC or
	// ex_regs manipulation)
	.align	16
se_slow_switch:
	movl	KSEG OFS__THREAD__RCV_REGS (DEST_esi), %eax
	movl	4+REG_EDX (%esp), %edx
	movl	4+REG_EBX (%esp), %ecx
	movl	%edx, KSEG REG_EDX (%eax)		// dst_regs->edx = dw1
	movl	%ecx, KSEG REG_EBX (%eax)		// dst_regs->ebx = dw2
	movl	KSEG OFS__THREAD__ID (THIS_ebx), %edx
#ifdef CONFIG_ABI_X0
	movl	4+REG_EDI (%esp), %ecx
#else
	movl	KSEG OFS__THREAD__ID+4 (THIS_ebx), %ecx
#endif
	movl	$RETURN_DOPE, KSEG REG_EAX (%eax)
	movl	%edx, KSEG REG_ESI (%eax)	    // dst_regs->esi = id.low
	movl	%ecx, KSEG REG_EDI (%eax)	    // dst_regs->edi = id.high
	RESET_KERNEL_SEGMENTS
	movl	%ebp, %esp			    // load new stack pointer
	popl	%eax
	jmp	*%eax


#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
	// Handle deceit bit == don't switch to receiver. As we got here there
	// is no receive operation since it would not make sense to handle that
	// case in shortcut. If we _have_ a receive phase that would induce a
	// normal schedule() after the send phase.
	// Here we are sure that we don't need to switch to the destination
	// (in this case we wouldn't take the shortcut).
se_handle_deceit:
#define DEST_esi %esi
#define REGS_esp %esp

#if defined(CONFIG_EXCEPTION_IPC) || defined(CONFIG_HANDLE_SEGMENTS)
	movl THIS_ebx, %eax
	movl DEST_esi, %edx
	movl $0, %ecx
	call switch_exception_context_wrapper
#endif

	//   dst->ready_enqueue();
	xorl	%edx, %edx

	// (dst->state() & Thread_ready) is true here!
	cmpl	%edx, KSEG OFS__THREAD__READY_NEXT (DEST_esi)
	je	se_de_enqueue

se_de_no_enqueue:
	// shortcut success
	RESET_THREAD_IPC_MASK_AT THIS_ebx
	movl	KSEG OFS__THREAD__RCV_REGS (DEST_esi), %eax
	movl	REG_EDX (REGS_esp), %edx
	movl	REG_EBX (REGS_esp), %ecx
	movl	%edx, KSEG REG_EDX (%eax)
	movl	%ecx, KSEG REG_EBX (%eax)
	movl	KSEG OFS__THREAD__ID (THIS_ebx), %edx
#ifdef CONFIG_ABI_X0
	movl	REG_EDI (REGS_esp), %ecx
#else
	movl	KSEG OFS__THREAD__ID+4 (THIS_ebx), %ecx
#endif
	movl	%edx, KSEG REG_ESI (%eax)	   // dst_regs->esi = id.low
	movl	%ecx, KSEG REG_EDI (%eax)	   // dst_regs->edi = id.high
	movl	$RETURN_DOPE, KSEG REG_EAX (%eax)

#if defined(CONFIG_SMALL_SPACES) && defined(CONFIG_IO_PROT)
	orl	$EFLAGS_IF, 36(%esp)
	addl	$28, %esp
	subl	%eax, %eax
	iret
#endif

#if defined(CONFIG_SMALL_SPACES) && !defined(CONFIG_IO_PROT)
	movl	KSEG OFS__THREAD__ESP (THIS_ebx), %ecx
	subl	$16, %ecx
	movl	$VAL__MEM_LAYOUT__SMAS_TRAMPOLINE, %edx
	subl	%eax, %eax
	// don't need to restore the user's IOPL since it's still valid
	sysexit
#endif

#if !defined(CONFIG_SMALL_SPACES)
	movl	KSEG OFS__THREAD__EIP (THIS_ebx), %edx
	subl	$2, %edx
	movl	KSEG OFS__THREAD__ESP (THIS_ebx), %ecx
	subl	%eax, %eax
	sti
	// don't need to restore the user's IOPL since it's still valid
	sysexit
#endif


se_de_enqueue:
	// The deceiting bit was set which means that we don't switch to
	// the receiver of the IPC. To ensure that we don't forget the
	// IPC partner we have to enqueue it into the ready queue.
	READY_ENQUEUE DEST_esi, se_de_no_enqueue, KSEG 0+// scratches ecx, edx
	jmp	se_de_no_enqueue
#endif // CONFIG_DECEIT_BIT_DISABLES_SWITCH

	.globl	in_slow_ipc2
	.globl	se_ret_switch

#endif // CONFIG_PF_UX

#ifdef CONFIG_SMALL_SPACES
	// current_user_gdt != this->small_space_area()
	// (small ==> small, or big ==> small, or small ==> big)
	//
	// precondition  : eax = this->space_context()
	//                 edx = this->small_space_area()
	//                 edi = current_user_gdt
	// postcondition : edx & 1 = 1: need_flush_iobitmap_tlb = true
	//                 edx & 2 = 1: need_flush_whole_tlb = true
	.align	16
se_smas_setup_space:
	pushl	%esi
	SET_GDT_USER KSEG 0+			// scratches ecx, esi
	movl	%edx, KSEG KMEM_CURRENT_GDT
	movl	%ecx, %ds
	movl	%ecx, %es
	pop	%esi

	movl	PAGE_DIR_ADDR, %ecx
	addl	KSEG PHYSMEM_OFFS, %ecx		// Space_context::current()

	testl	$0xFF0000FC, %edx		// small ==> big?
	jnz	1f				// no

	// small ==> big
	cmpl	%eax, %ecx			// == this->space_context()?
	setne	%dl
	shlb	$1, %dl
	jmp	se_smas_setup_space_done_to_big

1:	// small ==> small or big ==> small
	push	%esi
	movl	%ecx, %esi			// Space_context::current
#ifdef CONFIG_IO_PROT
	movl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%esi), %ecx
	andl	$1, %ecx
	orl	%ecx, %edx			// need_flush_iobitmap_tlb = 1
	movl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_IO_BMAP_BAK) << 2 (%eax),\
		%ecx
	movl	%ecx, KSEG PDIR_IDX(VAL__MEM_LAYOUT__IO_BITMAP) << 2 (%esi)
#endif
	movl	%esi, %eax

	testl	$0xFF0000FC, %edi		// small ==> small?
	jnz	1f				// yes => no smas_update

	// big ==> small: Copy complete SMAS area from Kmem::dir() to
	// current_space::dir().
	movl	%esi, %edi
	movl	KSEG KMEM_KDIR, %esi
	movl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%esi), %ecx
	cmpl	KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%edi), %ecx
	jne	2f

1:	popl	%esi
	jmp	se_smas_setup_space_done

	.align	8
2:	// Only occurs in very rare cases. Perhaps we should fail the
	// shortcut if we would call this code.
	movl	%ecx, KSEG PDIR_IDX(VAL__MEM_LAYOUT__SMAS_VERSION) << 2 (%edi)
	int	$3
	leal	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_START) << 2 (%esi), %esi
	leal	PDIR_IDX(VAL__MEM_LAYOUT__SMAS_START) << 2 (%edi), %edi
	xorl	%ecx, %ecx
	movl	$ (PDIR_IDX(VAL__MEM_LAYOUT__SMAS_END - \
			    VAL__MEM_LAYOUT__SMAS_START) - 1), %ecx
3:	movl	KSEG (%esi, %ecx, 4), %edx
	movl	%edx, KSEG (%edi, %ecx, 4)
	decl	%ecx
	jns	3b

	movb	$2, %dl				// need_flush_whole_tlb = 1
	pop	%esi
	jmp	se_smas_setup_space_done
#endif


	.globl	in_slow_ipc1
	.globl	i30_ret_switch

#endif // CONFIG_ASSEMBLER_IPC_SHORTCUT

	// fast return from Dirq::hit
	.align	16
	.globl	fast_ret_from_irq
fast_ret_from_irq:
	CHECK_SANITY $3				// scratches ecx
	RESET_USER_SEGMENTS $3,in_cli		// scratches ecx
	RESTORE_STATE_AFTER_IPC
	popl	%eax
	andl	$0x7f, 4(%esp)			// if entered using sysenter
	orl	$EFLAGS_IF, 8(%esp)		// if entered using sysenter
	iret

