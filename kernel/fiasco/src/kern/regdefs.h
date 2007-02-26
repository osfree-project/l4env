#ifndef REGDEFS_H
#define REGDEFS_H

/*
 * Register and register bit definitions for ia32 and UX architectures.
 */

#define CR0_PE		0x00000001	/* Protection Enable		*/
#define CR0_MP		0x00000002	/* Monitor Coprocessor		*/
#define CR0_EM		0x00000004	/* Emulation Coproz.		*/
#define CR0_TS		0x00000008	/* Task Switched		*/
#define CR0_ET		0x00000010	/* Extension Type		*/
#define CR0_NE		0x00000020	/* Numeric Error		*/
#define CR0_WP		0x00010000	/* Write Protect		*/
#define CR0_AM		0x00040000	/* Alignment Mask		*/
#define CR0_NW		0x20000000	/* Not Write-Through		*/
#define CR0_CD		0x40000000	/* Cache Disable		*/
#define CR0_PG		0x80000000	/* Paging 			*/
	
#define CR3_PWT		0x00000008	/* Page-Level Write Transparent	*/
#define CR3_PCD		0x00000010	/* Page-Level Cache Disable	*/

#define CR4_VME		0x00000001	/* Virtual 8086 Mode Extensions	*/
#define CR4_PVI		0x00000002	/* Protected Mode Virtual Ints	*/
#define CR4_TSD		0x00000004	/* Time Stamp Disable		*/
#define CR4_DE		0x00000008	/* Debugging Extensions		*/
#define CR4_PSE		0x00000010	/* Page Size Extensions		*/
#define CR4_PAE		0x00000020	/* Physical Address Extensions	*/
#define CR4_MCE		0x00000040	/* Machine Check Exception	*/
#define CR4_PGE		0x00000080	/* Page Global Enable		*/
#define CR4_PCE		0x00000100	/* Perfmon Counter Enable	*/
#define CR4_OSFXSR	0x00000200	/* OS Supports FXSAVE/FXRSTOR	*/
#define CR4_OSXMMEXCPT	0x00000400	/* OS Supports SIMD Exceptions	*/

#define EFLAGS_CF	0x00000001	/* Carry Flag			*/
#define EFLAGS_PF	0x00000004	/* Parity Flag			*/
#define EFLAGS_AF	0x00000010	/* Adjust Flag			*/
#define EFLAGS_ZF	0x00000040	/* Zero Flag			*/
#define EFLAGS_SF	0x00000080	/* Sign Flag			*/
#define EFLAGS_TF	0x00000100	/* Trap	Flag			*/
#define EFLAGS_IF	0x00000200	/* Interrupt Enable		*/
#define EFLAGS_DF	0x00000400	/* Direction Flag		*/
#define EFLAGS_OF	0x00000800	/* Overflow Flag		*/
#define EFLAGS_IOPL	0x00003000	/* I/O Privilege Level (12+13)	*/
#define EFLAGS_IOPL_K	0x00000000                      /* kernel */
#define EFLAGS_IOPL_U	0x00003000                      /* user */  
#define EFLAGS_NT	0x00004000	/* Nested Task			*/
#define EFLAGS_RF	0x00010000	/* Resume			*/
#define EFLAGS_VM	0x00020000	/* Virtual 8086 Mode		*/
#define EFLAGS_AC	0x00040000	/* Alignment Check		*/
#define EFLAGS_VIF	0x00080000	/* Virtual Interrupt		*/
#define EFLAGS_VIP	0x00100000	/* Virtual Interrupt Pending	*/
#define EFLAGS_ID	0x00200000	/* Identification		*/

/* CPU Feature Flags */
#define FEAT_FPU	0x00000001	/* FPU On Chip			*/
#define FEAT_VME	0x00000002	/* Virt. 8086 Mode Enhancements	*/
#define FEAT_DE		0x00000004	/* Debugging Extensions		*/
#define FEAT_PSE	0x00000008	/* Page Size Extension		*/
#define FEAT_TSC	0x00000010	/* Time Stamp Counter		*/
#define FEAT_MSR	0x00000020	/* Model Specific Registers	*/
#define FEAT_PAE	0x00000040	/* Physical Address Extension	*/
#define FEAT_MCE	0x00000080	/* Machine Check Exception	*/
#define FEAT_CX8	0x00000100	/* CMPXCHG8B Instruction	*/
#define FEAT_APIC	0x00000200	/* APIC On Chip			*/
#define FEAT_SEP	0x00000800	/* Sysenter/Sysexit Present	*/
#define FEAT_MTRR	0x00001000	/* Memory Type Range Registers	*/
#define FEAT_PGE	0x00002000	/* PTE Global Bit Extension	*/
#define FEAT_MCA	0x00004000	/* Machine Check Architecture	*/
#define FEAT_CMOV	0x00008000	/* Conditional Move Instruction */
#define FEAT_PAT	0x00010000	/* Page Attribute Table		*/
#define FEAT_PSE36	0x00020000	/* 32 bit Page Size Extension	*/
#define FEAT_PSN	0x00040000	/* Processor Serial Number	*/
#define FEAT_CLFSH	0x00080000	/* CLFLUSH Instruction		*/
#define FEAT_DS		0x00200000	/* Debug Store			*/
#define FEAT_ACPI	0x00400000	/* Thermal Monitor & Clock	*/
#define FEAT_MMX	0x00800000	/* MMX Technology		*/
#define FEAT_FXSR	0x01000000	/* FXSAVE/FXRSTOR Instructions	*/
#define FEAT_SSE	0x02000000	/* SSE				*/
#define FEAT_SSE2	0x04000000	/* SSE2				*/
#define FEAT_SS		0x08000000	/* Self Snoop			*/
#define FEAT_HTT	0x10000000	/* Hyper-Threading Technology	*/
#define FEAT_TM		0x20000000	/* Thermal Monitor		*/
#define FEAT_PBE	0x80000000	/* Pending Break Enable		*/

/* CPU Extended Feature Flags */
#define FEATX_PNI	0x00000001	/* Prescott New Instructions	*/
#define FEATX_MONITOR	0x00000008	/* MONITOR/MWAIT Support	*/
#define FEATX_DSCPL	0x00000010	/* CPL Qualified Debug Store	*/
#define FEATX_TM2	0x00000100	/* Thermal Monitor 2		*/
#define FEATX_CNXTID	0x00000400	/* L1 Context ID (adapt/shared)	*/

/* Page Fault Error Codes */
/* PF_ERR_REMTADDR and PF_ERR_USERADDR are UX-emulation only */
#define PF_ERR_PRESENT	0x00000001	/* PF: Page Is Present In PTE 	*/
#define PF_ERR_WRITE	0x00000002	/* PF: Page Is Write Protected  */
#define PF_ERR_USERMODE	0x00000004	/* PF: Caused By User Mode Code	*/
#define PF_ERR_RESERVED	0x00000008	/* PF: Reserved Bit Set in PDIR	*/
#define PF_ERR_REMTADDR	0x40000000	/* PF: In Remote Address Space	*/
#define PF_ERR_USERADDR	0x80000000	/* PF: In User Address Space	*/

/* Model Specific Registers */
#define SYSENTER_CS_MSR		0x174	/* Kernel Code Segment		*/
#define SYSENTER_ESP_MSR	0x175	/* Kernel Syscall Entry		*/
#define SYSENTER_EIP_MSR	0x176	/* Kernel Stack Pointer		*/

#define CPU_FAMILY_386          3
#define CPU_FAMILY_486          4
#define CPU_FAMILY_PENTIUM      5
#define CPU_FAMILY_PENTIUM_PRO  6

#endif
