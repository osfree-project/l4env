/*
 * Remote GDB debugging on x86 machines for the Flux OS Toolkit
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
/*
 * This file contains the x86-specific code for remote GDB debugging.
 * When a trap occurs, we take a standard trap saved state frame
 * (defined in flux/x86/base_trap.h),
 * and convert it into the gdb_state structure that GDB wants.
 * We also convert the x86 trap number into an appropriate signal number.
 *
 * This code can interface with different remote GDB stubs,
 * e.g. for the serial versus Ethernet protocols.
 * The 'gdb_stub' function pointer is used to call the correct stub.
 */

#include <flux/signal.h>
#include <flux/x86/trap.h>
#include <flux/x86/base_trap.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/gdb.h>

void (*gdb_signal)(int *inout_signo, struct gdb_state *inout_state);

unsigned gdb_trap_recover;

int gdb_trap(struct trap_state *ts)
{
	struct gdb_state r;
	int signo, orig_signo;

	if (gdb_signal == 0)
		return -1;

	/* If a recovery address has been set,
	   use it and return immediately.  */
	if (gdb_trap_recover)
	{
		ts->eip = gdb_trap_recover;
		return 0;
	}

	/* Convert the x86 trap code into a generic signal number.  */
	/* XXX some of these are probably not really right.  */
	switch (ts->trapno)
	{
		case -1 : signo = SIGINT; break; /* hardware interrupt */
		case 0 : signo = SIGFPE; break; /* divide by zero */
		case 1 : signo = SIGTRAP; break; /* debug exception */
		case 3 : signo = SIGTRAP; break; /* breakpoint */
		case 4 : signo = 16; break; /* into instruction (overflow) */
		case 5 : signo = 16; break; /* bound instruction */
		case 6 : signo = SIGILL; break; /* Invalid opcode */
		case 7 : signo = SIGFPE; break; /* coprocessor not available */
		case 8 : signo = 7; break; /* double fault */
		case 9 : signo = SIGSEGV; break; /* coprocessor segment overrun */
		case 10 : signo = SIGSEGV; break; /* Invalid TSS */
		case 11 : signo = SIGSEGV; break; /* Segment not present */
		case 12 : signo = SIGSEGV; break; /* stack exception */
		case 13 : signo = SIGSEGV; break; /* general protection */
		case 14 : signo = SIGSEGV; break; /* page fault */
		case 16 : signo = 7; break; /* coprocessor error */
		default: signo = 7;         /* "software generated"*/
	}
	orig_signo = signo;

	/* Convert the trap state into GDB's format.  */
	r.gs = ts->gs;
	r.fs = ts->fs;
	r.es = ts->es;
	r.ds = ts->ds;
	r.edi = ts->edi;
	r.esi = ts->esi;
	r.ebp = ts->ebp;
	r.ebx = ts->ebx;
	r.edx = ts->edx;
	r.ecx = ts->ecx;
	r.eax = ts->eax;
	r.eip = ts->eip;
	r.cs = ts->cs;
	r.eflags = ts->eflags;
	if (ts->cs & 3)
	{
		/* Came from user mode:
		   stack pointer is saved in the trap frame.  */
		r.esp = ts->esp;
		r.ss = ts->ss;
	}
	else
	{
		/* Came from kernel mode: we didn't switch stacks,
		   so stack pointer is ts+TR_KSIZE.  */
		r.esp = (unsigned)&ts->esp;
		r.ss = get_ss();
	}

	/* Call the appropriate GDB stub to do its thing.  */
	gdb_signal(&signo, &r);

	/* Stuff GDB's modified state into our trap_state.  */
	ts->gs = r.gs;
	ts->fs = r.fs;
	ts->es = r.es;
	ts->ds = r.ds;
	ts->edi = r.edi;
	ts->esi = r.esi;
	ts->ebp = r.ebp;
	ts->ebx = r.ebx;
	ts->edx = r.edx;
	ts->ecx = r.ecx;
	ts->eax = r.eax;
	ts->eip = r.eip;
	ts->cs = r.cs;
	ts->eflags = r.eflags;
	if (r.cs & 3)
	{
		ts->esp = r.esp;
		ts->ss = r.ss;
	}
	else
	{
		/* XXX currently we don't know how to change the kernel esp.
		   We could do it by physically moving the trap frame.  */
	}

	/* If GDB sent us back a signal number,
	   convert that back into a trap number.  */
	if (signo != 0)
	{
		/* If the signal number was unchanged from what we sent,
		   leave the trap number unchanged as well.  */
		if (signo == orig_signo)
			return -1;

		/* Otherwise, try to guess an appropriate trap number.
		   We can't do much, but try to get the common ones.
		   If we can't make a decent guess,
		   just leave the trap number unchanged.  */
		switch (signo)
		{
			case SIGFPE:	ts->trapno = T_DIVIDE_ERROR; break;
			case SIGTRAP:	ts->trapno = T_DEBUG; break;
			case SIGILL:	ts->trapno = T_INVALID_OPCODE; break;
			case SIGSEGV:	ts->trapno = T_GENERAL_PROTECTION;
					break;
		}

		return -1;
	}

	/* GDB consumed the signal - just resume execution normally.  */
	return 0;
}

