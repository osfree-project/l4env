/* -*- asm -*- */

#include "config_tcbsize.h"

#define S_FRAME_SIZE	36

#define S_PSR           28
#define S_PC		32

.macro  switch_to_kernel
	str	lr, s_lr
	mrs	lr, spsr
	str	lr, s_spsr
	msr	cpsr_c, #0xd3
	nop
	nop
	nop
	str	lr, [sp, #-8]!
	ldr	lr, s_spsr
	msr	spsr, lr
	ldr	lr, s_lr
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
	
.macro exceptionframe adjust
	sub	lr, lr, #\adjust
	str	lr, [sp, #4]
        mrs	lr, spsr    
        str     lr, [sp, #-12]!
	stmib	sp, {sp,lr}^	
	nop
.endm


/*	
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


	
.macro syscallframe	
	stmib	sp, {lr}^
	nop			
        mrs	lr, spsr        
        str     lr, [sp, #-12]!	
	stmib	sp, {sp}^
.endm



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

/*
**
** Exception entry points.
**
*/

/*
 * Exception undef ()
 *
 *    Exception is an undefined instruction.
 *
 */
undef_entry:
	switch_to_kernel
	mov	r0,#0xFE
	b	kern_kdebug_entry
	
/*
 * Exception swi ()
 *
 *    Exception is a software interrupt (typically a syscall in normal
 *    OSes).
 *
 */
swi_entry:
	switch_to_kernel
	mov	r0,#0xFD
	b	kern_kdebug_entry


	
/*
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
	cmn	lr, #0x28		@ Range Check !!! UNSIGNED !!!
	bls	prefetch_abort		@ no syscall
	cmn	lr, #0x08
	bhi	prefetch_abort
	syscallframe			
	stmdb   sp, {r0 - r12}^
        nop
        sub     sp, sp, #13*4
	ldr	r0, s_lr
	adr	r1, sys_call_table
	sub	r0, r1, r0
	adr	lr, 2f
        msr	cpsr_c, #0x13 // enable IRQs
1:	ldr	pc, [r0]
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
	
prefetch_abort:
	exceptionframe 4

	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #0x00ff0000
	orr	r1, r1, #0x00100000
	ldr	r0, [sp, #68]
	mov	r2, r0

	adr	lr, exception_return
	ldr	pc,  .LCpagefault_entry	@ Jump to C code

kernel_prefetch_abort:			@ Kernel generated IAbort
					@ Should not get IAborts in kernel
					@ Kernel Panic
	switch_to_kernel
	mov	r0,#0xFC
	b	kern_kdebug_entry

/*
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
	stmdb   sp, {r0 - r12}^         @ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	/* user data abort */
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #0x00ff0000
	mrc     p15, 0, r0, c6, c0, 0	@ Load FAR into r0
	ldr	r2, [sp, #68]
	ldr	r3, [r2]		@ Load faulting insn
	tst	r3, #(1<<20)
	orrne	r1, r1, #0x00100000	@ Set read fault bit
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	orreq	r1, r1, #0x00080000
	adr	lr, exception_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault	

.LCpagefault_entry:	.word	pagefault_entry


/*
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
exception_return:
	ldmia	sp, {r0 - r12}^		@ Restore user state
	nop
	add	sp, sp, #(13*4)

	ldr	lr, [sp], #12		@ Unstack SPSR
	msr	spsr, lr		@ Load SPSR from kernel_lr
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	ldmeqdb	sp, {sp,lr}^		@ restore user sp and lr
	nop
	add	sp, sp, #8		@ Read return address
	ldr	lr, [sp, #-4]		@ Read return address
	moveqs	pc, lr			@ return and switch mode
        ldrne   lr, [sp, #-8]           @ load old kernel lr
	ldmdb	sp, {pc}^		@ switches mode - doesnt do anything
	nop

	bl	kern_kdebug_entry
/*
 * Exception irq ()
 *
 *    Exception is an interrupt.  Generate interrupt IPC.

         *
 */
irq_entry:	
	switch_to_kernel
	exceptionframe 4

	stmdb   sp, {r0 - r12}^   	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)
        mov     r0, sp
//	ldr	r0, [sp, #68]
#if 0 /*def EXCEPTION_VECTOR_RELOCATED*/
        bl      irq_handler
#else
        mov     lr, pc
        ldr     pc, 1f
#endif
	b	exception_return
#if 1	
1:	.word	irq_handler
#endif



/*
 * Exception fiq ()
 *
 *    Exception is a fast interrupt.  Generate interrupt IPC.
 *
 */
fiq_entry:
	switch_to_kernel
	mov	r0,#0xFD
	b	kern_kdebug_entry

#if 0
#define SYSCALL(name)			  \
sys_##name##_entry:			  \
	syscallframe			; \
	stmdb   sp!, {r0 - r12}^	; \
	bl	sys_##name##_wrapper	; \
	ldmia   sp!, {r0 - r12}^	; \
	/* Return */			; \
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
	
sys_call_table:
	.word sys_kdb_ke
	.word sys_kdb_ke
SYSCALL(ipc)
/*	.word ipc_short_cut_wrapper*/
SYSCALL(id_nearest)
SYSCALL(fpage_unmap)
SYSCALL(thread_switch)
SYSCALL(thread_schedule)
SYSCALL(thread_ex_regs)
SYSCALL(task_new)
	.word sys_kdb_ke
	
/**********************************************************************
	kdebug entry
 **********************************************************************/

3:	.word enter_jdb
	
	.globl	kern_kdebug_entry
kern_kdebug_entry:
        str     sp,_kdebug_stack_top-8
        str     lr,_kdebug_stack_top-4
        mrs	lr, cpsr
	str	lr,_kdebug_stack_top-12
        mrs	lr, spsr
	str	lr,_kdebug_stack_top-16
        adr     sp,_kdebug_stack_top-(15*4 + 16)
        stmia   sp,{r0-r14}^
        nop
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



