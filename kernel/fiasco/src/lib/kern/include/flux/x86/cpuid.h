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
 * CPU identification for x86 processors.
 */
#ifndef _FLUX_X86_CPUID_H_
#define _FLUX_X86_CPUID_H_

#include <cdefs.h>

struct cpu_info
{
	unsigned 	stepping	: 4;
	unsigned 	model		: 4;
	unsigned 	family		: 4;
	unsigned	type		: 2;
	unsigned	reserved	: 18;
	unsigned	feature_flags;
	char		vendor_id[12];
	unsigned char	cache_config[16];
};

/*
 * Values of the cpu_info.family field
 */
#define CPU_FAMILY_386		3
#define CPU_FAMILY_486		4
#define CPU_FAMILY_PENTIUM	5
#define CPU_FAMILY_PENTIUM_PRO	6

/*
 * Values of the cpu_info.type field
 */
#define CPU_TYPE_ORIGINAL	0
#define CPU_TYPE_OVERDRIVE	1
#define CPU_TYPE_DUAL		2

/*
 * CPU feature_flags bit definitions.
 */
#define CPUF_ON_CHIP_FPU	0x00000001	/* On-chip floating point */
#define CPUF_VM86_EXT		0x00000002	/* Virtual mode extensions */
#define CPUF_IO_BKPTS		0x00000004	/* I/O breakpoint support */
#define CPUF_4MB_PAGES		0x00000008	/* 4MB page support */
#define CPUF_TS_COUNTER		0x00000010	/* Timestamp counter */
#define CPUF_PENTIUM_MSR	0x00000020	/* Pentium model-spec regs */
#define CPUF_PAGE_ADDR_EXT	0x00000040	/* Page address extensions */
#define CPUF_MACHINE_CHECK_EXCP	0x00000080	/* Machine check exception */
#define CPUF_CMPXCHG8B		0x00000100	/* CMPXCHG8B instruction */
#define CPUF_LOCAL_APIC		0x00000200	/* CPU contains a local APIC */
#define CPUF_MEM_RANGE_REGS	0x00001000	/* memory type range regs */
#define CPUF_PAGE_GLOBAL_EXT	0x00002000	/* page global extensions */
#define CPUF_MACHINE_CHECK_ARCH	0x00004000	/* Machine check architecture */
#define CPUF_CMOVCC		0x00008000	/* CMOVcc instructions */

/*
 * Cache configuration descriptor entry values.
 */
#define CPUC_NULL			0x00
#define CPUC_CODE_TLB_4K_4W_64E		0x01
#define CPUC_CODE_TLB_4M_4W_4E		0x02
#define CPUC_DATA_TLB_4K_4W_64E		0x03
#define CPUC_DATA_TLB_4M_4W_8E		0x04
#define CPUC_CODE_L1_32B_4W_8KB		0x06
#define CPUC_DATA_L1_32B_2W_8KB		0x0a
#define CPUC_COMB_L2_32B_4W_128KB	0x41
#define CPUC_COMB_L2_32B_4W_256KB	0x42
#define CPUC_COMB_L2_32B_4W_512KB	0x43


__BEGIN_DECLS
/*
 * Generic routine to detect the current processor
 * and fill in the supplied cpu_info structure with all information available.
 * If the vendor ID cannot be determined, it is left a zero-length string.
 * This routine assumes the processor is at least a 386 -
 * since it's a 32-bit function, it wouldn't run on anything less anyway.
 */
void cpuid(struct cpu_info *out_id);

__END_DECLS

#endif /* _FLUX_X86_CPUID_H_ */
