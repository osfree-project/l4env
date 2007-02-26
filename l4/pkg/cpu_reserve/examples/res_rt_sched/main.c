/*!
 * \file   cpu_reserve/examples/res_rt_sched/main.c
 * \brief  example for RT reservation, based on next_res
 *
 * \date   09/06/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/rt_sched.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/timeout.h>
#include <l4/sys/types.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/kip.h>
#include <l4/util/util.h>
#include <l4/util/parse_cmd.h>
#include <l4/rmgr/librmgr.h>
#include <l4/log/l4log.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/env/errno.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>
#include "watch.h"

int show_nextres_error;
int show_left;
int show_change;
int use_cpu_res=1;

l4_threadid_t worker_id	= L4_INVALID_ID;

l4_ssize_t l4libc_heapsize = 64*1024;

int time_mand = 12000;

static void worker(void*arg){
    int loops_mand, loops_opt;
    int loop;
    int err, j;
    double fact=1.1;

    err = l4_rt_next_period();
    if(err){
	l4env_perror("l4_rt_next_period()", err);
    }

    loop = 0;
    loops_mand = 10000000;
    loops_opt  = 3000000;
    while(1) {
	l4_kernel_clock_t left;

        // next period
        err = l4_rt_next_period();

        if (err != 0)
	    l4env_perror("l4_rt_next_period()", err);

        for (j = 0; j < loops_mand; j++)
            ;  // do some work ...

        // next reservation
        err = l4_rt_next_reservation(1, &left);
        if (err != 0)
	    LOGd(show_nextres_error,"next_reservation result = %d", err);
	LOGd(show_left, "mandatory left: %lld micros, err=%d",
	     left, err);

        for (j = 0; j < loops_opt; j++)
            ;  // do some work ...

        loop++;
        if (loop % 4 == 0){
	    // fall back to timesharing timeslice
            err = l4_rt_next_reservation(2, &left);
            if (err != 0){
		LOGd(show_nextres_error,
		     "next_reservation result = %d", err);
	    }
	    LOGd(show_left, "optional left: %lld micros", left);

            printf("Period = %d, mand time %d, opt loops %d, count (%d, %d)\n",
		   loop, time_mand, loops_opt, *count_mand, *count_opt);

	    /* modify the length of the timeslices */
	    if (*count_mand == 0){
		LOGd(show_change,
		     "Decreasing mandatory timeslice from %d to %d",
		     time_mand, (int)(time_mand / fact));
		time_mand /= fact;
	    } else {
		LOGd(show_change,
		     "Increasing mandatory timeslice from %d to %d",
		     time_mand, (int)(time_mand*fact));
		time_mand *= fact;
	    }
	    if(use_cpu_res){
		int wcet = time_mand;
		if((err = l4cpu_reserve_change(worker_id, 1,
					       -1, &wcet, -1))!=0) {
		    l4env_perror("l4cpu_reserve_change()", -err);
		    return;
		}
	    } else {
		if((err = l4_rt_change_timeslice(worker_id,
						 1, 50, time_mand))!=0) {
		    LOG("l4_rt_change_timeslice(): %d", err);
		    return;
		}
	    }

            // adapt optional work load
            if (*count_opt == 0){
		LOGd(show_change,
			 "Increasing opional workload from %d to %d",
			 loops_opt, (int)(loops_opt * 1.01));
                loops_opt *= 1.01;
	    } else {
		LOGd(show_change,
			 "Decreasing optional workload from %d to %d",
			 loops_opt, (int)(loops_opt * .98));
                loops_opt *= 0.98;
	    }

            // reset counters
            *count_mand  = 0;
            *count_opt   = 0;
	}
    }
}


