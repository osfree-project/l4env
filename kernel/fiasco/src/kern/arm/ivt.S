/* -*- asm -*- */

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
	nop
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
	cmn	lr, #(7*4)		@ Range Check !!! UNSIGNED !!!
	bls	prefetch_abort		@ no syscall
	b	syscall

#if 0
	nop
	b	prefetch_abort		@ NULL pointer access ?
	b	ipc_entry		@ abort_lr = 0x08
	b	id_nearest_entry	@ abort_lr = 0x0C
	b	fpage_unmap_entry	@ abort_lr = 0x10
	b	thread_switch_entry	@ abort_lr = 0x14
	b	thread_schedule_entry	@ abort_lr = 0x18
	b	lthread_ex_regs_entry	@ abort_lr = 0x1C
	b	create_thread_entry	@ abort_lr = 0x20
	b	user_kdebug_entry	@ abort_lr = 0x24
#endif
prefetch_abort:
	exceptionframe 4

	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc     p15, 0, r0, c5, c0, 0	@ Load FSR into r0
	ldr	r1, [sp, #68]
	mov	r2, r1

	adr	lr, exception_return
#if 0 /*EXCEPTION_VECTOR_RELOCATED*/
	b	user_pagefault
#else
	ldr	pc, 1f			@ Jump to C code
#endif

#if 1 /*ndef EXCEPTION_VECTOR_RELOCATED*/
1:	.word	user_pagefault
#endif

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
	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	/* user data abort */
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	mrc     p15, 0, r0, c6, c0, 0	@ Load FAR into r0
	ldr	r2, [sp, #68]

	adr	lr, exception_return	@ set return address
#if 0 /*def EXCEPTION_VECTOR_RELOCATED*/
	bne	kernel_pagefault
	b	user_pagefault
#else
	ldrne	pc, 1f			@ kernel pagefault
	ldr	pc, 2f			@ user pagefault
#endif
	b	kern_kdebug_entry

#if 1	
1:	.word	kernel_pagefault
2:	.word	user_pagefault
#endif


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

	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	mov	r0, lr
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


	
/*
**
** Syscall entry points
**
*/
	
		
/*
 * Syscall ipc ()
 *
 *    Pre:	r0 = destination thread id
 *		r1 = Snd descriptor,
 *		r2 = Rcv descriptor,
 *		r3 = timeout
 *		r4 - r12 = 9 msg words to send
 *    Post:	r0 = Msg dope + cc,
 *		r1 = Source thread ID,
 *		r4 - r12 = 9 msg words received (or undefined).
 *
 */
syscall:
	syscallframe
	stmdb   sp, {r0 - r12}^
	mvn	r0, lr, lsl #2
	adr	lr, 1f
	ldr	pc, 2f			@ Jump to C code
1:	

	ldmia   sp, {r0 - r12}^	
	/* Return */
	ldr	lr, [sp]		@ Unstack SPSR
	msr	spsr, lr		@ Load SPSR from abort_lr
	ldmib	sp, {sp,lr}^		@ restore user sp and lr
	nop
	add	sp, sp, #20
	ldr	lr, [sp, #-4]		@ Read return address
	movs	pc, lr			@ return and switch mode

2:	.word dispatch_syscall



/**********************************************************************
	kdebug entry
 **********************************************************************/

3:	.word enter_jdb
	
	.globl	kern_kdebug_entry
kern_kdebug_entry:
	sub	sp, sp, #12
	str	sp, [sp, #-0x10]
	str	lr, [sp, #-0x0C]
	mrs	lr, cpsr
	str	lr, [sp, #-0x08]
//	str	lr, [sp, #-0x04] /* Store cpsr in frame->spsr ??? */ 
	mrs	lr, spsr
	str	lr, [sp, #-0x04] /* Store spsr in frame->spsr */ 
	str	lr, [sp, # 0x04] /* Store spsr on stack */
	sub	sp, sp, #(15*4 + 16)
	stmia	sp, {r0-r14}^
	nop
	mov	r0, sp
	
	/* use kdebug's own stack */
	ldr	sp, _kdebug_stack_top
	stmdb	sp!, {r0}
	adr	lr, 1f
	ldr	pc, 3b
1:
	ldr	sp, _kdebug_stack_top
	ldr	sp, [sp, #-4]
	ldmia	sp, {r0-r14}^
	nop
	add	sp, sp, #(15*4 + 16)
	ldr	lr, [sp, #-0x08]
	msr	cpsr, lr
	ldr	lr, [sp, # 0x04] /* Restore spsr from stack */
	msr	spsr, lr
	ldr	lr, [sp, #-0x0C]
	ldr	sp, [sp, #-0x10]
	add	sp, sp, #12
	mov	pc, lr

	.globl	_kdebug_stack_bottom
_kdebug_stack_bottom:
	.space	1024

	.globl	_kdebug_stack_top
_kdebug_stack_top:
	.word	_kdebug_stack_top



