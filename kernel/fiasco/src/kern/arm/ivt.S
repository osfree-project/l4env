/* -*- asm -*- */

#include "config_tcbsize.h"
#include "tcboffset.h"



/**********************************************************************
 * calculate the TCB address from a stack pointer
 */
.macro CONTEXT_OF reg, ptr
	bic	\reg, \ptr, #((THREAD_BLOCK_SIZE-1) & 0xff)
	bic	\reg, \reg, #((THREAD_BLOCK_SIZE-1) & 0xff00)
.endm

/**********************************************************************
 * Reset the thread cancel flag. 
 * Register r0 is scratched and contains the thread state afterwards
 */
.macro RESET_THREAD_CANCEL_AT tcb
	ldr 	r0, [\tcb, #(OFS__THREAD__STATE)]
	bic	r0, r0, #0x100
	str	r0, [\tcb, #(OFS__THREAD__STATE)]
.endm

/****************************
 * some handy definitions
 */
#define S_FRAME_SIZE	36

#define S_PSR           28
#define S_PC		32

/**************************************************************************
 * Enter kernel mode (i.e. switch from any exception mode to the 
 * kernel mode and transfer the exception state).
 */
.macro  __switch_to_kernel reg
	str	lr, s_lr
	mrs	\reg, spsr
	str	\reg, s_spsr
	msr	cpsr_c, #0xd3
	str	lr, [sp, #-8]!
	ldr	lr, s_spsr
	msr	spsr, lr
	ldr	lr, s_lr
.endm
.macro  switch_to_kernel
	__switch_to_kernel r14
.endm

/*************************************************************************
 * return for an exception
 */
.macro 	return_from_exception
	ldr	lr, [sp], #12		@ Unstack SPSR
	msr	spsr, lr		@ Load SPSR from kernel_lr
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	moveq	lr, sp			@ restore user lr and user sp by
					@ using lr as source register for
					@ ldmeqdb ('ldmsqdb sp, {sp,lr}^'
					@ does not work on arm920t)
	ldmeqdb	lr, {sp,lr}^		@ restore user sp and lr
	add	sp, sp, #8		@ Read return address
	ldr	lr, [sp, #-4]		@ Read return address
	moveqs	pc, lr			@ return and switch mode
        ldrne   lr, [sp, #-8]           @ load old kernel lr
	ldmdb	sp, {pc}^		@ switches mode - doesnt do anything
	nop
.endm

/***********************************************************************
 * Enter the kernel slowtrap handler
 *
 * Stack the state and call 'slowtrap_entry' with sp and error code
 */
.macro 	enter_slowtrap_w_stack errorcode
	mov	r1, #\errorcode
	stmdb	sp!, {r0, r1}
	mov 	r0, sp
	adr	lr, exception_return
	ldr	pc, .LCslowtrap_entry
.endm

.macro 	enter_slowtrap errorcode
	stmdb 	sp!, {r0 - r12}
	enter_slowtrap_w_stack \errorcode
.endm

/*	
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr'  |  (pc after syscall)
 *             +-------+
 *   old sp -> | km_lr |
 *	       +-------+
 *             |  lr^  |  (user lr)
 *             +-------+
 *             |  sp^  |  (user sp)
 *             +-------+
 *       sp -> |  spsr |
 *             +-------+
 *             |       |
 *
 */


/*************************************************************************
 *
 * Generate stack for exception entries
 * - Adjust return address
 * - Store return address at [sp + 4]
 * - Store spsr at [sp - 12]
 * - Store user sp at [sp - 4]
 * - Store user lr at [sp - 8]
 * - sp := sp - 12
 */
.macro	exceptionframe adjust
	sub	lr, lr, #\adjust
	@ Adjust PC if it is in the special atomic insns area
        cmp     lr, #0xffffe000
	addls	pc, pc, #12
	cmp	lr, #0xfffff000
	addhs   pc, pc, #4
	tst	lr, #0x10
	biceq	lr, lr, #0x0ff
	@ ---------------------------------------------------
	str	lr, [sp, #4]
        mrs	lr, spsr    
        str     lr, [sp, #-12]!
	stmib	sp, {sp,lr}^	
	nop
.endm


/***************************************************************************
 * Generate stack for system call entries
 *
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr^  |  (pc after syscall)
 *             +-------+
 *   old sp -> | km_lr |
 *	       +-------+
 *             |  xx   |
 *             +-------+
 *             |  sp^  |  (user sp)
 *             +-------+
 *       sp -> |  spsr |
 *             +-------+
 *             |       |
 *
 */
.macro	syscallframe	
	stmib	sp, {lr}^
	nop			
        mrs	lr, spsr        
        str     lr, [sp, #-12]!	
	stmib	sp, {sp}^
.endm

.macro  enter_sys_call no_sys_call
	cmn	lr, #0x2C		@ Range Check !!! UNSIGNED !!!
	bls	\no_sys_call		@ no syscall
	cmn	lr, #0x08
	bhi	\no_sys_call
	syscallframe			
	stmdb   sp!, {r0 - r12}
	CONTEXT_OF	r1, sp
	RESET_THREAD_CANCEL_AT r1       @ sets r0 to state 
	tst	r0, #0x10000
	bne	alien_syscall
	ldr	r0, s_lr
	adr	r1, sys_call_table
	sub	r0, r1, r0
	adr	lr, 2f
1:	ldr	pc, [r0]
.global fast_ret_from_irq
fast_ret_from_irq:
2:      nop
        ldmia   sp, {r0 - r12}^
        nop
        msr	cpsr_c, #0xd3 // disable IRQs
        add     sp, sp, #13*4

	/* Return */			
	ldr	lr, [sp]		
	msr	spsr, lr		
	ldmib	sp, {sp,lr}^		
	nop				
	add	sp, sp, #20		
	ldr	lr, [sp, #-4]		
	movs	pc, lr	
.endm


/**************************************************************************
 * The Exception vector table.
 */
.section	.vect,"a"
.globl	exception_vector
exception_vector:
	nop				/* RESET	*/
	b	undef_entry		/* UNDEF	*/
	b	swi_entry		/* SWI		*/
	b	inst_abort_entry	/* IABORT	*/
	b	data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	irq_entry		/* IRQ		*/
	b	fiq_entry		/* FIQ		*/


/* locations to pass lr and spsr from one mode to the other
   these are globally shared !!! */
.section	.excp.text,"xa"
s_lr:	.word	0
s_spsr:	.word	0

/***************************************************************************
**
** Exception entry points.
**
*/

/***************************************************************************
 * Exception undef ()
 *
 *    Exception is an undefined instruction.
 *
 */
undef_entry:
	switch_to_kernel
	exceptionframe 4
	enter_slowtrap 0x00100000
	
/**************************************************************************
 * Exception swi ()
 *
 *    Exception is a software interrupt (typically a syscall in normal
 *    OSes).
 *
 */
swi_entry:
	switch_to_kernel
	enter_sys_call no_sys_call
no_sys_call:
	exceptionframe 4
	enter_slowtrap 0x00200000

.align 4

/***************************************************************************
 * Exception inst_abort ()
 *
 *    Exception is a prefetch (instruction) abort.  This exception is also
 *    used for L4 syscalls.  If the exception address is in the range 0x00
 *    to 0x24 (in the exception vector page), this is interpreted as a
 *    syscall number.  Some platforms allow the exception vector to be
 *    relocated to the beginning of the last 64K of memory.  For these
 *    platforms, we use a negative (i.e. end of address space) value to
 *    indicate the syscall number.  If exception is not within the syscall
 *    range, generate a pager IPC (or panic if within the kernel).
 *
 */
inst_abort_entry:

	switch_to_kernel

	
/**************************************************************************/
prefetch_abort: @ A real prefetch abort occured --- handled as a page fault 
	exceptionframe 4
	stmdb   sp!, {r0 - r3, r12} 	@ Stack rest of user state

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #0x00ff0000
	orr	r1, r1, #0x00330000     @ Set read bit and prefetch abort
	ldr	r0, [sp, #(9*4)]
	mov	r2, r0
	add 	r3, sp, #(5*4)
	stmdb	sp!, {r0, r1}
	adr	lr, pagefault_return
	ldr	pc,  .LCpagefault_entry	@ Jump to C code

kernel_prefetch_abort:			@ Kernel generated IAbort
					@ Should not get IAborts in kernel
					@ Kernel Panic
@	switch_to_kernel
	adr	r0, kernel_prefetch_abort_label
	b	kern_kdebug_entry





/****************************************************************************
 * Exception data_abort ()
 *
 *    Exception is a data abort.  If exception happened in user mode,
 *    generate pager IPC.  If exception happened in kernel mode, it is
 *    probably due to a non-mapped TCB (or else we panic).
 *
 *
 * Stack layout:
 *
 *   old sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | km_lr | +64
 *	       +-------+
 *             |  ulr  | +60
 *             +-------+
 *             |  usp  | +56
 *             +-------+
 *             |  spsr | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *       sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 *
 */
data_abort_entry:

	switch_to_kernel

	exceptionframe 8
	stmdb   sp!, {r0 - r3, r12}          @ Stack rest of user state

	/* user data abort */
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #0x00ff0000
	mrc     p15, 0, r0, c6, c0, 0	@ Load FAR into r0
	ldr	r2, [sp, #(9*4)]
	tst	lr, #0x20		@ comes from thumb mode?
	bne	.LChandle_thumb
	@ arm insns
	ldr	r3, [r2]		@ Load faulting insn
	tst	r3, #(1<<20)
.LCret_handle_thumb:
	orrne	r1, r1, #0x00020000	@ Set read fault bit
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	add 	r3, sp, #(5*4)
	orreq	r1, r1, #0x00010000
	orr	r1, r1, #0x00400000     @ Set error code to data abort
	stmdb	sp!, {r0, r1}
	adr	lr, pagefault_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault	

.LChandle_thumb:
	@ thumb insns
	ldrh	r3, [r2]
	and     r3, r3, #0xfe00
	teq     r3, #0x5600
	orreq   r1, r1, #0x00020000	@ Set read fault bit
	beq     .LCret_handle_thumb
	tstne   r3, #(1<<11)
	b .LCret_handle_thumb

.LCpagefault_entry:	.word	pagefault_entry
.LCslowtrap_entry:	.word	slowtrap_entry


/***************************************************************************
 * Generic return code for restoring the thread state after exceptions.
 *
 * Stack layout:
 *
 *	 sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | km_lr | +64
 *             +-------+
 *             |  ulr  | +60
 *             +-------+
 *             |  usp  | +56
 *             +-------+
 *             |  spsr | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *   old sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 */	
pagefault_return:
	cmp 	r0, #0
	ldmia	sp!, {r12, lr}
	beq	slowtrap_from_pagefault
	
        msrne	cpsr_c, #0xd3 // disable IRQs
	ldmneia	sp!, {r0 - r3, r12}		@ Restore user state
	return_from_exception

slowtrap_from_pagefault:
	ldmia 	sp!, {r0 - r3}
	stmdb 	sp!, {r0 - r11}
	stmdb 	sp!, {r12, lr}
	mov	r0, sp
	adr   	lr, exception_return
	ldr	pc, .LCslowtrap_entry	@ slow trap	

exception_return:
        msr	cpsr_c, #0xd3 // disable IRQs
	add	sp, sp, #8
	ldmia	sp!, {r0 - r12}
	return_from_exception


/***************************************************************************
 * Exception irq ()
 *
 *    Exception is an interrupt.  Generate interrupt IPC.
 *
 */
irq_entry:	
	switch_to_kernel
	exceptionframe 4

	stmdb   sp!, {r0 - r3, r12}   	@ Stack rest of user state
#if 0 /*def EXCEPTION_VECTOR_RELOCATED*/
        bl      irq_handler
#else
@	ldr	r0, [sp,#28]
        mov     lr, pc
        ldr     pc, 1f
#endif
	ldmia	sp, {r0 - r3, r12}		@ Restore user state
        msr	cpsr_c, #0xd3 // disable IRQs
	add	sp, sp, #20
	return_from_exception
#if 1	
1:	.word	irq_handler
#endif



/******************************************************************************
 * Exception fiq ()
 *
 *    Exception is a fast interrupt.  Trap into kernel debugger.
 *
 */
fiq_entry:
	switch_to_kernel
	adr	r0, fiq_label
	b	kern_kdebug_entry


/**************************************************************************/
/* The alien stuff is below                                               */
/**************************************************************************/
alien_syscall: @ Do it for an alien ---------------------------------------
	tst	r0, #0x20000
	bicne	r0, r0, #0x20000
	bne	1f
	@ Trap alien before system call -----------------------------------
	@ The trap is an insn abort on the syscall address in the kernel.
	ldr	lr, [sp, #(16+13*4)]
	str	lr, [sp, #(8 +13*4)]
	ldr	lr, s_lr
	sub	lr, lr, #4
	str	lr, [sp, #(16+13*4)]
	enter_slowtrap_w_stack 0x00300000
	@ Never reach this -- end up in user land after exception reply

1:	@ Resume the alien system call ------------------------------------
	str	r0, [r1, #(OFS__THREAD__STATE)]
	ldr	r0, s_lr
	adr	r1, sys_call_table
	sub	r0, r1, r0
	adr	lr, 2f
	ldr	pc, [r0]
2:	nop @ The return point after the resumed alien system call --------
        msr	cpsr_c, #0xd3 // disable IRQs
	@ Trap after the resumed alien system call ------------------------
	@ The trap occurs at the insn where the system call returns to.
	@ Set the bit 0x00010000 to indicate a trap after the resumed 
	@ system call.
	enter_slowtrap_w_stack 0x00310000

	
/*****************************************************************************/
/* The syscall table stuff                                                   */
/*****************************************************************************/
#if 0
#define SYSCALL(name)			  \
sys_##name##_entry:			  \
	syscallframe			; \
	stmdb   sp!, {r0 - r12}^	; \
	bl	sys_##name##_wrapper	; \
	ldmia   sp!, {r0 - r12}^	; \
	/* Return */			; \
        msr	cpsr_c, #0xd3           ; \ // disable IRQs
	ldr	lr, [sp]		; \
	msr	spsr, lr		; \
	ldmib	sp, {sp,lr}^		; \
	nop				; \
	add	sp, sp, #20		; \
	ldr	lr, [sp, #-4]		; \
	movs	pc, lr			;

#else

#define SYSCALL(name)			\
	.word sys_##name##_wrapper
#endif

.globl sys_call_table
sys_call_table:
	.word sys_kdb_ke
	.word sys_kdb_ke
/*SYSCALL(ipc)*/
	.word ipc_short_cut_wrapper
SYSCALL(id_nearest)
SYSCALL(fpage_unmap)
SYSCALL(thread_switch)
SYSCALL(thread_schedule)
SYSCALL(thread_ex_regs)
SYSCALL(task_new)
	.word sys_kdb_ke
SYSCALL(u_lock)
	
	.align 4	
	.global leave_by_trigger_exception

leave_by_trigger_exception:
	sub 	sp, sp, #20   // restore old return frame
	stmdb 	sp!, {r0 - r12}

	/* restore original IP */
	CONTEXT_OF r1, sp
	ldr 	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]
	str	r0, [sp, #(13*4 + 16)]
	mov     r0, #~0
	str	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]

	/* enforce user mode */
	ldr	r0, [sp, #(13*4 + 0)]
	and 	r0, r0, #~0xcf /* mask out mode bits and IRQ flags */
	str	r0, [sp, #(13*4 + 0)]

	enter_slowtrap_w_stack 0x00500000

	
kernel_prefetch_abort_label: .string "Kernel prefetch abort"
missed_excpt_ret_label:      .string "ERROR in exception return"
fiq_label:	             .string "FIQ entry"

.align 4

/**********************************************************************
	kdebug entry
 **********************************************************************/

/* STACK LAYOUT
   _kdebug_stack_top  | 
                      +-------------+
		      | lr (kernel) |
		      +-------------+
		      | sp (kernel) |
		      +-------------+
		      | cpsr        |
		      +-------------+
		      | spsr        |
		      +-------------+
		      | r14^ (lr^)  |
		      +-------------+
		      | r13^ (sp^)  |
		      +-------------+
		      | r12         |
		      +-------------+
		      | r11         |
		      +-------------+
		      | r10         |
		      +-------------+
		          :     :
		      +-------------+
		      | r0          |
		      +-------------+
*/

3:	.word enter_jdb
	
	.globl	kern_kdebug_entry
kern_kdebug_entry:
	mov     r1, r0
        str     sp,_kdebug_stack_top-8
        str     lr,_kdebug_stack_top-4
        mrs	lr, cpsr
	str	lr,_kdebug_stack_top-12
        mrs	lr, spsr
	str	lr,_kdebug_stack_top-16
        adr     sp,_kdebug_stack_top-(15*4 + 16)
        stmia   sp,{r0-r14}^
@	adr     r1, fiq_label
        mov     r0, sp
	adr	lr, 1f
	ldr	pc, 3b
1:
        ldmia   sp,{r0-r14}^
	ldr	lr,_kdebug_stack_top-12
        msr	cpsr, lr
        ldr     lr,_kdebug_stack_top-16
        msr	spsr, lr
        ldr     sp,_kdebug_stack_top-8
	ldr	lr,_kdebug_stack_top-4
	mov	pc, lr			@ return and switch mode


	.globl	_kdebug_stack_bottom
_kdebug_stack_bottom:
	.space	1024

	.globl	_kdebug_stack_top
_kdebug_stack_top:
	.word	_kdebug_stack_top

