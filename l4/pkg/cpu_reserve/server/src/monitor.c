/*!
 * \file   cpu_reserve/server/src/monitor.c
 * \brief  
 *
 * \date   11/12/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include "monitor.h"
#include "sched.h"

int enable_rt_mon;

static inline int thread_equal(l4_threadid_t a, l4_threadid_t b){
    return a.id.lthread == b.id.lthread &&
	a.id.task == b.id.task;
}

/*!\brief Start the histogramm of dl overflow of a thread
 *
 * \pre scheds-lock should be hold outside
 */
int monitor_start(const sched_t *sched, const l4_threadid_t *thread){
    int i;

    /* find the entry with ID 1, as this determines the maxval and
     * the name of the histogram */
    lock_scheds();
    for(i=0;i<sched_cur_threads;i++){
	if(thread_equal(scheds[i]->thread, *thread) &&
	   scheds[i]->id == 1 && !is_dp(scheds[i])){
	    /* we found the thread. Create the histogramm */

	   char name[RT_MON_NAME_LENGTH];
	   snprintf(name, sizeof(name), "DL: %s", scheds[i]->name);
	    scheds[i]->dl_hist = rt_mon_hist_create(0, scheds[i]->wcet,
						    300,
						    name, "us", "occ.",
						    RT_MON_THREAD_TIME);
	    unlock_scheds();
	    return 0;
	}
    }
    unlock_scheds();
    return -L4_ENOTFOUND;
}


/*!\brief Monitor the consumed time of a thread using the rt_mon lib
 *
 * \param thread	the thread itself
 * \param time		consumed time of the thread so far
 */
void monitor_thread_dl(l4_threadid_t *thread, l4_cpu_time_t time){
    int i;

    //LOG("Period end, thread=" l4util_idfmt, l4util_idstr(*thread));
    /* find the thread */
    lock_scheds();
    for(i=0;i<sched_cur_threads;i++){
	if(thread_equal(scheds[i]->thread, *thread) &&
	   scheds[i]->id == 1 && !is_dp(scheds[i]) && 
	   scheds[i]->dl_hist){
	    /* we found the thread. Update its histogramm */
	    if(scheds[i]->dl_old){
		rt_mon_hist_insert_data(scheds[i]->dl_hist,
					time-scheds[i]->dl_old,1);
	    }
	    scheds[i]->dl_old = time;
	    goto e_back;
	}
    }
  e_back:
    unlock_scheds();
}


