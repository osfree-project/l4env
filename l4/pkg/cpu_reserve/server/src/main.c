/*!
 * \file   cpu_reserve/server/src/main.c
 * \brief  CPU reservation daemon
 *
 * \date   09/04/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <stdio.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/ktrace.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/irq.h>
#include <l4/sys/rt_sched.h>
#include <l4/names/libnames.h>
#include <l4/cpu_reserve/sched.h>
#include "cpu_reserve-server.h"
#include <l4/dm_mem/dm_mem.h>
#include <l4/thread/thread.h>
#include "sched.h"
#include "granularity.h"
#include "watch.h"
#include "monitor.h"

static int verbose;
int watch_verbose;
l4_ssize_t l4libc_heapsize = 64*1024;
l4_threadid_t main_id;

int l4cpu_reserve_add_component(l4_threadid_t *caller,
				const l4_threadid_t *thread,
				const char *name,
				int prio,
				int period,
				int *wcet,
				int deadline,
				int *id_p,
				CORBA_Server_Environment *env){
    int pos, check;
    sched_t *sched;
    const char*ncopy;
    int err=0, id=1;

    LOGd(verbose, l4util_idfmt " \"%s\": p=%d T=%d C=%d D=%d",
	 l4util_idstr(*thread), name, prio, period, *wcet, deadline);

    *wcet = granularity_roundup(*wcet);
    if(period==0) return -L4_EINVAL;
    if(sched_cur_threads == sched_max_threads) return -L4_ENOMEM;
    /* do we habe other timeslices of the same thread, maybe even
     *  with a different period? */
    for(pos=0;pos<sched_cur_threads;pos++){
	if(!is_dp(scheds[pos]) &&
	   l4_thread_equal(scheds[pos]->thread, *thread)){
	    if(scheds[pos]->period != period) return -L4_EINVAL;
	    id++;
	}
    }

    if((sched = (sched_t*)calloc(1,sizeof(sched_t)))==0){
	return -L4_ENOMEM;
    }
    if((ncopy = (const char*)malloc(strlen(name)+1))==0){
	return -L4_ENOMEM;
    }
    memcpy((char*)ncopy, name, strlen(name)+1);

    *sched = (sched_t){name:ncopy, thread:*thread, id:id,
		       creator:*caller,
		       prio:prio, period: period,
                       wcet:*wcet, deadline:deadline,
		       watcher:L4THREAD_INVALID_ID};
    pos = sched_index(prio);
    if(deadline){
	int w;
	w = sched_response_time(sched, pos, 0);
        if(verbose) printf("  reserved wcet: %d, response_time: %d\n",
			   *wcet, w);
        if(w<0){
	    err = -L4_ETIME;
	    goto e_sched;
	}
    }

    for(check=0; check<sched_cur_threads; check++){
	if(scheds[check]->prio > prio) break;

        /* check that all threads of prio p meet their deadline */
        if(!is_dp(scheds[check]) && scheds[check]->deadline){
            int p2 = sched_index(scheds[check]->prio);
            int w = sched_response_time(scheds[check], p2, sched);
            if(verbose) printf(
		"  ->" l4util_idfmt "/%d \"%s\" p=%d T=%d C=%d D=%d: time=%d\n",
		l4util_idstr(scheds[check]->thread),
		scheds[check]->id,
		scheds[check]->name,
		scheds[check]->prio, scheds[check]->period,
		scheds[check]->wcet, scheds[check]->deadline, w);
            if(w<0){
		err = -L4_ETIME;
		goto e_sched;
	    }
        }
    }

    /* and do the reservation at the kernel */
    if((err = l4_rt_add_timeslice(*thread, prio, *wcet))!=0){
	LOG_Error("l4_rt_add_timeslice("l4util_idfmt", p=%d C=%d)=%d",
		  l4util_idstr(*thread), prio, *wcet, err);
	err = -L4_EPERM;
	goto e_sched;
    }

    if(id==1){
	// set period
	if(verbose) printf("  setting period at kernel to %d\n", period);
	l4_rt_set_period (*thread, period);
    }

    lock_scheds();
    /* insert into scheds */
    memmove(scheds+pos+1, scheds+pos,
	    sizeof(sched_t*)*(sched_cur_threads-pos));
    sched_cur_threads++;
    scheds[pos]=sched;
    unlock_scheds();
    *id_p = id;
    return 0;

  e_sched:
    free((char*)sched->name);
    free(sched);
    return err;
}

