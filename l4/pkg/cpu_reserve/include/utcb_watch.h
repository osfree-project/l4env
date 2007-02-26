/*!
 * \file   cpu_reserve/include/utcb_watch.h
 * \brief  Watch threads using the ring-buffer in their UTCB
 *
 * \date   01/12/2005
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * Using a special Fiasco extension, scheduling events of threads will
 * be written into the UTCBs of those threads. Using an additional polling
 * thread, the data will be collected and written into rt_mon histogramms.
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __CPU_RESERVE_INCLUDE_UTCB_WATCH_H_
#define __CPU_RESERVE_INCLUDE_UTCB_WATCH_H_

#include <l4/cpu_reserve/utcb_stat.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

#define L4CPU_RESERVE_UTCB_NAME_LEN 32

/*!\brief Initialize watching threads using the utcb rings
 *
 * \param poll_interval		poll interval, in microseconds
 *
 * \retval 0			OK, error else
 */
int l4cpu_reserve_utcb_watch_init(int polltime);

/*!\brief Add a thread to the watched threads
 *
 * \param  thread		thread to be watched
 * \param  utcb			pointer to the utcb of that thread
 * \param  name			name of the thread/watch entity
 * \param  ts_count		maximum number of timeslices to watch
 *				(incl. the be-timeslice)
 * \retval 0			OK, error else
 */
int l4cpu_reserve_utcb_watch_add(l4_threadid_t thread,
				 l4cpu_reserve_utcb_t *utcb,
				 const char*name,
				 unsigned maxtime,
				 int ts_count);

/*!\brief Stop watching of a thread
 *
 * \param  thread		thread that is not to be watched any longer
 *
 * \retval 0			OK, error else
 */
int l4cpu_reserve_utcb_watch_del(const l4cpu_reserve_utcb_t *utcb);

EXTERN_C_END
#endif