int main (int argc, const char**argv)
{
    int period=150000, word1, ret;
    l4_kernel_info_t *kinfo;
    l4_threadid_t next_period_id;
    l4thread_t worker_t;

    if(parse_cmdline(&argc, &argv,
		     'm', "mandpreempt", "log mandatory preemption ipcs",
		     PARSE_CMD_SWITCH, 1, &show_mand_preempts,
		     'o', "preempt", "log optional preemption ipcs",
		     PARSE_CMD_SWITCH, 1, &show_opt_preempts,
		     'l', "left", "show time left on next-res",
		     PARSE_CMD_SWITCH, 1, &show_left,
		     'e', "nextreserr", "show if next_res() returned error",
		     PARSE_CMD_SWITCH, 1, &show_nextres_error,
		     'c', "change", "show if time/loops are changed",
		     PARSE_CMD_SWITCH, 1, &show_change,
		     'd', "direct", "change scheduling parms directly at the kernel",
		     PARSE_CMD_SWITCH, 0, &use_cpu_res,
		     0, 0)) return 1;

    if(use_cpu_res){
	LOG("Using cpu reservation server");
    }else{
	LOG("Do reservations directly at the kernel");
    }
    kinfo = l4util_kip_map();

    if((worker_t = l4thread_create_named(worker, ".worker",
					 0, L4THREAD_CREATE_ASYNC))<0){
	l4env_perror("l4thread_create()", -worker_t);
	return 1;
    }
    worker_id = l4thread_l4_id(worker_t);
    next_period_id = l4_next_period_id(worker_id);

    /* start to watch our timeslices */

    if(use_cpu_res){
	/* Use cpu reservation server to do the admission, reservation
	 * and watching */
	unsigned *array;
	int wcet;
	
	// add mandatory timeslice
	wcet = time_mand;
	if((ret = l4cpu_reserve_add(worker_id, "mandatory",
				    50, period,
				    &wcet,
				    period, &word1))!=0) {
	    l4env_perror("l4cpu_reserve_add(mandatory)", -ret);
	    return 1;
	}
	
	// add optional mandatory timeslice
	wcet = 5000;
	if((ret = l4cpu_reserve_add(worker_id, "optional",
				    40, period,
				    &wcet,
				    0, &word1))!=0) {
	    l4env_perror("l4cpu_reserve_add(optional)", -ret);
	    return 1;
	}

	/* add watcher */
	if((ret = l4cpu_reserve_watch(worker_id, &array))!=0){
	    l4env_perror("l4cpu_reserve_watch()", -ret);
	    return 1;
	}
	count_mand = array+1;
	count_opt = array+2;
	*count_mand  = 0;
	*count_opt   = 0;

	// commit to periodic work
	if((ret = l4cpu_reserve_begin_strictly_periodic(
		worker_id, kinfo->clock + 20000))!=0) {
	    l4env_perror("l4cpu_reserve_begin_strictly_periodic()", -ret);
	    return 1;
	}
    } else {
	/* Use direct kernel calls to initiate CPU reservation and watching.
	 * No admission process. */

	l4thread_t preempter_t;
	l4_threadid_t preempter;
	l4_threadid_t pager;

	// create preempter
	preempter_t = l4thread_create_named(preempter_thread, ".preempt",
					    &worker_id,
					    L4THREAD_CREATE_SYNC);
	if(preempter_t<0){
	    l4env_perror("l4thread_create()", -preempter_t);
	    return 1;
	}
	preempter = l4thread_l4_id(preempter_t);
	rmgr_set_prio(preempter, 255);
	
	// set preemter
	pager = L4_INVALID_ID;
	l4_thread_ex_regs(worker_id, -1, -1, &preempter, &pager,
			  &word1, &word1, &word1);

	// add mandatory timeslice
	if((ret = l4_rt_add_timeslice(worker_id, 50, time_mand))!=0) {
	    LOG("l4_rt_add_timeslice(): %d", ret);
	    return 1;
	}
	
	// add optional mandatory timeslice
	if((ret = l4_rt_add_timeslice(worker_id, 40, 5000))!=0) {
	    LOG("l4_rt_add_timeslice(): %d", ret);
	    return 1;
	}
	
	// set period
	l4_rt_set_period (worker_id, period);
	
	// commit to periodic work
	if((ret = l4_rt_begin_strictly_periodic(worker_id,
						kinfo->clock + 20000))!=0) {
	    LOG("l4_rt_begin_strictly_periodic(): %d", ret);
	    return 1;
	}
    }
    l4_sleep_forever();
}
