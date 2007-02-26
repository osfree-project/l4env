/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University.
 * Copyright (c) 1996,1995 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *	Definitions relating to x86 page directories and page tables.
 */
#ifndef	_FLUX_X86_PAGING_H_
#define _FLUX_X86_PAGING_H_

#define INTEL_OFFMASK	0xfff	/* offset within page */
#define PDESHIFT	22	/* page descriptor shift */
#define PDEMASK		0x3ff	/* mask for page descriptor index */
#define PTESHIFT	12	/* page table shift */
#define PTEMASK		0x3ff	/* mask for page table index */

/*
 *	Convert linear offset to page descriptor/page table index
 */
#define lin2pdenum(a)	(((a) >> PDESHIFT) & PDEMASK)
#define lin2ptenum(a)	(((a) >> PTESHIFT) & PTEMASK)

/*
 *	Convert page descriptor/page table index to linear address
 */
#define pdenum2lin(a)	((vm_offset_t)(a) << PDESHIFT)
#define ptenum2lin(a)	((vm_offset_t)(a) << PTESHIFT)

/*
 *	Number of ptes/pdes in a page table/directory.
 */
#define NPTES	(ptoa(1)/sizeof(Pt_entry))
#define NPDES	(ptoa(1)/sizeof(Pt_entry))

/*
 *	Hardware pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */
#define INTEL_PTE_VALID		0x00000001
#define INTEL_PTE_WRITE		0x00000002
#define INTEL_PTE_USER		0x00000004
#define INTEL_PTE_WTHRU		0x00000008
#define INTEL_PTE_NCACHE 	0x00000010
#define INTEL_PTE_REF		0x00000020
#define INTEL_PTE_MOD		0x00000040
#define INTEL_PTE_GLOBAL	0x00000100
#define INTEL_PTE_AVAIL		0x00000e00
#define INTEL_PTE_PFN		0xfffff000

#define INTEL_PDE_VALID		0x00000001
#define INTEL_PDE_WRITE		0x00000002
#define INTEL_PDE_USER		0x00000004
#define INTEL_PDE_WTHRU		0x00000008
#define INTEL_PDE_NCACHE 	0x00000010
#define INTEL_PDE_REF		0x00000020
#define INTEL_PDE_MOD		0x00000040	/* only for superpages */
#define INTEL_PDE_SUPERPAGE	0x00000080
#define INTEL_PDE_GLOBAL	0x00000100	/* only for superpages */
#define INTEL_PDE_AVAIL		0x00000e00
#define INTEL_PDE_PFN		0xfffff000

/*
 *	Macros to translate between page table entry values
 *	and physical addresses.
 */
#define	pa_to_pte(a)		((a) & INTEL_PTE_PFN)
#define	pte_to_pa(p)		((p) & INTEL_PTE_PFN)
#define	pte_increment_pa(p)	((p) += INTEL_OFFMASK+1)

#define	pa_to_pde(a)		((a) & INTEL_PDE_PFN)
#define	pde_to_pa(p)		((p) & INTEL_PDE_PFN)
#define	pde_increment_pa(p)	((p) += INTEL_OFFMASK+1)


#ifndef ASSEMBLER

#include <flux/x86/types.h>
#include <flux/x86/proc_reg.h>

/*
 *	i386/i486/i860 Page Table Entry
 */
typedef unsigned int	Pt_entry;
#define PT_ENTRY_NULL	((Pt_entry *) 0)

typedef unsigned int	Pd_entry;
#define PD_ENTRY_NULL	((Pt_entry *) 0)

/*
 * Read and write the page directory base register (PDBR).
 */
#define set_pdbr(pdir)	set_cr3(pdir)
#define get_pdbr()	get_cr3()

/*
 * Invalidate the entire TLB.
 */
#define inval_tlb()	set_pdbr(get_pdbr())

/*
 * Load page directory 'pdir' and turn paging on.
 * Assumes that 'pdir' equivalently maps the physical memory
 * that contains the currently executing code,
 * the currently loaded GDT and IDT, etc.
 */
extern __inline void paging_enable(vm_offset_t pdir)
{
	/* Load the page directory.  */
	set_cr3(pdir);

	/* Turn on paging.  */
	asm volatile("movl  %0,%%cr0	\n\t"
		     "jmp   1f		\n\t"
		     "1:		\n\t"
		     : : "r" (get_cr0() | CR0_PG));
}

#endif /* !ASSEMBLER */

#endif
