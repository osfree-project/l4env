#ifndef FIXUP_CODE_S__
#define FIXUP_CODE_S__

#include "globalconfig.h"

#ifdef CONFIG_LOCAL_IPC

#include "config_tcbsize.h"
#include "config_gdt.h"
#include "tcboffset.h"
#include "local_ipc_asm.h"
#include "low_level.h"
#include "shortcut.h"

#define ASM_PAGE_MASK	0xfffff000

/* scratches %edx */
.macro	SET_READY_STATE dst_thread
	orl	$Thread_ready,  OFS__THREAD__STATE (\dst_thread)

	andl	$~(Thread_ipc_receiving_mask |	\
		   Thread_ipc_in_progress |	\
		   Thread_lipc_ready	  | \
		   Thread_utcb_ip_sp), OFS__THREAD__STATE(\dst_thread)
	
	
	movl	OFS__THREAD__UTCB_PTR(\dst_thread), %edx
	
	/* set in utcb lipc no possible */
	movl	$VAL__UTCB_STATE_NO_LIPC,  OFS__UTCB__STATUSWORD (%edx)
.endm
	

/*
  set Thread state to waiting
  scratches %edx
 */
.macro SET_IPC_WAIT_STATE rcv_thread
	movl	OFS__THREAD__STATE (\rcv_thread), %edx

	andl	$(~Thread_ready), %edx

	orl	$(Thread_receiving | \
		Thread_ipc_in_progress | \
		Thread_lipc_ready |\
		Thread_utcb_ip_sp), %edx

	movl	%edx, OFS__THREAD__STATE (\rcv_thread)
	movl	$0, OFS__THREAD__PARTNER (\rcv_thread)
.endm



/* scratches %ecx, %edx */
.macro NEED_STACK_COPY  this_ptr, fail_exit
	movl	global_utcb_ptr, %edx
	movl	OFS__THREAD__LOCAL_ID(\this_ptr), %ecx
	cmpl	(%edx), %ecx
	je	\fail_exit
.endm


.macro REGS this_ptr, dst
	leal	(THREAD_BLOCK_SIZE - OFS__ENTRY_FRAME__MAX)(\this_ptr), \dst
.endm


/* scrachtes %ecx */
.macro FROM_USER_MODE csseg, fail_exit
        movl    \csseg, %ecx
        andb    $3, %cl
        jz      \fail_exit
.endm


/* scratches %edx */
.macro LOCAL_ID_TO_KTCB_PTR local_id, dst, fail_exit
	
	/* test in utcb area range */
	movl	\local_id, %edx
	subl	$VAL__V2_UTCB_ADDR, %edx
	jb	\fail_exit

	cmpl	$(VAL__MAX_THREADS_PER_TASK*OFS__USERTCB__MAX), %edx
	jae	\fail_exit

	/* correct alignment */
	andl	$~(OFS__USERTCB__MAX-1), %edx

	/* shift to KTCB offset */
	shll	$(SHIFT__THREAD - SHIFT__UTCB), %edx

	/*
	  get space-tcb base address,
	  i.e. tcb-Address of the first thread in the address space
	*/
	movl	%esp, \dst
	andl	$~((THREAD_BLOCK_SIZE*VAL__MAX_THREADS_PER_TASK)-1), \dst

	/* add the offset (threadnr * sizeof(KTCB)) */
	addl	%edx, \dst
.endm


/*
  where is the LIPC-CODE mapped for this Task
*/
.macro LIPC_CODE target
	movl	LIPC_KIP_ADDR, \target
	addl	$OFS__KIP__LIPC_CODE, \target
.endm


