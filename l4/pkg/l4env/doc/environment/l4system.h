/***********************************************************
 *
 * File: system.h
 *
 ***********************************************************/

#ifndef __SYSTEM_SYSTEM_H__
#define __SYSTEM_SYSTEM_H__

/*
 * kernel implementations
 */
#define L4_KERNEL_L4				1
#define L4_KERNEL_L4KA				2
#define L4_KERNEL_L4MIPS			3
#define L4_KERNEL_L4ALPHA			4
#define L4_KERNEL_FIASCO			5

/*
 * kernel interface
 */
#define L4_KERNEL_INTERFACE_V2		1
#define L4_KERNEL_INTERFACE_X0		2
#define L4_KERNEL_INTERFACE_X1		3
#define L4_KERNEL_INTERFACE_V4		4
#define L4_KERNEL_INTERFACE_V5		5
#define L4_KERNEL_INTERFACE_UNKNOWN	0xffffffff

/* 
 * page sizes supported by platform 
 */
#define L4_SYS_PAGESIZE_1K	(1 << 10)
#define L4_SYS_PAGESIZE_4K	(1 << 12)
#define L4_SYS_PAGESIZE_8K	(1 << 13)
#define L4_SYS_PAGESIZE_4M	(1 << 22)

/*
 * architectures
 */
#define L4_PROC_ARCH_IA32		0x10000
#define L4_PROC_ARCH_IA64		0x20000
#define L4_PROC_ARCH_MIPS		0x30000
#define L4_PROC_ARCH_ALPHA		0x40000
#define L4_PROC_ARCH_SHX		0x50000
#define L4_PROC_ARCH_ARM		0x60000
#define L4_PROC_ARCH_PPC		0x70000
#define L4_PROC_ARCH_M68K		0x80000
#define L4_PROC_ARCH_UNKNOWN	        0xffff0000

#define L4_PROC_ARCH_MASK		0xffff0000
#define PROCESSOR_ARCH(x)		((x) & L4_PROC_ARCH_MASK)

/*
 * processors
 */
// Intel IA32
#define L4_PROCESSOR_INTEL_386		0x10003
#define L4_PROCESSOR_INTEL_486		0x10004
#define L4_PROCESSOR_INTEL_586		0x10005
#define L4_PROCESSOR_INTEL_686		0x10006
// Intel IA64
#define L4_PROCESSOR_INTEL_ITANIUM	0x20001u
// Mips
#define L4_PROCESSOR_MIPS_R3000		0x30003
#define L4_PROCESSOR_MIPS_R4000		0x30004
#define L4_PROCESSOR_MIPS_R10000	0x3000A
// Alpha
#define L4_PROCESSOR_ALPHA_21064	0x40001
#define L4_PROCESSOR_ALPHA_21164	0x40002
#define L4_PROCESSOR_ALPHA_21264	0x40003
// Hitachi Super H
#define L4_PROCESSOR_HITACHI_SH3	0x50001
#define L4_PROCESSOR_HITACHI_SH3E	0x50002
#define L4_PROCESSOR_HITACHI_SH4	0x50003
// Arm
#define L4_PROCESSOR_ARM_720		0x60001
#define L4_PROCESSOR_ARM_820		0x60002
#define L4_PROCESSOR_ARM_920		0x60003
#define L4_PROCESSOR_ARM_STRONGARM	0x60004
// PowerPC
#define L4_PROCESSOR_PPC_601		0x70001
#define L4_PROCESSOR_PPC_602		0x70002
#define L4_PROCESSOR_PPC_604		0x70003
#define L4_PROCESSOR_PPC_620		0x70004
// Motorola 68K
#define L4_PROCESSOR_M68K_68000		0x80001
#define L4_PROCESSOR_M68K_DRAGBALL	0x80002

/*
 * cache architecture
 */
#define L4_CACHE_NO_CACHE			0
#define L4_CACHE_CONS_SHARE			1
#define L4_CACHE_CONS_SMP			2
#define L4_CACHE_CONS_BUS			3


typedef struct {
	/* kernel identification */
	l4_umword_t		kernel;
	l4_umword_t		kernel_interface;
	l4_umword_t		kernel_version;

	/* processor identification */
	l4_umword_t		processor;
	l4_umword_t		cpu_frequency;
	l4_umword_t		bus_frequency;

	/* smp systems */
	l4_umword_t		number_cpus;

	/* virtual memory sizes */
	qword_t		mem_page_sizes;
	qword_t		mem_virt_sugg_max;
	qword_t		mem_virt_max;

	/* cache */
	l4_umword_t		cache_type;
	l4_umword_t		cache_size_l1;
	l4_umword_t		cache_size_l2;
	l4_umword_t		cache_size_l3;
} system_info_t;


/*
 * include architecture definition files
 */
#undef L4_SYS_ARCH

#ifdef(ARCH_X86)
# define ARCH_IA32
#endif

#if defined(ARCH_IA32)
# include "ia32.h"
#endif

#if defined(ARCH_IA64)
# include "ia64.h"
#endif

#if defined(ARCH_MIPS)
# include "mips.h"
#endif

#if defined(ARCH_ALPHA)
# include "alpha.h"
#endif

#if defined(ARCH_SHX)
# include "shx.h"
#endif

#if defined(ARCH_ARM)
# include "arm.h"
#endif

#if defined(ARCH_PPC)
# include "ppc.h"
#endif

#if defined(ARCH_M68K)
# include "m68k.h"
#endif



#endif __SYSTEM_SYSTEM_H__
