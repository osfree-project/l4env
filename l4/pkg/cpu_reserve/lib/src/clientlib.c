/*!
 * \file   cpu_reserve/lib/src/clientlib.c
 * \brief  Client bindings for the CPU reservation server
 *
 * \date   09/06/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/env/errno.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/rt_sched.h>
#include <l4/names/libnames.h>
#include <stdlib.h>
#include "cpu_reserve-client.h"

static l4_threadid_t server_id = L4_INVALID_ID;

typedef void* (*malloc_t)(unsigned long);

#define my_default_environment \
  { { _corba: { major: CORBA_NO_EXCEPTION, repos_id: 0} }, \
      { param: 0}, L4_IPC_NEVER_INITIALIZER, \
      { fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, \
      (malloc_t)malloc, free }

static void l4cpu_reserve_set_preempter(l4_threadid_t thread,
					l4_threadid_t preempter){
    l4_threadid_t pager;
    l4_umword_t dummy;

    pager = L4_INVALID_ID;
    l4_thread_ex_regs_flags(thread, -1, -1, &preempter, &pager,
		            &dummy, &dummy, &dummy,
                            L4_THREAD_EX_REGS_NO_CANCEL);
}

static int server_init(void){
    if(!l4_is_invalid_id(server_id)) return 0;

    return names_waitfor_name(l4cpu_reserve_name,
			      &server_id, 5000)?0:-L4_EIPC;
}

int l4cpu_reserve_add(l4_threadid_t thread,
		      const char*name,
		      int prio,
		      int period,
		      int *wcet,
		      int deadline,
		      int *id){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_add_call(&server_id,
				  &thread,
				  name,
				  prio,
				  period,
				  wcet,
				  deadline,
				  id,
				  &env);
}

int l4cpu_reserve_delayed_preempt(l4_threadid_t thread,
				  int id,
				  int prio,
				  int *delay){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_delayed_preempt_call(&server_id,
					      &thread,
					      id,
					      prio,
					      delay,
					      &env);
}

int l4cpu_reserve_change(l4_threadid_t thread,
			 int id,
			 int new_prio,
			 int *new_wcet,
			 int new_deadline){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_change_call(&server_id,
				     &thread,
				     id,
				     new_prio,
				     new_wcet,
				     new_deadline,
				     &env);
}

int l4cpu_reserve_delete_thread(l4_threadid_t thread){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    /* Remove a preempter, as this could be the watcher, wich
     * we want to remove */
    l4cpu_reserve_set_preempter(thread, L4_INVALID_ID);

    return l4cpu_reserve_delete_thread_call(&server_id,
					    &thread,
					    &env);
}

int l4cpu_reserve_delete_task(l4_threadid_t task){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_delete_task_call(&server_id,
					  &task,
					  &env);
}

int l4cpu_reserve_begin_strictly_periodic(l4_threadid_t thread,
					  l4_kernel_clock_t clock){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_begin_strictly_periodic_call(&server_id,
						      &thread,
						      clock,
						      &env);
}

int l4cpu_reserve_begin_strictly_periodic_self(l4_threadid_t thread){
    int err;
    DICE_DECLARE_ENV(env);
    l4_threadid_t s;

    if((err = server_init())!=0) return err;

    if(l4_is_invalid_id(thread)) thread = l4_myself();
    s = l4_next_period_id(server_id);
    return l4cpu_reserve_begin_strictly_periodic_call(&s, &thread, 0, &env);
}

int l4cpu_reserve_begin_strictly_periodic_self_deprecated(
    l4_threadid_t thread, l4_kernel_clock_t clock){
    int err;
    DICE_DECLARE_ENV(env);
    l4_threadid_t s;

    if((err = server_init())!=0) return err;

    if(l4_is_invalid_id(thread)) thread = l4_myself();
    s = l4_next_period_id(server_id);
    return l4cpu_reserve_begin_strictly_periodic_call(&s,
						      &thread,
						      clock,
						      &env);
}

int l4cpu_reserve_begin_minimal_periodic(l4_threadid_t thread,
					 l4_kernel_clock_t clock){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_begin_minimal_periodic_call(&server_id,
						     &thread,
						     clock,
						     &env);
}

int l4cpu_reserve_begin_minimal_periodic_self(l4_threadid_t thread){
    int err;
    DICE_DECLARE_ENV(env);
    l4_threadid_t s;

    if((err = server_init())!=0) return err;

    if(l4_is_invalid_id(thread)) thread = l4_myself();
    s = l4_next_period_id(server_id);
    return l4cpu_reserve_begin_minimal_periodic_call(&s, &thread, 0, &env);
}

int l4cpu_reserve_begin_minimal_periodic_self_deprecated(
    l4_threadid_t thread, l4_kernel_clock_t clock){
    int err;
    DICE_DECLARE_ENV(env);
    l4_threadid_t s;

    if((err = server_init())!=0) return err;

    if(l4_is_invalid_id(thread)) thread = l4_myself();
    s = l4_next_period_id(server_id);
    return l4cpu_reserve_begin_minimal_periodic_call(&s,
						     &thread,
						     clock,
						     &env);
}

int l4cpu_reserve_end_periodic(l4_threadid_t thread){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_end_periodic_call(&server_id,
					   &thread,
					   &env);
}

int l4cpu_reserve_watch(l4_threadid_t thread,
			unsigned **addr){
    int err;
    DICE_DECLARE_ENV(env);
    l4dm_dataspace_t ds;
    l4_threadid_t preempter;

    if((err = server_init())!=0) return err;

    if((err = l4cpu_reserve_watch_call(&server_id,
				       &thread,
				       &ds,
				       &preempter,
				       &env))!=0) return err;

    l4cpu_reserve_set_preempter(thread, preempter);

    if(addr!=0){
	if((err = l4rm_attach(&ds, L4_PAGESIZE, 0, L4DM_RW, (void**)addr))!=0){
	    /* the server is watching the thread although we cannot see it */
	    return err;
	}
    }
    return 0;
}


int l4cpu_reserve_scheds_count(void){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_scheds_count_call(&server_id, &env);
}

int l4cpu_reserve_scheds_get(int idx,
			     char **name,
			     l4_threadid_t *thread,
			     l4_threadid_t *creator,
			     int *id,
			     int *prio,
			     int *period,
			     int *wcet,
			     int *deadline){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_scheds_get_call(&server_id,
					 idx,
					 name,
					 thread,
					 creator,
					 id,
					 prio,
					 period,
					 wcet,
					 deadline,
					 &env);
}

int l4cpu_reserve_time_demand(l4_threadid_t thread,
			      int id){
    int err;
    DICE_DECLARE_ENV(env);

    if((err = server_init())!=0) return err;

    return l4cpu_reserve_time_demand_call(&server_id,
					  &thread, id,
					  &env);
}

