/*
 *  asm-i386/acpi.h
 *
 *  Copyright (C) 2001 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2001 Patrick Mochel <mochel@osdl.org>
  *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef _ASM_ACPI_H
#define _ASM_ACPI_H

#ifdef __KERNEL__

#include <linux/init.h>

#define COMPILER_DEPENDENT_INT64   long long
#define COMPILER_DEPENDENT_UINT64  unsigned long long

/*
 * Calling conventions:
 *
 * ACPI_SYSTEM_XFACE        - Interfaces to host OS (handlers, threads)
 * ACPI_EXTERNAL_XFACE      - External ACPI interfaces 
 * ACPI_INTERNAL_XFACE      - Internal ACPI interfaces
 * ACPI_INTERNAL_VAR_XFACE  - Internal variable-parameter list interfaces
 */
#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

/* Asm macros */

#define ACPI_ASM_MACROS
#define BREAKPOINT3
#define ACPI_DISABLE_IRQS() __cli()
#define ACPI_ENABLE_IRQS()  __sti()
#define ACPI_FLUSH_CPU_CACHE()	wbinvd()


static inline int
__acpi_acquire_global_lock (unsigned int *lock)
{
	unsigned int old, new_, val;
	do {
		old = *lock;
		new_ = (((old & ~0x3) + 2) + ((old >> 1) & 0x1));
		val = cmpxchg(lock, old, new_);
	} while (unlikely (val != old));
	return (new_ < 3) ? -1 : 0;
}

static inline int
__acpi_release_global_lock (unsigned int *lock)
{
	unsigned int old, new_, val;
	do {
		old = *lock;
		new_ = old & ~0x3;
		val = cmpxchg(lock, old, new_);
	} while (unlikely (val != old));
	return old & 0x1;
}

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
	((Acq) = __acpi_acquire_global_lock((unsigned int *) GLptr))

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
	((Acq) = __acpi_release_global_lock((unsigned int *) GLptr))

/*
 * Math helper asm macros
 */
#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) \
        asm("divl %2;"        \
        :"=a"(q32), "=d"(r32) \
        :"r"(d32),            \
        "0"(n_lo), "1"(n_hi))


#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) \
    asm("shrl   $1,%2;"             \
        "rcrl   $1,%3;"             \
        :"=r"(n_hi), "=r"(n_lo)     \
        :"0"(n_hi), "1"(n_lo))

#ifdef CONFIG_ACPI_PCI
extern int acpi_noirq;
extern int acpi_pci_disabled;
static inline void acpi_noirq_set(void) { acpi_noirq = 1; }
static inline void acpi_disable_pci(void) 
{
	acpi_pci_disabled = 1; 
	acpi_noirq_set();
}
extern int acpi_irq_balance_set(char *str);
#else
static inline void acpi_noirq_set(void) { }
static inline void acpi_disable_pci(void) { acpi_noirq_set(); }
static inline int acpi_irq_balance_set(char *str) { return 0; }
#endif

#ifdef CONFIG_ACPI_BOOT 
extern int acpi_lapic;
extern int acpi_ioapic;
extern int acpi_strict;
extern int acpi_disabled;
extern int acpi_ht;
extern int acpi_skip_timer_override;
void __init check_acpi_pci(void);
static inline void disable_acpi(void) 
{ 
	acpi_disabled = 1;
	acpi_ht = 0;
	acpi_disable_pci();
}

/* Fixmap pages to reserve for ACPI boot-time tables (see fixmap.h) */
#define FIX_ACPI_PAGES 4

#else	/* !CONFIG_ACPI_BOOT */
#define acpi_lapic 0
#define acpi_ioapic 0
#endif	/* !CONFIG_ACPI_BOOT */

#ifdef CONFIG_ACPI_SLEEP

extern unsigned long saved_eip;
extern unsigned long saved_esp;
extern unsigned long saved_ebp;
extern unsigned long saved_ebx;
extern unsigned long saved_esi;
extern unsigned long saved_edi;

static inline void acpi_save_register_state(unsigned long return_point)
{
	saved_eip = return_point;
	asm volatile ("movl %%esp,(%0)" : "=m" (saved_esp));
	asm volatile ("movl %%ebp,(%0)" : "=m" (saved_ebp));
	asm volatile ("movl %%ebx,(%0)" : "=m" (saved_ebx));
	asm volatile ("movl %%edi,(%0)" : "=m" (saved_edi));
	asm volatile ("movl %%esi,(%0)" : "=m" (saved_esi));
}

#define acpi_restore_register_state()   do {} while (0)


/* routines for saving/restoring kernel state */
extern int acpi_save_state_mem(void);
extern int acpi_save_state_disk(void);
extern void acpi_restore_state_mem(void);

extern unsigned long acpi_wakeup_address;

extern void do_suspend_lowlevel_s4bios(int resume);

/* early initialization routine */
extern void acpi_reserve_bootmem(void);

#endif /*CONFIG_ACPI_SLEEP*/


#endif /*__KERNEL__*/

#endif /*_ASM_ACPI_H*/
