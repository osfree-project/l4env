#ifndef CONFIG_GDT_H
#define CONFIG_GDT_H

#include "globalconfig.h"

// We want to be Hazelnut compatible, because we want to use the same
// sysenter ipc bindings. When doing sysenter they push 0x1b and the
// returning eip. The 0x1b value is used for small address spaces where
// the code segment has to be reloadeded after a sysexit was performed
// (sysexit loads a flat 4GB segment into cs and ss).
//
// Further, the order of GDT_CODE_KERNEL, GDT_DATA_KERNEL, GDT_CODE_USER,
// GDT_DATA_USER is important since the sysenter/sysexit instruction pair
// supposes that layout (see Intel Reference Manual about sysenter/sysexit)
#define GDT_CODE_KERNEL		(0x08)
#define GDT_DATA_KERNEL		(0x10)
#define GDT_CODE_USER		(0x18)
#define GDT_DATA_USER		(0x20)
#define GDT_TSS			(0x28)		// hardware task segment
#define GDT_TSS_DBF		(0x30)		// tss for double fault handler

#ifdef CONFIG_ABI_V4
#define GDT_UTCB_ADDRESS	(0x38)		// segment for UTCB pointer
#define GDT_MAX			(0x40)
#else
#define GDT_MAX			(0x38)
#endif

#endif

