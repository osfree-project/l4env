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

#include <flux/x86/base_cpu.h>
#include <flux/x86/base_paging.h>
#include <assert.h>
//#include <stdio.h>

#define PAGE_SIZE		(1 << 12)
#define SUPERPAGE_SIZE		(1 << 22)
#define SUPERPAGE_MASK		(SUPERPAGE_SIZE - 1)
#define	superpage_aligned(x)	((((vm_offset_t)(x)) & SUPERPAGE_MASK) == 0)

int pdir_map_range(vm_offset_t pdir_pa, vm_offset_t la, vm_offset_t pa,
		   vm_size_t size, Pt_entry mapping_bits)
{
  //  printf("pdir_map_range(%p,%p,%p,%08x,%08x)\n",
  //	 (void*)pdir_pa, (void*)la, (void*)pa, size, mapping_bits);
        assert( la+size-1 > la ); // avoid 4GB wrap around
	assert(mapping_bits & INTEL_PTE_VALID);
	assert(!(mapping_bits & INTEL_PTE_PFN));

	while (size > 0)
	{
		Pd_entry *pde = pdir_find_pde(pdir_pa, la);

		/* Use a 4MB page if we can.  */
		if (superpage_aligned(la) && superpage_aligned(pa)
		    && (size >= SUPERPAGE_SIZE)
		    && (base_cpuid.feature_flags & CPUF_4MB_PAGES))
		{
	                /* a failed assertion here may indicate a memory wrap
                           around problem */
			assert(!(*pde & INTEL_PDE_VALID));
			/* XXX what if an empty page table exists
			   from previous finer-granularity mappings? */
			*pde = pa | mapping_bits | INTEL_PDE_SUPERPAGE;
			la += SUPERPAGE_SIZE;
			pa += SUPERPAGE_SIZE;
			size -= SUPERPAGE_SIZE;
		}
		else
		{
			Pt_entry *pte;

			/* Find the page table, creating one if necessary.  */
			if (!(*pde & INTEL_PDE_VALID))
			{
				vm_offset_t ptab_pa;
				int rc;

				/* Allocate a new page table.  */
				rc = ptab_alloc(&ptab_pa);
				if (rc)
					return rc;

				/* Set the pde to point to it.  */
				*pde = pa_to_pte(ptab_pa)
					| INTEL_PDE_VALID | INTEL_PDE_USER
					| INTEL_PDE_WRITE;
			}
			assert(!(*pde & INTEL_PDE_SUPERPAGE));
			pte = ptab_find_pte(pde_to_pa(*pde), la);

			/* Use normal 4KB page mappings.  */
			do
			{
				assert(!(*pte & INTEL_PTE_VALID));

				/* Insert the mapping.  */
				*pte = pa | mapping_bits;

				/* Advance to the next page.  */
				pte++;
				la += PAGE_SIZE;
				pa += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			while ((size > 0) && !superpage_aligned(la));
		}
	}

	return 0;
}

