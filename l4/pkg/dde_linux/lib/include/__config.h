/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/include/__config.h
 * \brief  Configuration / Debug Level
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/**
 * \defgroup cfg Compilation Time Configuration
 *
 * Configuration of Linux DDE at compilation time via macros for debugging
 * output level and parameter tuning.
 */

#ifndef __DDE_LINUX_LIB_INCLUDE___CONFIG_H_
#define __DDE_LINUX_LIB_INCLUDE___CONFIG_H_

/**
 * \name DEBUG_ Macros
 * \ingroup cfg
 *
 * 0 ... no output
 * 1 ... debugging output for this group
 *
 * @{
 */
#define DEBUG_ERRORS        1 /**< verbose error handling */
#define DEBUG_MSG           0 /**< show debug messages */
#define DEBUG_MALLOC        0 /**< debug memory allocations */
#define DEBUG_MALLOC_EACH   0 /**< debug msg for each alloc/free */
#define DEBUG_PALLOC        0 /**< debug page allocations */
#define DEBUG_ADDRESS       0 /**< debug address conversion */
#define DEBUG_PCI           0 /**< debug PCI support */
#define DEBUG_IRQ           0 /**< debug IRQ handling */
#define DEBUG_RES           0 /**< debug resource handling */
#define DEBUG_RES_TRACE     0 /**< debug function calls in memory/io
                                   resource management*/
#define DEBUG_TIMER         0 /**< debug timers */
#define DEBUG_SOFTIRQ       0 /**< debug softirqs */

#define DEBUG_PROCESS       0 /**< debug process-level */

#define DEBUG_SOUND         0 /**< debug sound */
#define DEBUG_SOUND_REG     0 /**< debug sound device registration */
#define DEBUG_SOUND_WRITE   0 /**< debug sound write */
#define DEBUG_SOUND_READ    0 /**< debug sound read */

#define DEBUG_SLAB          0 /**< debug slab functions */

/** @} */

/**
 * \name Configuration Macros
 * \ingroup cfg
 *
 * @{
 */
#define MM_KREGIONS         1  /**< max number of kmem regions */

#define SCHED_YIELD_OPT     1  /**< scheduling method for TASK_RUNNING user contexts
                                    0 - l4_yield() / 1 - l4_usleep(to) */
#define SCHED_YIELD_TO      10 /**< timeout for scheduling sleep (in us) */

#define MAX_IRQ_HANDLERS    8  /**< maximal number of handlers for one
                                    interrupt */

#define SOFTIRQ_THREADS     1  /**< number of desired softirq threads 
                                    ONLY one supported for now! */

#define PCI_DEVICES         32 /**< number of supported PCI devices */

/** @} */

#endif