int l4cpu_reserve_delayed_preempt_component(l4_threadid_t *caller,
					    const l4_threadid_t *thread,
					    int id,
					    int prio,	/* ignored */
					    int *delay,
					    CORBA_Server_Environment *env){
    sched_t *sched, *dp;
    char*ncopy;
    int len, pos, err;

    LOGd(verbose, l4util_idfmt ": id=%d C=%d",
	 l4util_idstr(*thread), id, *delay);
    /* Current kernel does not support DP anyway, and DP is handled at
     * client only. Thus, granularity is ideal. */
    
    if(sched_cur_threads == sched_max_threads) return -L4_ENOMEM;
    /* thread present, but not DP? */
    sched=0;
    for(pos=0;pos<sched_cur_threads;pos++){
	if(!l4_thread_equal(scheds[pos]->thread, *thread) ||
	   id!=scheds[pos]->id) continue;
	if(is_dp(scheds[pos])) return -L4_EINVAL;
	sched=scheds[pos];
    }
    if(sched==0 && id) return -L4_EINVAL;

    if((dp = (sched_t*)calloc(1,sizeof(sched_t)))==0){
	return -L4_ENOMEM;
    }
    if(sched) len = strlen(sched->name);
    else len=0;
    if((ncopy = (char*)malloc(len+1+strlen(".dp")))==0){
	return -L4_ENOMEM;
    }
    if(sched)memcpy(ncopy, sched->name, len);
    memcpy(ncopy+len, ".dp", strlen(".dp")+1);

    *dp = (sched_t){name:ncopy, thread:*thread, id:id,
		    creator:*caller,
		    prio:sched?sched->prio:0, period: 0,
		    wcet:*delay, deadline:0,
		    watcher:L4THREAD_INVALID_ID};

    for(pos=0; pos<sched_cur_threads; pos++){
        if(!is_dp(scheds[pos]) && scheds[pos]->deadline){
            int p2 = sched_index(scheds[pos]->prio);
            int w = sched_response_time(scheds[pos], p2, dp);
            if(verbose) printf(
		"  ->" l4util_idfmt "/%d \"%s\" p=%d T=%d C=%d D=%d: time=%d\n",
		l4util_idstr(scheds[pos]->thread),
		scheds[pos]->id,
		scheds[pos]->name,
		scheds[pos]->prio, scheds[pos]->period,
		scheds[pos]->wcet, scheds[pos]->deadline, w);
            if(w<0){
		err = -L4_ETIME;
		goto e_dp;
	    }
        }
    }

    /* insert into scheds */
    pos = sched_index(dp->prio);
    lock_scheds();
    memmove(scheds+pos+1, scheds+pos,
	    sizeof(sched_t*)*(sched_cur_threads-pos));
    sched_cur_threads++;
    scheds[pos]=dp;
    unlock_scheds();
    return 0;

  e_dp:
    free((char*)dp->name);
    free(dp);
    return err;


}

