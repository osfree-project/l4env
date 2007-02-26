#ifndef CONFIG_GDT_H
#define CONFIG_GDT_H

#include "globalconfig.h"

// We want to be Hazelnut compatible, because we want to use the same
// sysenter IPC bindings. When doing sysenter they push 0x1b and the
// returning EIP. The 0x1b value is used for small address spaces where
// the code segment has to be reloaded after a sysexit was performed
// (sysexit loads a flat 4GB segment into CS and SS).
//
// Furthermore, the order of GDT_CODE_KERNEL, GDT_DATA_KERNEL, GDT_CODE_USER,
// GDT_DATA_USER is important since the sysenter/sysexit instruction pair
// assumes this layout (see Intel Reference Manual about sysenter/sysexit)

#define GDT_CODE_KERNEL		(0x08)		// #1
#define GDT_DATA_KERNEL		(0x10)		// #2

#if defined(CONFIG_IA32)

#define GDT_CODE_USER		(0x18)		// #3
#define GDT_DATA_USER		(0x20)		// #4
#define GDT_TSS			(0x28)		// #5: hardware task segment
#define GDT_TSS_DBF		(0x30)		// #6: tss for dbf handler

#endif

#if defined(CONFIG_AMD64)

#define GDT_DATA_USER		(0x18)		// #3
#define GDT_CODE_USER		(0x20)		// #4
#define GDT_TSS			(0x28)		// #5 hardware task segment

#endif

#define GDT_LDT			(0x38)		// #7

#if defined(CONFIG_EXCEPTION_IPC)

#define GDT_UTCB		(0x40)		// #8 segment for UTCB pointer
#define GDT_TLS1		(0x48)		// #9
#define GDT_TLS2		(0x50)		// #10
#define GDT_TLS3		(0x58)		// #11
#define GDT_MAX			(0x60)

#else

#define GDT_UTCB		GDT_DATA_USER	// dummy
#define GDT_TLS1		(0x00)		// dummy
#define GDT_TLS2		(0x00)		// dummy
#define GDT_TLS3		(0x00)		// dummy
#define GDT_MAX			(0x40)

#endif

#endif // CONFIG_GDT_H