.macro	COPY_KERNEL_STACK_ASM csseg

	pushl	%edi
	pushl	%ebx
	pushl	%eax
	
	ESP_TO_TCB_AT %ebx
	
	NEED_STACK_COPY %ebx, 8f


	
	movl	global_utcb_ptr, %ecx
	LOCAL_ID_TO_KTCB_PTR (%ecx), %eax, 8f
	
	/* check valid state */
	testl	$Thread_lipc_ready, OFS__THREAD__STATE (%eax)
	jz	8f

	/* locked? */
	cmpl	$0, OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (%eax)
	jne	8f

	SET_READY_STATE	%eax

	SET_IPC_WAIT_STATE %ebx

	/* calc stack offset */
	movl	%esp, %edx
	subl	%ebx, %edx
	
	pushl	%edi
	cld
	pushl	%esi
	
	/* this + stack_offset */
	leal	(%ebx, %edx), %esi
	leal	(%eax, %edx), %edi

	/* size = TCB_SIZE - stack_offset */
	movl	$THREAD_BLOCK_SIZE, %ecx
	subl	%edx, %ecx
	shrl	$2, %ecx
	
	/* copy stack */
	rep	movsd

	popl	%esi
	popl	%edi

	/* calc new stackptr = DST_TCB + stack_offset */
	leal	(%eax, %edx), %esp

	/* set the new stack also in the TSS */	
	// *(kmem::kernel_esp()) = reinterpret_cast<Address>(regs() + 1);
	movl	 CPU_TSS, %edx
	leal	THREAD_BLOCK_SIZE (%eax), %ecx
	movl	%ecx, 4 (%edx)  // x86_tss.esp0

	/* we are on the new stack now, setting up a ipc-stack on the old */
	REGS %ebx, %edx
	
	//  ipc_regs->rcv_desc(0);	
	movl $0, REG_EBP(%edx)

	//  ipc_regs->snd_desc(0);	
//	movl $0, REG_EAX(%edx)

	//  ipc_regs->ecx = L4_timeout::NEVER;
//	movl $0, REG_ECX(%edx)

	//  ipc_regs->rcv_source(L4_uid::INVALID);
//	movl $0, REG_EDI(%edx)
//	movl $0, REG_ESI(%edx)

	/*   set_receive_regs(ipc_regs);  */
	movl	%edx, OFS__THREAD__RCV_REGS (%ebx)
	
	/*  *--sp = (Mword ) &_asm_user_invoke_from_localipc; */
	movl	$_asm_user_invoke_from_localipc, -4(%edx)
	subl	$4, %edx
	
	/*   set_kernel_sp(sp); */
	movl %edx, OFS__THREAD__KERNEL_SP(%ebx)
8:

	/* lazy update the utcb state to run */
//	movl	OFS__THREAD__UTCB_PTR(%ebx), %ecx
//	orb	$VAL__UTCB_STATE_NO_LIPC, OFS__UTCB__STATUSWORD(%ecx)

	popl	%eax
	popl	%ebx
	popl	%edi
7:
.endm


/*
  ebx is the this pointer, entry_frame and ipc_sys_frame are on the stack
  segmentes are set, %esp pointing to to the begin of save_regs
  this is the "optimized" version of copy kernel stack if we entered
  the kernel with int30
*/
.macro COPY_KERNEL_STACK_ASM_IPC_FRAME


	/* test if userland thread switch happened */	
	NEED_STACK_COPY %ebx, 9f

	/* calculate new KTCB from new current utcb pointer */
	movl	global_utcb_ptr, %ecx
	LOCAL_ID_TO_KTCB_PTR (%edx), %edi, 9f

	/* check valid state, if the ktcb is not mapped this should also fail */
	testl	$Thread_lipc_ready, OFS__THREAD__STATE (%edi)
	jz	9f

	/* dst locked? */
	cmpl	$0, OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (%edi)
	jne	9f

	/* update state set new tread running */
	SET_READY_STATE %edi

	/* set current thread sleeping */
	SET_IPC_WAIT_STATE %ebx

	/* caculate dst->regs() */
	REGS %edi, %eax

	/* now copy the entry frame to the new stack */
	movl REG_ECX(%esp), %edx
	movl REG_EDX(%esp), %ecx
	
	movl %edx, REG_ECX(%eax)
	movl %ecx, REG_EDX(%eax)

	movl REG_ESI(%esp), %edx
	movl REG_EDI(%esp), %ecx
	
	movl %edx, REG_ESI(%eax)
	movl %ecx, REG_EDI(%eax)

	movl REG_EBX(%esp), %edx
	movl REG_EBP(%esp), %ecx
	
	movl %edx, REG_EBX(%eax)
	movl %ecx, REG_EBP(%eax)

	movl REG_EAX(%esp), %edx
	movl REG_EIP(%esp), %ecx
	
	movl %edx, REG_EAX(%eax)
	movl %ecx, REG_EIP(%eax)

	movl REG_ESP(%esp), %edx
	movl REG_EFL(%esp), %ecx
	
	movl %edx, REG_ESP(%eax)
	movl %ecx, REG_EFL(%eax)

	/* switch stack pointer in the tss */
	// *(kmem::kernel_esp()) = reinterpret_cast<Address>(regs() + 1);
	movl	CPU_TSS, %edx
	leal	THREAD_BLOCK_SIZE (%edi), %ecx
	movl	%ecx, 4 (%edx)  // x86_tss.esp0

	/* set valid receive descriptor */
	//  ipc_regs->rcv_desc(0);	
	movl $0, REG_EBP(%esp)

	/*   current()->set_receive_regs(ipc_regs);  */
	movl	%esp, OFS__THREAD__RCV_REGS (%ebx)
	
	/*  *--sp = (Mword ) &_asm_user_invoke_from_localipc; */
	pushl $_asm_user_invoke_from_localipc
	
	/*   current()->set_kernel_sp(sp); */
	movl %esp, OFS__THREAD__KERNEL_SP(%ebx)

	/* switch stack */
	movl	%eax, %esp

	/* set new this ptr, the shortcut depends on it */
	movl %edi, %ebx

	/* restore %eax, for the shortcut and syscall-stuff  */
	movl REG_EAX(%esp), %eax