int l4cpu_reserve_change_component(l4_threadid_t *caller,
				   const l4_threadid_t *thread,
				   int id,
				   int new_prio,
				   int *new_wcet,
				   int new_deadline,
				   CORBA_Server_Environment *env){
    sched_t *sched;
    int pos, err=0;
    int check_local=0, check_others=0;
    sched_t sched_copy;

    LOGd(verbose, l4util_idfmt ": p=%d C=%d D=%d",
	 l4util_idstr(*thread), new_prio, *new_wcet, new_deadline);
    if(*new_wcet>=0) *new_wcet = granularity_roundup(*new_wcet);
    /* find the sched context */
    for(pos=0;pos<sched_cur_threads;pos++){
	if(!is_dp(scheds[pos]) &&
	   l4_thread_equal(scheds[pos]->thread, *thread) &&
	   scheds[pos]->id == id){
	    break;
	}
    }
    if(pos==sched_cur_threads) return -L4_ENOTFOUND;

    sched = scheds[pos];
    memcpy(&sched_copy, sched, sizeof(sched_t));

    /* what can happen:
     * prio increases -> local times will be ok, check all others
     *			 below or equal new prio
     * prio decreases -> check local times and all below or equal new
     * wcet increases -> check local and all below or equal
     * wcet decreases -> ok
     * dl   increases -> ok
     * dl   decreases -> check local
     */
    if(new_prio>=0){
	sched->prio = new_prio;
	check_others=1;
	if(new_prio < sched_copy.prio) check_local=1;
    }
    if(*new_wcet>=0){
	sched->wcet = *new_wcet;
	if(*new_wcet > sched_copy.wcet){
	    check_others = check_local = 1;
	}
    }
    if(new_deadline>=0){
	sched->deadline = new_deadline;
	if(!sched_copy.deadline || (new_deadline &&
				    new_deadline < sched_copy.deadline)){
	    check_local=1;
	}
    }
    /* if deadline is not "optional" now, check it */
    if(check_local && new_deadline){
	int w, i;
	i = sched_index(sched->prio);
	/* we changed sched it in-place, thus there is no new context */
	w = sched_response_time(sched, i, 0);
        if(verbose) printf("  reserved wcet: %d, response_time: %d\n",
			   sched->wcet, w);
	if(w<0){
	    err = -L4_ETIME;
	    goto e_back;
	}
    }
    if(check_others){
	int i;

	for(i=0; i<sched_cur_threads; i++){
	    int p2, w;

	    if(scheds[i]->prio > sched->prio) break;
	    if(is_dp(scheds[i]) ||
	       scheds[i]==sched ||
	       scheds[i]->deadline==0 ) continue;

	    p2 = sched_index(scheds[i]->prio);
	    /* we changed sched in-place, thus there is no new context */
	    w = sched_response_time(scheds[i], p2, 0);
	    if(verbose)printf(
		"  ->" l4util_idfmt "/%d \"%s\" p=%d T=%d C=%d D=%d: time=%d\n",
		l4util_idstr(scheds[i]->thread),
		scheds[i]->id,
		scheds[i]->name,
		scheds[i]->prio, scheds[i]->period,
		scheds[i]->wcet, scheds[i]->deadline, w);
	    if(w<0){
		err = -L4_ETIME;
		goto e_back;
	    }
	}
    }
    /* all checks passed. Change at kernel, remove the old sched,
     * insert the new.
     */
    if(l4_rt_change_timeslice(sched->thread, sched->id,
			      sched->prio, sched->wcet)){
	err = -L4_EPERM;
	goto e_back;
    }
    /* if prio changed, remove sched at entry pos */
    if(new_prio>=0){
	int new_pos;

	lock_scheds();
	sched_cur_threads--;
	/* remove the old */
	if(pos!=sched_cur_threads){
	    memmove(scheds+pos, scheds+pos+1,
		    sizeof(sched_t*)*(sched_cur_threads-pos));
	}
	/* insert the new */
	new_pos = sched_index(new_prio);
	memmove(scheds+new_pos+1, scheds+new_pos,
		sizeof(sched_t*)*(sched_cur_threads-new_pos));
	sched_cur_threads++;
	scheds[new_pos]=sched;
	unlock_scheds();
    }
    /* and finally return */
    return 0;

  e_back:
    memcpy(sched, &sched_copy, sizeof(sched_t));
    return err;
}



