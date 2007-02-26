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

#include <flux/x86/eflags.h>
#include <flux/x86/gdb.h>

void gdb_set_trace_flag(int trace_enable, struct gdb_state *state)
{
	if (trace_enable)
		state->eflags |= EFL_TF;
	else
		state->eflags &= ~EFL_TF;
}