9:

.endm


/* 
	scratches normaly %eax, %ecx and %edx
	if fixup needed then %eax and %edi too
*/
.macro FIXUP_KERNEL_STACK_ASM csseg, eip_reg, eax_reg, ecx_reg,	\
			  ebp_reg, esp_reg, esi_reg, edi_reg

	FROM_USER_MODE \csseg, 5f

#define	EIP_offset	%ecx
/*
	first we need to determine if the current user eip
	is in the LIPC-KIP area
*/


	movl	\eip_reg, EIP_offset
	movl	LIPC_KIP_ADDR, %edx

	/* test if user eip is in the LIPC-KIP */
	movl	EIP_offset, %eax

	andl	$ASM_PAGE_MASK, %eax
	cmpl	%eax, %edx
	jne	4f
	
	/* lipc code area location in the kip */
	addl	$OFS__KIP__LIPC_CODE, %edx

	/* calc the OFFSET to LIPC-KIP */
	subl	%edx, EIP_offset
	cmpl	$_lipc_restart_point_offset, EIP_offset
	jbe	4f

	cmpl	$_lipc_forward_point_offset, EIP_offset
	ja	1f


	/* restart point */

	/* set the user eip back to the restart point */
	leal	_lipc_restart_point_offset(%edx), %eax
	movl	%eax, \eip_reg

	jmp	4f

1:	cmpl	$_lipc_finish_point_offset, EIP_offset

	jae	4f

	/* forward code */

	/* scratches %edx */
	LOCAL_ID_TO_KTCB_PTR \ecx_reg, %edi, 4f
	LOCAL_ID_TO_KTCB_PTR \eax_reg, %eax, 4f
	/* we are in the forward code, only eax_reg, ecx_reg and esi_reg
	   needed for the complete LIPC */

	movl	EIP_offset, %edx
#undef 	EIP_offset
	movl	OFS__THREAD__UTCB_PTR(%edi), %ecx

	cmpl	$_lipc_rcv_desc_invalid, %edx
	ja	2f

	/* set the source utcb to sleep */
	movl	\esi_reg, %edx
	
	cmpl	$0,%edx
	jne	1f
	int3
	
1:	movl	%edx, OFS__UTCB__STATUSWORD(%ecx)

2:	
	/* reload the LIPC-CODE location */
	LIPC_CODE %edx

	/* set user eip to finish point */
	leal	_lipc_finish_point_offset(%edx), %edx
	movl	%edx, \eip_reg

	/* store gs:0, the current utcb */
	movl	global_utcb_ptr, %ecx	// load pointer to utcb pointer page
	movl	\eax_reg, %edx
	movl	%edx, (%ecx)
	
	/* store the source V2 L4_uid in edi:esi */
	movl	OFS__THREAD__ID(%edi), %edx
	movl	%edx, \esi_reg

	movl	(OFS__THREAD__ID+4)(%edi), %edx
	movl	%edx, \edi_reg
	/* if edi_reg directly point to %edi, dont touch it after that */
	
	/* load kernel utcb ptr from the Dstination K-TCB */
	movl	OFS__THREAD__UTCB_PTR(%eax), %edx

	/* 
	load %esp and %ebp from the dstination UTCB
	the lipc code will do a jmp *%ebp after this */
	movl	OFS__UTCB__ESP(%edx), %ecx
	movl	%ecx, \esp_reg

	movl	OFS__UTCB__EIP(%edx), %eax
	movl	%eax, \ebp_reg

	movl	$VAL__UTCB_STATE_NO_LIPC, OFS__UTCB__STATUSWORD(%edx)


4:
	/* check and copy kernel stack if necessary */
	/* dummy user csseg */
	COPY_KERNEL_STACK_ASM $0x3

5:
.endm


/* 
   Assembler wrapper for the C++ version of the roolback/forward and fixup
   code
*/

