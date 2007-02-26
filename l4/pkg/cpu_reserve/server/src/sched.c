/*!
 * \file   cpu_reserve/server/src/sched.c
 * \brief  management of the scheduling data
 *
 * \date   09/04/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <string.h>
#include <stdlib.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include "sched.h"

int sched_cur_threads, sched_max_threads;
sched_t **scheds;
l4semaphore_t scheds_lock = L4SEMAPHORE_UNLOCKED;

/*!\brief initalize arrays
 *
 * \retval 0		OK
 * \retval -L4_ENOMEM	out of memory
 */
int sched_init(void){
    return (scheds = calloc(sizeof(sched_t*),
			    sched_max_threads))!=0?0:-L4_ENOMEM;
}

/*\brief return position where a thread with prio had to be inserted
 *
 * All threads below the returned position have a lower prio,
 * all threads at or above the position have a higher or equal prio.
 *
 * The function returns cur_threads if the given prio is the highest so far.
 */
int sched_index(int prio){
    int pos, min_pos, end_pos;

    if(sched_cur_threads==0) return 0;
    if(prio>scheds[sched_cur_threads-1]->prio) return sched_cur_threads;
    end_pos = sched_cur_threads;      /* all threads >=end_pos have
				       * higher or equal prio */
    min_pos=0;
    pos=0;
    while(1){
        pos = (min_pos+end_pos)/2;
        if(pos==min_pos){
            if(scheds[pos]->prio < prio) return pos+1;
            return pos;
        }
        if(scheds[pos]->prio < prio){
            min_pos = pos;
        } else {
            end_pos = pos;
        }
    }
}

/*!\brief Get max blocking time due to non-preemption for a thread
 *
 * \param prio		prio of the thread to analyze
 * \param new_		optional additional thread, not in #scheds yet
 *
 * \return 		max blocking time due to non-preemption by
 *			lower-prioritized threads
 *
 * This function gets the max blocking time due to non-preemption for
 * a thread of the given priority. Note that only threads with *lower*
 * priority are taken into account, as others do not lead to blocking
 * due to non-preemption but to normal preemption. Normal preemption
 * is handled elsewhere.
 */
static int get_max_dp(int prio, sched_t *new_){
    int i, maxdp=0;

    for(i=0; i<sched_cur_threads; i++){
	
	if(scheds[i]->prio>=prio) break;
	if(is_dp(scheds[i]) && scheds[i]->wcet>maxdp)
	    maxdp = scheds[i]->wcet;
    }
    if(new_ && is_dp(new_) && new_->prio<prio && new_->wcet>maxdp){
	maxdp = new_->wcet;
    }
    return maxdp;
}

/*!\brief Get the response time of the thread s
 *
 * we do response-time analysis based on the wcet of the higher-prioritized
 * threads with (period T, wcet C) (sometimes called time-demand analysis):
 * Iterative proces: 
 *  r_j+1 = sum_i { ceil(r_j/T_i)*C_i }
 * until fixpoint is found
 *
 * \param  sched	contains sched params. If found in scheds, it is
 *			not included again
 * \param  from		consider all threads at and above pos
 * \param  new_         a new thread, that is not in scheds yet, but has
 *                      higher prio than s. maybe 0.
 * \retval >0           response time, <=deadline
 * \retval -1           respones time would be>deadline
 */
int sched_response_time(sched_t *sched, int from, sched_t *new_){
    int r, r_j, pos;

    /* find max delayed preemption due to lower-priorizied threads 
     * and add it to the start value. */
    r = sched->wcet + get_max_dp(sched->prio, new_);
    for(;;){
        r_j = sched->wcet + get_max_dp(sched->prio, new_);
        for(pos=from; pos<sched_cur_threads; pos++){
            if(scheds[pos]==sched) continue;
	    if(!is_dp(scheds[pos])){
		r_j += ( ((r+scheds[pos]->period-1)/scheds[pos]->period)
			 * scheds[pos]->wcet );
	    } else {
		/* DP */
	    }
        }
        if(new_ && !is_dp(new_)){
            r_j += ((r+new_->period-1)/new_->period) * new_->wcet;
        }
	if(r_j > (sched->deadline?sched->deadline:sched->period))
	    return -1;
        if(r_j == r) return r_j;

        r = r_j;
    }
}

/*!\brief Free an element at the given position
 */
int sched_free(int pos){
    sched_t *sched;

    if(pos>=sched_cur_threads) return -L4_EINVAL;
    sched = scheds[pos];
    sched_cur_threads--;
    memmove(scheds+pos, scheds+pos+1,
	    sizeof(sched_t*)*(sched_cur_threads-pos));
    if(sched->watch) l4dm_mem_release(sched->watch);
    if(sched->watcher!=L4THREAD_INVALID_ID){
	l4thread_shutdown(sched->watcher);
    }
    if(sched->dl_hist) rt_mon_hist_free(sched->dl_hist);
    free((char*)sched->name);
    free(sched);
    return 0;
}

/*!\brief Ask the watcher of a sched-entry to stop and wait for its readiness
 */
int sched_prepare_free(int pos){
    sched_t *sched;
    l4_msgdope_t result;

    if(pos>=sched_cur_threads) return -L4_EINVAL;
    sched = scheds[pos];

    if(sched->watcher!=L4THREAD_INVALID_ID){
	/* wake the watcher fn and wait for its readyness to be killed */
	l4_ipc_send(l4thread_l4_id(sched->watcher), L4_IPC_SHORT_MSG, 0, 0,
		    L4_IPC_NEVER, &result);
	l4semaphore_down(&sched->watcher_end_sem);
    }
    return 0;
}

void lock_scheds(void){
    l4semaphore_down(&scheds_lock);
}

void unlock_scheds(void){
    l4semaphore_up(&scheds_lock);
}
