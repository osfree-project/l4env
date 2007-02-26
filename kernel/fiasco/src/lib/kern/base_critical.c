/* 
 * Copyright (c) 1996 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * Simple "global critical section" implementation for panic, debugging, etc.
 * This implementation is for uniprocessors; for multiprocessors,
 * the SMP startup code in libsmp.a overrides it with a different version.
 */

#include <flux/x86/proc_reg.h>

static unsigned entry_count;
static unsigned saved_eflags;

void base_critical_enter(void)
{
	if (entry_count == 0) {
		saved_eflags = get_eflags();
		cli();
	}
	entry_count++;
}

void base_critical_leave(void)
{
	entry_count--;
	if (entry_count == 0) {
		set_eflags(saved_eflags);
	}
}

