/*!
 * \file   cpu_reserve/server/src/sched.h
 * \brief  prototypes for scheduling management
 *
 * \date   09/04/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __CPU_RESERVE_SERVER_SRC_SCHED_H_
#define __CPU_RESERVE_SERVER_SRC_SCHED_H_
#include <l4/sys/types.h>
#include <l4/rt_mon/histogram.h>
#include <l4/semaphore/semaphore.h>

extern l4_threadid_t main_id;

typedef struct sched_t{
    const char*name;
    l4_threadid_t thread;
    l4_threadid_t creator;
    int id;		/* id of reservation */
    int prio;
    int period;			// if 0, delayed preemption
    int wcet;
    int deadline;       	// if 0, don't care
    unsigned *watch;		// array for watching. 0 if unset
    l4thread_t watcher;		// watcher thread, L4THREAD_INVALID if unset
    l4semaphore_t watcher_end_sem; //end-sema for watcher thread
    l4_cpu_time_t dl_old;
    rt_mon_histogram_t *dl_hist;
} sched_t;
extern int sched_cur_threads, sched_max_threads;
extern sched_t **scheds;
extern l4semaphore_t scheds_lock;

extern int sched_init(void);
extern int sched_index(int prio);
extern int sched_response_time(sched_t *sched, int pos, sched_t *new_);
extern int sched_prepare_free(int pos);
extern int sched_free(int pos);
static inline int is_dp(sched_t *sched){
    return sched->period==0;
}

/* Locking is required when modifiying the scheds-array and when accessing
 * it from the watcher threads. */
void lock_scheds(void);
void unlock_scheds(void);

#endif