/* 
   Do the rollback and rollforward stuff in fixup_kern_stack.
   If necessary, copy the kernel stack to the new one and setup a special stack
   frame on the old kernel stack.
   Because we will get stackrefs as macro params,
   we build up directly the call stack via mov and negative offset.
*/
.macro FIXUP_KERNEL_STACK_C csseg, eip_reg, eax_reg, ecx_reg, \
			  ebp_reg, esp_reg, esi_reg, edi_reg
	
	FROM_USER_MODE \csseg, 3f

/*	build up the call stack */	
	leal	\eip_reg, %edx
	movl	%edx, -36(%esp)

	leal	\eax_reg, %ecx
	movl	%ecx, -32(%esp)

	leal	\ecx_reg, %edx
	movl	%edx, -28(%esp)

	leal	\ebp_reg, %ecx
	movl	%ecx, -24(%esp)

	leal	\esp_reg, %edx
	movl	%edx, -20(%esp)

	leal	\esi_reg, %ecx
	movl	%ecx, -16(%esp)

	leal	\edi_reg, %edx
	movl	%edx, -12(%esp)

	pushl	%eax
	pushl	%ebp

	addl	$-28,  %esp

	call	fixup_kern_stack

	addl	$28,  %esp

	movl	%esp, -4(%esp)
	addl	$-4, %esp

	call	copy_kernelstack

	addl	$4, %esp

//	test if copy_kernelstack had copied the kernelstack
//	and returns a new stackpointer
	
	cmpl	%eax, %esp
	je	2f

	ESP_TO_TCB_AT %edx
	mov	%eax, %esp

	push	%edx
	call	fixup_old_kernel_stack

	addl	$4, %esp
2:	popl	%ebp
	popl	%eax
3:
.endm

/* 
   If necessary, copy the kernel stack to the new one and setup a special stack
   frame on the old kernel stack.
   No rollback/rollforward is performed.
   Because we will get stackrefs as macro params,
   we build up directly the call stack via mov and negative offset.
*/
.macro COPY_KERNEL_STACK_C csseg
	
	FROM_USER_MODE \csseg, 3f
	
	pushl	%eax
	pushl	%ebp

	movl	%esp, -4(%esp)
	addl	$-4, %esp

	call	copy_kernelstack
	addl	$4, %esp

	cmp	%eax, %esp
	je	2f

	ESP_TO_TCB_AT %edx
	mov	%eax, %esp

	push	%edx
	call	fixup_old_kernel_stack

	addl	$4, %esp
2:	popl	%ebp
	popl	%eax
3:
.endm



.macro COPY_KERNEL_STACK csseg
//	COPY_KERNEL_STACK_ASM \csseg
	COPY_KERNEL_STACK_C \csseg
.endm

.macro FIXUP_KERNEL_STACK csseg, eip_reg, eax_reg, ecx_reg, \
			  ebp_reg, esp_reg, esi_reg, edi_reg

//	FIXUP_KERNEL_STACK_ASM	\csseg, \eip_reg, \eax_reg, \ecx_reg, \
//				\ebp_reg, \esp_reg, \esi_reg, \edi_reg

	FIXUP_KERNEL_STACK_C	\csseg, \eip_reg, \eax_reg, \ecx_reg, \
				\ebp_reg, \esp_reg, \esi_reg, \edi_reg


.endm


.macro LIPC_IPC_COPY_STACK sp,tcb
	COPY_KERNEL_STACK 32(\sp)
	ESP_TO_TCB_AT \tcb	/* recalc this ptr */
	STORE_EIP_ESP_IN_UTCB \tcb
.endm

.macro LIPC_COPY_STACK sp,tcb
	COPY_KERNEL_STACK 32(\sp)
	ESP_TO_TCB_AT \tcb		/* recalc this ptr */
.endm

.macro LIPC_COPY_STACK_OFS sp, ofs
	COPY_KERNEL_STACK \ofs(\sp)
.endm

.macro LIPC_RELOAD_IP_SP tcb
	ESP_TO_TCB_AT %ebx	/* recalc this ptr */
	RELOAD_EIP_ESP_FROM_UTCB %ebx
.endm

#else /* CONFIG_LOCAL_IPC */

.macro LIPC_IPC_COPY_STACK sp,tcb
.endm
.macro LIPC_COPY_STACK sp,tcb
.endm
.macro LIPC_COPY_STACK_OFS sp, ofs
.endm
.macro LIPC_RELOAD_IP_SP tcb
.endm

#endif
#endif