int l4cpu_reserve_delete_thread_component(l4_threadid_t *caller,
					  const l4_threadid_t *thread,
					  CORBA_Server_Environment *env){
    int i, err;

    LOGd(verbose, l4util_idfmt, l4util_idstr(*thread));

    /* lookup thread to see if we know it */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_thread_equal(scheds[i]->thread, *thread)) break;
    }
    if(i==sched_cur_threads) return -L4_ENOTFOUND;

    /* kill the reservation at fiasco */
    err = l4_rt_remove(*thread);
    if(err){
	LOG_Error("l4_rt_remove("l4util_idfmt")=%d",
		  l4util_idstr(*thread), err);
	return -L4_EPERM;
    }

    /* prepare the watcher threads to be killed, w/o holding a lock */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_thread_equal(scheds[i]->thread, *thread)){
	    sched_prepare_free(i);
	}
    }
    
    lock_scheds();
    /* remove the entries from our database */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_thread_equal(scheds[i]->thread, *thread)){
	    sched_free(i);
	    i--;
	}
    }
    unlock_scheds();
    return 0;
}

int l4cpu_reserve_delete_task_component(l4_threadid_t *caller,
					const l4_threadid_t *task,
					CORBA_Server_Environment *env){
    int i, found=0;

    LOGd(verbose, l4util_idfmt, l4util_idstr(*task));

    /* prepare the watcher threads to be killed, w/o holding a lock */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_task_equal(scheds[i]->thread, *task)){
	    sched_prepare_free(i);
	}
    }
    lock_scheds();
    for(i=0;i<sched_cur_threads;i++){
	if(l4_task_equal(scheds[i]->thread, *task)){
	    /* caution: the following may fail. This is bad, as
	     * inconsistencies may occur. */
	    l4_rt_remove(scheds[i]->thread);
	    sched_free(i);
	    i--;
	    found=1;
	}
    }
    unlock_scheds();
    return found?0:-L4_ENOTFOUND;
}

int l4cpu_reserve_begin_strictly_periodic_component(
    l4_threadid_t *caller,
    const l4_threadid_t *thread,
    l4_kernel_clock_t clock,
    CORBA_Server_Environment *env){

    int err;

    LOGd(verbose, l4util_idfmt " at clock=%lld", l4util_idstr(*thread), clock);

    //l4_sleep(10);
    LOGk("start periodic("l4util_idfmt",%lld)...",
	 l4util_idstr(*thread),clock);
    err = l4_rt_begin_strictly_periodic(*thread, clock);
    LOGk("periodic("l4util_idfmt")=%d", l4util_idstr(*thread), err);
    if(err){
	LOG_Error("l4_rt_begin_strictly_periodic("l4util_idfmt") failed",
		  l4util_idstr(*thread));
    }
    return err?-L4_EPERM:0;
}

int l4cpu_reserve_begin_minimal_periodic_component(
    l4_threadid_t *caller,
    const l4_threadid_t *thread,
    l4_kernel_clock_t clock,
    CORBA_Server_Environment *env){

    int err;

    LOGd(verbose, l4util_idfmt " at clock=%lld", l4util_idstr(*thread), clock);

    err = l4_rt_begin_minimal_periodic(*thread, clock);
    return err?-L4_EPERM:0;
}

int l4cpu_reserve_end_periodic_component(l4_threadid_t *caller,
					 const l4_threadid_t *thread,
					 CORBA_Server_Environment *env){

    int err;

    LOGd(verbose, l4util_idfmt, l4util_idstr(*thread));

    err = l4_rt_end_periodic(*thread);
    return err?-L4_EPERM:0;
}

