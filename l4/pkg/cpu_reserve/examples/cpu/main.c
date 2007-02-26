/*!
 * \file   cpu_reserve/examples/cpu/main.c
 * \brief  Client app to control the cpu_reserved
 *
 * \date   09/04/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/util/parse_cmd.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <stdlib.h>
#include <stdio.h>

l4_ssize_t l4libc_heapsize = 64*1024;

/*!\brief convert a string "xxx.yy" to a threadid
 *
 * string is interpreted in hex
 */
static int a2t(const char*name, l4_threadid_t *t){
    int task, thread;
    char*p;
    
    task = strtol(name, &p, 16);
    if(p==name || *p!='.') return -1;
    p++;
    name=p;
    thread = strtol(name, &p, 16);
    if(*p) return -1;
    if(task>=2048 || thread>=128) return -1;
    *t = L4_NIL_ID;
    t->id.task = task;
    t->id.lthread = thread;
    return 0;
}

static int a2T(const char*name, l4_threadid_t *t){
    int task;
    char*p;
    
    task = strtol(name, &p, 16);
    if(p==name ||*p) return -1;
    if(task>=2048) return -1;
    *t = L4_NIL_ID;
    t->id.task = task;
    return 0;
}

int main(int argc, const char**argv){
    const char*thread_name, *task_name, *change_name, *dp_name;
    int prio, deadline, wcet;
    int err, id, list=0, watch;

    if(parse_cmdline(&argc, &argv,
		     't', "thread", "delete reservation for given thread",
		     PARSE_CMD_STRING, 0, &thread_name,
		     'T', "task", "delete reservation for given task",
		     PARSE_CMD_STRING, 0, &task_name,
		     'c', "change", "change reservation for given thread",
		     PARSE_CMD_STRING, 0, &change_name,
		     'i', "id", "timeslice id (use wich change)",
		     PARSE_CMD_INT, 1, &id,
		     'p', "prio", "new prio (use wich change)",
		     PARSE_CMD_INT, -1, &prio,
		     'D', "deadline", "new deadline (use with change)",
		     PARSE_CMD_INT, -1, &deadline,
		     'C', "wcet", "new wcet (use with change)",
		     PARSE_CMD_INT, -1, &wcet,
		     'd', "dp", "add delayed preemption (thread,id,wcet)",
		     PARSE_CMD_STRING, 0, &dp_name,
		     'l', "list", "list all reservations",
		     PARSE_CMD_SWITCH, 1, &list,
		     'w', "watch", "watch every n seconds",
		     PARSE_CMD_INT, 0, &watch,
		     0, 0)) return 1;

    if(thread_name){
	l4_threadid_t thread_t;
	if(a2t(thread_name, &thread_t)){
	    LOG_Error("invalid thread name: %s", thread_name);
	    return 1;
	}
	LOG("deleting reservations for thread " l4util_idfmt,
	    l4util_idstr(thread_t));
	if((err = l4cpu_reserve_delete_thread(thread_t))!=0){
	    l4env_perror("l4cpu_reserve_delete_thread()", -err);
	    return 1;
	}
    }
    if(task_name){
	l4_threadid_t task_t;
	if(a2T(task_name, &task_t)){
	    LOG_Error("invalid task name: %s", task_name);
	    return 1;
	}
	LOG("deleting reservations for task " l4util_idfmt,
	    l4util_idstr(task_t));
	if((err = l4cpu_reserve_delete_task(task_t))!=0){
	    l4env_perror("l4cpu_reserve_delete_task()", -err);
	    return 1;
	}
    }
    if(change_name){
	l4_threadid_t change_t;
	int C;
	if(a2t(change_name, &change_t)){
	    LOG_Error("invalid thread name: %s", change_name);
	    return 1;
	}
	LOG("changing reservations for thread " l4util_idfmt ", id %d",
	    l4util_idstr(change_t), id);
	if(prio>=0) LOG("  new prio: %d", prio);
	if(deadline>=0) LOG("  new deadline: %d", deadline);
	if(wcet>=0) LOG("  new wcet: %d", wcet);
	
	C=wcet;
	if((err = l4cpu_reserve_change(change_t, id, prio,
				       &C, deadline))!=0){
	    l4env_perror("l4cpu_reserve_change()", -err);
	    return 1;
	}
	if(wcet>=0) LOG("  actual CPU reservation: %d", C);
    }
    if(dp_name){
	l4_threadid_t dp_t;

	if(a2t(dp_name, &dp_t)){
	    LOG_Error("invalid thread name: %s", dp_name);
	    return 1;
	}
	LOG("adding delayed preemption of length %d for "l4util_idfmt":%d",
	    wcet, l4util_idstr(dp_t), id);

	if((err = l4cpu_reserve_delayed_preempt(dp_t, id, prio, &wcet))!=0){
	    l4env_perror("l4cpu_reserve_delayed_preempt()", -err);
	    return 1;
	}
    }

    if(list || watch){
	int i, c;

	do{
	    if((c = l4cpu_reserve_scheds_count())<0){
		l4env_perror("l4cpu_reserve_scheds_count()", -c);
		return 1;
	    }
	    printf("%d registered CPU reservations\n", c);
	    for(i=0; i<c; i++){
		l4_threadid_t t, creator;
		int id, delay;
		char *n;
		int period;
		
		if((err=l4cpu_reserve_scheds_get(i, &n, &t, &creator, &id,
						 &prio, &period, &wcet,
						 &deadline))<0){
		    l4env_perror("l4cpu_reserve_scheds_get(%d)", -err, i);
		    return 1;
		}
		if(period){
		    delay=l4cpu_reserve_time_demand(t, id);
		    if(delay==-L4_ETIME) delay=-1;
		    else if(delay<0) {
			l4env_perror("l4cpu_reserve_time_demand()", -delay);
			return 1;
		    }
		    printf(l4util_idfmt
			   "/%d p=%#4x T=%5d C=%5d D=%5d, delay=%d\n",
			   l4util_idstr(t), id, prio, period, wcet, deadline,
			   delay);
		} else {
		    printf(l4util_idfmt
			   "/%d p=%#4x delayed preemption, C=%5d\n",
			   l4util_idstr(t), id, prio, wcet);
		}
		printf("  name=\"%s\", created by " l4util_idfmt ", nr=%d\n",
		       n, l4util_idstr(creator), i);
		free(n);
	    }
	    l4_sleep(1000*watch);
	}while(watch);
    }
    return 0;
}
