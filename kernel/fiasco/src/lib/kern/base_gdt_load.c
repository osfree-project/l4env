/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the name IBM not be used in advertising or publicity 
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

#include <flux/x86/seg.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/base_vm.h>
#include <flux/x86/base_gdt.h>

void base_gdt_load(void)
{
	struct pseudo_descriptor pdesc;

	/* Create a pseudo-descriptor describing the GDT.  */
	pdesc.limit = sizeof(base_gdt) - 1;
	pdesc.linear_base = kvtolin(&base_gdt);

	/* Load it into the CPU.  */
	set_gdt(&pdesc);

	/*
	 * Reload all the segment registers from the new GDT.
	 */
	asm volatile("ljmp  %0,$1f ;  1:" : : "i" (KERNEL_CS));
	set_ds(KERNEL_DS);
	set_es(KERNEL_DS);
	set_ss(KERNEL_DS);

	/*
	 * Clear out the FS and GS registers by default,
	 * since they're not needed for normal execution of GCC code.
	 */
	set_fs(0);
	set_gs(0);
}

