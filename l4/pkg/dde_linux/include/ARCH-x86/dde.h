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
#include <l4/sys/compiler.h>
#include <l4/omega0/client.h>

#include <l4/dde_linux/ctor.h>

/** Configuration for Linux header file usage */
#ifndef DDE_LINUX
# define DDE_LINUX
#endif
#ifndef __KERNEL__
# define __KERNEL__
#endif

EXTERN_C_BEGIN

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

/** Return amount of free memory.
 */
int l4dde_mm_kmem_avail(void);

/** Return begin and end of kmem regions
 */
int l4dde_mm_kmem_region(unsigned num, l4_addr_t *start, l4_addr_t *end);

/** Address conversion region addition.
 *
 * \param  va    virtual start address
 * \param  pa    physical start address
 * \param  size  region size
 */
void l4dde_add_region(l4_addr_t va, l4_addr_t pa, l4_size_t size);

/** Address conversion region removal.
 *
 * \param  va    virtual start address
 * \param  size  region size
 */
void l4dde_remove_region(l4_addr_t va, l4_size_t size);

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

/** Get IRQ thread number
 * \ingroup mod_irq
 *
 * \param  irq          IRQ number the irq-thread used with request_irq()
 * \return thread-id    threadid of IRQ thread, or L4_INVALID_ID if not
 *                      initialized
 */
l4_threadid_t l4dde_irq_l4_id(int irq);

/** Initialize IRQ handling.
 *
 * \return 0 on success, negative error code otherwise
 */
int l4dde_irq_init(int omega0);

/** Set an alien IPC handler for interrupt requests
 *
 * \param  handler      new handler, or 0 to disable it.
 *
 * \retval -1           special case: Called prior to dde-lib-initialization
 * \return old value, 0 initially.
 *
 * \pre  DDE lib must be initialized already.
 * \note This call influences future interrupt wait calls. Ongoing IRQ waits
 *       are closed waits and won't receive alien IPCs.
 */
omega0_alien_handler_t l4dde_set_alien_handler(omega0_alien_handler_t handler);

/** Set a deferred irq handler
 *
 * \param  irq          IRQ to set deferred handler for, must be allocated
 *                      with request_irq() already.
 * \param  def_handler  handler to call after standard handler
 * \param  dev_def_id   additional argument to def_handler calls
 *
 * On occurance of an interrupt, the deferred handler is called after
 * the standard handler(s), this is after the registered linux handlers.
 */
extern int l4dde_set_deferred_irq_handler(unsigned int irq,
                                          void (*def_handler) (int, void *),
                                          void *dev_def_id);

int l4dde_pci_init(void);

int l4dde_keventd_init(void);

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

/** DDE2.6: Initialize driver classes
 *
 * \return 0 on success; negative error code otherwise
 *
 * This function is only implemented in DDE 2.6 (and above).
 */
int l4dde_driver_classes_init(void);

EXTERN_C_END
#endif

