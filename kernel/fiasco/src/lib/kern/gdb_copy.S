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

/*
 * Copy between kernel address space and a GDB buffer,
 * detecting and recovering from any invalid accesses that occur.
 * Since we are merely copying in the same address space (the kernel),
 * both the gdb_copyin and gdb_copyout routines can be the same.
 *
 * arg0:	source address
 * arg1:	destination address
 * arg2:	byte count
 */
ENTRY(gdb_copyin)
ENTRY(gdb_copyout)
	pushl	%esi
	pushl	%edi			/* save registers */

	movl	8+S_ARG0,%esi		/* get user start address */
	movl	8+S_ARG1,%edi		/* get kernel destination address */
	movl	8+S_ARG2,%edx		/* get count */

	movl	$copy_fail,EXT(gdb_trap_recover)

	/*cld*/				/* count up: default in all GCC code */
	movl	%edx,%ecx		/* move by longwords first */
	shrl	$2,%ecx
	rep
	movsl				/* move longwords */
	movl	%edx,%ecx		/* now move remaining bytes */
	andl	$3,%ecx
	rep
	movsb

	xorl	%eax,%eax		/* return 0 for success */

copy_ret:
	movl	$0,EXT(gdb_trap_recover)

	popl	%edi			/* restore registers */
	popl	%esi
	ret				/* and return */

copy_fail:
	movl	$-1,%eax		/* return -1 for failure */
	jmp	copy_ret		/* pop frame and return */


