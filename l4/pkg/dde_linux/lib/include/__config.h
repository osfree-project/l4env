/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/include/__config.h
 *
 * \brief	Configuration / Debug Level
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/**
 * \defgroup cfg Compilation Time Configuration
 *
 * Configuration of Linux DDE at compilation time via macros for debugging
 * output level and parameter tuning.
 */

#ifndef _LIBDDE___CONFIG_H
#define _LIBDDE___CONFIG_H

/**
 * \name DEBUG_ Macros
 * \ingroup cfg
 *
 * 0 ... no output
 * 1 ... debugging output for this group
 *
 * @{
 */
#define DEBUG_MALLOC	0	/**< debug memory allocations */
#define DEBUG_PALLOC	1	/**< debug page allocations */
#define DEBUG_ADDRESS	1	/**< debug address conversion */
#define DEBUG_PCI	1	/**< debug PCI support */
#define DEBUG_IRQ	0	/**< debug IRQ handling */
#define DEBUG_RES	0	/**< debug resource handling */
#define DEBUG_RES_TRACE 1	/**< debug function calls in memory/io
				     resource management*/
#define DEBUG_TIMER	0	/**< debug timers */
#define DEBUG_SOFTIRQ	0 	/**< debug softirqs */

#define DEBUG_PROCESS	1 	/**< debug process-level */

#define DEBUG_SOUND 	1	/**< debug sound */
#define DEBUG_SOUND_REG 1	/**< debug sound device registration */
#define DEBUG_SOUND_WRITE 0	/**< debug sound write */
#define DEBUG_SOUND_READ  0	/**< debug sound read */
#define DEBUG_SLAB	0	/**< debug slab functions */

/** @} */

/**
 * \name Configuration Macros
 * \ingroup cfg
 *
 * @{
 */
#define MM_KREGIONS	1	/**< max number of kmem regions */

#define SCHED_YIELD_OPT	1	/**< scheduling method for TASK_RUNNING user
				 * contexts 0 - l4_yield() / 1 - l4_usleep(to) */
#define SCHED_YIELD_TO	10	/**< timeout for scheduling sleep (in us) */

#define SOFTIRQ_THREADS	1	/**< number of desired softirq threads */

#define PCI_DEVICES	12	/**< number of supported PCI devices */

/** @} */

#endif /* !_LIBDDE___CONFIG_H */
