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

#include <flux/x86/proc_reg.h>
#include <flux/x86/base_vm.h>
#include <flux/x86/base_idt.h>

void base_idt_load(void)
{
	struct pseudo_descriptor pdesc;

	/* Create a pseudo-descriptor describing the GDT.  */
	pdesc.limit = sizeof(base_idt) - 1;
	pdesc.linear_base = kvtolin(&base_idt);

	/* Load the IDT.  */
	set_idt(&pdesc);
}