int l4cpu_reserve_watch_component(l4_threadid_t *caller,
				  const l4_threadid_t *thread,
				  l4dm_dataspace_t *ds,
				  l4_threadid_t *preempter,
				  CORBA_Server_Environment *env){
    int i;

    LOGd(verbose,
	 "thread="l4util_idfmt, l4util_idstr(*thread));
    /* find the thread-id. */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_thread_equal(scheds[i]->thread, *thread) &&
	   scheds[i]->id == 1 &&
	   !is_dp(scheds[i])){
	    void *addr;
	    int err;

	    /* we have the entry. We watch a thread only once. */
	    if(scheds[i]->watch) return -L4_EBUSY;

	    if((addr = l4dm_mem_ds_allocate_named(L4_PAGESIZE, 0, "watch",
						  ds))==0)
		return -L4_EINVAL;
	    memset(addr, 0, L4_PAGESIZE);
	    /* share with caller */
	    if((err = l4dm_share(ds, *caller, L4DM_RW))!=0){
		goto e_mem;
	    }

	    /* generate monitor event, if monitoring is enabled */
	    if(monitor_enable && (err = monitor_start(scheds[i], thread))!=0){
		goto e_mem;
	    }
	    
	    scheds[i]->watcher_end_sem = L4SEMAPHORE_LOCKED;
	    if((err = l4thread_create_named(watcher_fn,
					    ".watcher",
					    &scheds[i]->watcher_end_sem,
					    L4THREAD_CREATE_ASYNC))<0){
		l4dm_mem_release(addr);
		return err;
	    }
	    scheds[i]->watcher = err;

	    /* set preempter */
	    *preempter = l4thread_l4_id(scheds[i]->watcher);
	    scheds[i]->watch = (unsigned*)addr;
	    
	    return 0;

	  e_mem:
	    l4dm_mem_release(addr);
	    return err;
	}
    }
    return -L4_ENOTFOUND;

}

int l4cpu_reserve_scheds_count_component(l4_threadid_t *caller,
					 CORBA_Server_Environment *env){
    LOGd_Enter(verbose, "returning %d", sched_cur_threads);
    return sched_cur_threads;
}

int l4cpu_reserve_scheds_get_component(l4_threadid_t *caller,
				       int idx,
				       char **name,
				       l4_threadid_t *thread,
				       l4_threadid_t *creator,
				       int *id,
				       int *prio,
				       int *period,
				       int *wcet,
				       int *deadline,
				       CORBA_Server_Environment *env){
    if(idx>=sched_cur_threads || idx<0) return -L4_EINVAL;
    *name	= (char*)scheds[idx]->name;
    *thread	= scheds[idx]->thread;
    *creator	= scheds[idx]->creator;
    *id		= scheds[idx]->id;
    *prio	= scheds[idx]->prio;
    *wcet	= scheds[idx]->wcet;
    *period	= scheds[idx]->period;
    *deadline	= scheds[idx]->deadline;
    return 0;
}

int l4cpu_reserve_time_demand_component(l4_threadid_t *caller,
					const l4_threadid_t *thread,
					int id,
					CORBA_Server_Environment *env){
    int i;

    /* find the thread-id. */
    for(i=0;i<sched_cur_threads;i++){
	if(l4_thread_equal(scheds[i]->thread, *thread) &&
	   scheds[i]->id==id && !is_dp(scheds[i])){
	    int delay;
	    delay = sched_response_time(scheds[i], i, 0);
	    return delay>=0?delay:-L4_ETIME;
	}
    }
    return -L4_EINVAL;
}

int main(int argc, const char**argv){
    int err;

    main_id = l4_myself();
    if(parse_cmdline(&argc, &argv,
                     't', "threads", "max number of threads to manage",
                     PARSE_CMD_INT, 1000, &sched_max_threads,
                     'v', "verbose", "verbose mode",
                     PARSE_CMD_SWITCH, 1, &verbose,
                     'w', "watch", "verbose: log watch preemptions",
                     PARSE_CMD_SWITCH, 1, &watch_verbose,
		     'm', "monitor", "monitor preemptions to rt_mon",
		     PARSE_CMD_SWITCH, 1, &monitor_enable,
                     0,0)) return 1;
    if((err = sched_init())!=0){
        l4env_perror("sched_init()", -err);
        return 1;
    }

    if(names_register(l4cpu_reserve_name)==0){
        LOG_Error("names_register failed");
        return 1;
    }
    printf("Scheduling granularity: %dµs\n", granularity());
    l4cpu_reserve_server_loop(0);
}
