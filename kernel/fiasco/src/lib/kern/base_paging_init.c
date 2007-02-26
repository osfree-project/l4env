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
 * Code to set up a basic paged memory environment on a PC.
 * Creates a single page directory, base_pdir,
 * which direct-maps all physical memory up to phys_mem_max (see phys_lmm.h).
 */

#include <flux/x86/proc_reg.h>
#include <flux/x86/base_cpu.h>
#include <flux/x86/base_paging.h>
#include <flux/x86/pc/phys_lmm.h>
#include <panic.h>


vm_offset_t base_pdir_pa;

void base_paging_init(void)
{
	if (ptab_alloc(&base_pdir_pa))
		panic("Can't allocate kernel page directory");

	if (pdir_map_range(base_pdir_pa, 0, 0, round_superpage(phys_mem_max),
		INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_USER))
		panic("Can't direct-map physical memory");

	/* Enable superpage support if we have it.  */
	if (base_cpuid.feature_flags & CPUF_4MB_PAGES)
	{
		set_cr4(get_cr4() | CR4_PSE);
	}

	/* Turn on paging.  */
	paging_enable(base_pdir_pa);
}

