/* 
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the nema IBM not be used in advertising or publicity 
 * pertaining to distribution of the software without specific, written
 * prior permission.
 * 
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * This file contains a 'gate_init' initialization table
 * to initialize the x86 processor trap vectors to default entrypoints.
 * These entrypoints simply push a standard trap_state frame
 * and jump to the 'trap_handler' routine.
 */

#define ASSEMBLER
#include <flux/x86/asm.h>
#include <flux/x86/eflags.h>
#include <flux/x86/trap.h>
#include <flux/x86/seg.h>
#include <flux/x86/gate_init.h>
#include <flux/x86/base_trap.h>

/*
 * No error code.  Clear error code and push trap number.
 */
#define	EXCEPTION(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	jmp	alltraps

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_U | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	jmp	alltraps

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(n)						;\
	jmp	alltraps


GATE_INITTAB_BEGIN(base_trap_inittab)

EXCEPTION(0x00,t_zero_div)
EXCEPTION(0x01,t_debug)
EXCEP_USR(0x03,t_int3)
EXCEP_USR(0x04,t_into)
EXCEP_USR(0x05,t_bounds)
EXCEPTION(0x06,t_invop)
EXCEPTION(0x07,t_nofpu)
EXCEPTION(0x08,a_dbl_fault)
EXCEPTION(0x09,a_fpu_over)
EXCEP_ERR(0x0a,a_inv_tss)
EXCEP_ERR(0x0b,t_segnp)
EXCEP_ERR(0x0c,t_stack_fault)
EXCEP_ERR(0x0d,t_gen_prot)
EXCEP_ERR(0x0e,t_page_fault)
EXCEPTION(0x0f,t_trap_0f)
EXCEPTION(0x10,t_fpu_err)
EXCEPTION(0x11,t_trap_11)
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

GATE_INITTAB_END

alltraps:

	/* Save the rest of the state frame.
	   Note that on the 486 and up,
	   pusha is slower than the equivalent set of manual pushes,
	   so don't do this in your own code if you want speed.
	   For our purposes here we're more worried
	   about size and simplicity...  */
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	/* Load the kernel's segment registers.  */
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es

	/* GCC likes the direction flag cleared.  */
	cld

	/* Call the C handler function if one has been installed.  */
	movl	EXT(base_trap_handler),%eax
	orl	%eax,%eax
	jz	1f
	pushl	%esp
	call	*%eax
	popl	%edx

	/* If the handler function returned zero (success),
	   then resume execution as if the trap never happened.
	   Otherwise, just panic.  */
	orl	%eax,%eax
	jnz	1f

	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	addl	$4*2,%esp	/* Pop trap number and error code */
	iret

1:
	/* Dump the register state and panic.  */
	UNEXPECTED_TRAP

