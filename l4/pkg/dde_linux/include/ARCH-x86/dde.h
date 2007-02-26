/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/include/ARCH-x86/dde.h
 * \brief  Linux Device Driver Environment (DDE)
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __DDE_LINUX_INCLUDE_ARCH_X86_DDE_H_
#define __DDE_LINUX_INCLUDE_ARCH_X86_DDE_H_

/* L4 */
#include <l4/sys/types.h>

/** Configuration for Linux header file usage */
#ifndef DDE_LINUX
# define DDE_LINUX
#endif
#ifndef __KERNEL__
# define __KERNEL__
#endif

/** Initialize Memory Management.
 *
 * \param  vmem_size    max size of memory pool for virtual memory allocations
 *                      - vmalloc()
 * \param  kmem_rsize   max size of memory pool for kernel memory allocations
 *                      - kmalloc()
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_mm_init(unsigned int vmem_size, unsigned int kmem_size);

/** Initialize time module.
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_time_init(void);

/** Initialize softirq handling.
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_softirq_init(void);

/** Set prio of installed handler.
 *
 * \param irq   IRQ number the thread requested with request_irq
 * \param prio  the prio to set
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_irq_set_prio(unsigned int irq, unsigned prio);

/** Initialize IRQ handling.
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_irq_init(int omega0);

int l4dde_pci_init(void);

/** Initalize process module
 *
 * \return 0 on success; negative error code otherwise
 */
int l4dde_process_init(void);

/** Add caller as new process level worker
 *
 * \return 0 on success; negative error code otherwise
 *
 * This allocates and initializes a new task_struct for the worker thread.
 */
int l4dde_process_add_worker(void);

/** BUG handler function ptr
 *
 * This function-ptr is called when BUG() is called in Linux code
 */
extern void (*dde_BUG)(const char*file, const char*function, int line) __attribute__((noreturn));

#endif
