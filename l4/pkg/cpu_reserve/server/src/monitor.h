/*!
 * \file   cpu_reserve/server/src/monitor.h
 * \brief  Prototypes for rt-mon interface functions
 *
 * \date   11/12/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __CPU_RESERVE_SERVER_SRC_MONITOR_H_
#define __CPU_RESERVE_SERVER_SRC_MONITOR_H_
#include <l4/sys/types.h>
#include <l4/rt_mon/histogram.h>
#include "sched.h"

int monitor_enable;
extern int monitor_start(const sched_t *sched, const l4_threadid_t *thread);
extern void monitor_thread_dl(l4_threadid_t *thread, l4_cpu_time_t time);

#endif
