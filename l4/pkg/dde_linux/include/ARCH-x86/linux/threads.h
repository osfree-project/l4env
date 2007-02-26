/* Mostly copied from Linux. See the DDE_LINUX define for diffs */

#ifndef _LINUX_THREADS_H
#define _LINUX_THREADS_H

#include <linux/config.h>

/*
 * The default limit for the nr of threads is now in
 * /proc/sys/kernel/threads-max.
 */
 
#ifdef CONFIG_SMP
#ifndef DDE_LINUX
#define NR_CPUS	32		/* Max processors that can be running in SMP */
#else /* DDE_LINUX */
/* XXX maximum could be max number of lthreads */
#define NR_CPUS	32
#endif /* DDE_LINUX */
#else
#define NR_CPUS 1
#endif

#define MIN_THREADS_LEFT_FOR_ROOT 4

/*
 * This controls the maximum pid allocated to a process
 */
#define PID_MAX 0x8000

#endif
