/*
 * Remote PC serial-line debugging for the Flux OS Toolkit
 * Copyright (C) 1996-1994 Sleepless Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Author: Bryan Ford
 */

#include <flux/x86/asm.h>

	.text

ENTRY(gdb_pc_com_intr)

	/* Push a dummy error code and trap number,
	   signaling gdb_trap() that a hardware interrupt occurred.  */
	pushl	$0
	pushl	$-1

	/* Save the rest of the state frame.  */
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

	pushl	%esp
	call	*BASE_TRAP_HANDLER
	popl	%edx

	/* acknowledge the interrupt */
	movb	$0x20,%al
	cmpl	$8,EXT(gdb_pc_com_irq)
	jg	1f
	outb	%al,$0xa0	/* XXX fully special nested mode?? */
1:	outb	%al,$0x20
	
	popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        popa
        addl    $4*2,%esp       /* Pop trap number and error code */
        iret
	

        .bss

ENTRY(gdb_pc_com_irq)
	.space	4
