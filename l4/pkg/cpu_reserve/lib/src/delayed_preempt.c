/*!
 * \file   cpu_reserve/lib/src/delayed_preempt.c
 * \brief  Delayed Preemption Emulation
 *
 * \date   09/22/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <stdio.h>
#include <l4/sys/kdebug.h>
#include <l4/util/rdtsc.h>
#include <l4/util/atomic.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/sys/rt_sched.h>

/*!\brief Log delayed preemption calls */
#define CONFIG_LOG_DP 0

/*!\brief Log timeouts */
#define CONFIG_LOG_TIMEOUT 1

/*!\brief Log timeouts to Fiasco tracebuffer (1) or Log (0) */
#define CONFIG_LOG_TO_KERN 1


static int dp_duration;				/* microseconds */
static volatile l4_int32_t preempt_counter;
static volatile l4_cpu_time_t preempt_start;	/* 0 if none started */

/* for debugging purposes: */
static l4thread_t thread=L4THREAD_INVALID_ID;
static void*pc;

int l4cpu_dp_reserve_task(int *duration){
    int err;

    if(dp_duration || !*duration) return -L4_EINVAL;
    if((err = l4cpu_reserve_delayed_preempt(l4_myself(),
					    0,	/* id */
					    0,	/* prio */
					    duration))!=0){
	return err;
    }
    dp_duration = *duration;
    return 0;
}

int l4cpu_dp_active(void){
    return preempt_counter;
}

int l4cpu_dp_begin(void){
    /* We need to cli only if nobody owns the lock. */
    if(thread == L4THREAD_INVALID_ID){
	LOGdk(CONFIG_LOG_DP, "cli()");
	l4_rt_dp_begin();
    } else {
	LOGdk(CONFIG_LOG_DP,"cli'd by %x", thread);
    }
    if(l4util_inc32_res((l4_uint32_t*)&preempt_counter)==1){
	LOGdk(CONFIG_LOG_DP, "delayed_preempt: started");
	if(l4cpu_dp_start_callback) l4cpu_dp_start_callback();

	preempt_start = l4_tsc_to_us(l4_rdtsc());
	thread = l4thread_myself();
	pc = __builtin_return_address(0);
	return 0;
    }

    /* preemption was already started. However, we allow multiple
     * starts from the same thread due to the spin_lock implementation */
    LOGdk(CONFIG_LOG_DP,
	  "delayed_preempt: ++ctr=%d", preempt_counter);
    if(thread == l4thread_myself()) return 0;
    else {
	char buf[10];
	sprintf(buf, "already started by: %x PC=%p", thread, pc);
	outstring(buf);
	enter_kdebug("preemption started twice");
	return -L4_EINVAL;
    }
}

int l4cpu_dp_end(void){
    int err;

    if((err=l4util_dec32_res((l4_uint32_t*)&preempt_counter))==0){
	l4_cpu_time_t diff;

	LOGdk(CONFIG_LOG_DP, "delayed_preempt: done now");

	diff = l4_tsc_to_us(l4_rdtsc()) - preempt_start;
	if(l4cpu_dp_stop_callback){
	    l4cpu_dp_stop_callback(diff, pc);
	}

	if(diff > dp_duration){
	    if(CONFIG_LOG_TO_KERN){
		LOGdk(CONFIG_LOG_DP, "DP timeout: %uus/%uus.",
		      (unsigned)diff, dp_duration);
	    } else {
		LOGdl(CONFIG_LOG_DP,
		      "Preemption lasted too long: %uµs, requested was %uµs.",
		      (unsigned)diff,
		      dp_duration);
	    }
	    err = -L4_ETIME;
	    goto e_return;
	}
	err = dp_duration-diff;
	goto e_return;
    }
    LOGdk(CONFIG_LOG_DP, "delayed_preempt: --ctr=%d", err);
    if(err<0){
	enter_kdebug("preemption not started");
	err = -L4_EINVAL;
	goto e_return;
    }
    /* still someone in the preemption, just return */
    return 0;

  e_return:
    thread = L4THREAD_INVALID_ID;
    pc=0;
    LOGdk(CONFIG_LOG_DP, "sti()");
    l4_rt_dp_end();
    return err;
}
