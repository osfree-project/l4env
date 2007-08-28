/*!
 * \file   cpu_reserve/server/src/watch.c
 * \brief  Preemption watcher
 *
 * \date   09/18/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/rt_sched.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/rt_mon/histogram.h>
#include <l4/env/errno.h>
#include "cpu_reserve-client.h"
#include "sched.h"
#include "watch.h"
#include "monitor.h"

extern int watch_verbose;

/*!\brief Compare threads, ignoring chief, nest and site
 */
static inline int thread_equal(l4_threadid_t a, l4_threadid_t b){
    return a.id.lthread == b.id.lthread &&
	a.id.task == b.id.task;
}

/*!\brief Will be called by our own watcher thread on timeslice overrrun
 */
static int watch_hit_ts(const l4_threadid_t *thread, int id){
    int i;

    lock_scheds();
    /* find the thread-id. */
    for(i=0;i<sched_cur_threads;i++){
	if(thread_equal(scheds[i]->thread, *thread) &&
	   scheds[i]->id == 1 &&
	   !is_dp(scheds[i])){
	    if(scheds[i]->watch) scheds[i]->watch[id]++;
	    unlock_scheds();
	    return 0;
	}
    }
    unlock_scheds();
    return -L4_ENOTFOUND;
}

void watcher_fn(void*arg){
    l4_rt_preemption_t dw;
    l4_msgdope_t result;
    l4_threadid_t thread;
    l4semaphore_t *end_sem = (l4semaphore_t*)arg;

    LOGdL(watch_verbose, "Waiting for Preemption-IPCs");

    while (1) {
        // wait for preemption IPC
        if (l4_ipc_wait(&thread,
			L4_IPC_SHORT_MSG, (l4_umword_t*)&dw.lh.low, (l4_umword_t*)&dw.lh.high,
			L4_IPC_NEVER, &result) == 0){

	    if(l4_thread_equal(thread, main_id)){
		/* we should stop */
		l4semaphore_up(end_sem);
		l4_sleep_forever();
	    }
	    switch(dw.p.type){
	    case L4_RT_PREEMPT_TIMESLICE:
		LOGd(watch_verbose, "TS "l4util_idfmt"/%d",
		     l4util_idstr(thread), dw.p.id);
		watch_hit_ts(&thread, dw.p.id);
		break;
	    case L4_RT_PREEMPT_DEADLINE: {
		LOGd(watch_verbose, "DL "l4util_idfmt, l4util_idstr(thread));
		if(monitor_enable){
		    int err;
		    l4_threadid_t next;
		    l4_cpu_time_t time;
		    l4_umword_t prio;

		    if((err = fiasco_get_cputime(thread, &next,
						 &time, &prio))!=0){
			LOG_Error("fiasco_get_cputime()");
			break;
		    }

		    monitor_thread_dl(&thread, time);
		}
		break;
	    }
            }
        } else
	    LOG("Preempt-receive returned %lx", L4_IPC_ERROR(result));
    }
}
