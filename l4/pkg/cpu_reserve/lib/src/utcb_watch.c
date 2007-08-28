/*!
 * \file   cpu_reserve/lib/src/utcb_watch.c
 * \brief  Timeslice watching using the UTCB Ring-buffer of scheduling events
 *
 * \date   01/12/2005
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include <l4/cpu_reserve/utcb_stat.h>
#include <l4/cpu_reserve/utcb_watch.h>
#include <l4/env/errno.h>
#include <l4/rt_mon/histogram.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>
#include <l4/thread/thread.h>

static void watch_thread(void*arg);

/*!\brief List-element to manage utcbs/threads */
typedef struct utcb_list_t{
    l4_threadid_t thread;	/* thread id */
    l4cpu_reserve_utcb_t *utcb;	/* utcb of that thread */
    int ts_count;		/* max. number of timeslices to watch */
    rt_mon_histogram_t **hists;	/* histogramms attached to the thread */
    unsigned long period;	/* period number, more a debugging aid */
    unsigned long long release;	/* release time of current period */
    int ts_num;			/* last known id of current period */
    char name[L4CPU_RESERVE_UTCB_NAME_LEN]; /* name of the thread-watch */
    struct utcb_list_t *next;	/* for list usage */
} utcb_list_t;

/*!\brief Cmd-type for IPC-communication with watch thread */
typedef enum watch_cmd_val_t {
    WATCH_CMD_INV,
    WATCH_CMD_ADD,
    WATCH_CMD_DEL,
} watch_cmd_t;

/*!\brief argument type for IPC-communication with watch thread */
typedef struct{
    l4_threadid_t thread;
    l4cpu_reserve_utcb_t *utcb;
    char name[L4CPU_RESERVE_UTCB_NAME_LEN]; /* name of the thread-watch */
    unsigned maxtime;
    int ts_count;
} watch_arg_t;

//! the utcbs/threads are kept in this list
static utcb_list_t *utcb_list;

//! thread-id of watcher thread
static l4_threadid_t watch_thread_id;

/****************************************************************************
 *
 * User-interface implementation
 *
 ***************************************************************************/

int l4cpu_reserve_utcb_watch_add(l4_threadid_t thread,
				 l4cpu_reserve_utcb_t *utcb,
				 const char*name,
				 unsigned maxtime,
				 int ts_count){
    l4_umword_t dummy;
    l4_msgdope_t result;
    int err, ans;
    watch_arg_t arg;

    arg.thread = thread;
    arg.utcb = utcb;
    strncpy(arg.name, name, L4CPU_RESERVE_UTCB_NAME_LEN);
    arg.name[L4CPU_RESERVE_UTCB_NAME_LEN-1]=0;
    arg.maxtime = maxtime;
    arg.ts_count = ts_count;

    if((err=l4_ipc_call(watch_thread_id, L4_IPC_SHORT_MSG,
			WATCH_CMD_ADD,
			(l4_umword_t)&arg,
			L4_IPC_SHORT_MSG, (l4_umword_t *)&ans, &dummy,
			L4_IPC_NEVER, &result))!=0) return err;
    return ans;
}

int l4cpu_reserve_utcb_watch_del(const l4cpu_reserve_utcb_t *utcb){
    l4_umword_t dummy;
    l4_msgdope_t result;
    int err, ans;

    if((err=l4_ipc_call(watch_thread_id, L4_IPC_SHORT_MSG,
			WATCH_CMD_DEL,
			(l4_umword_t)utcb,
			L4_IPC_SHORT_MSG, (l4_umword_t *)&ans, &dummy,
			L4_IPC_NEVER, &result))!=0) return err;
    return ans;
}

int l4cpu_reserve_utcb_watch_init(int poll_interval){
    l4thread_t w;

    if((w=l4thread_create_named(watch_thread, ".watcher",
				(void*)poll_interval,
				L4THREAD_CREATE_ASYNC))<0){
	l4env_perror("create watcher", -w);
	return w;
    }
    watch_thread_id = l4thread_l4_id(w);
    return 0;
}

/****************************************************************************
 *
 * Internal implementation at the watcher thread
 *
 ***************************************************************************/

/*!\brief Actually add a new thread to the list of watched threads
 */
static int watch_add_utcb_do(watch_arg_t*arg){
    int err=0, i;
    utcb_list_t *l;

    if(arg->ts_count==0) return -L4_EINVAL;
    if((l=calloc(sizeof(*l),1))==0) return -L4_ENOMEM;

    l->thread = arg->thread;
    l->ts_count = arg->ts_count;
    strncpy(l->name, arg->name, L4CPU_RESERVE_UTCB_NAME_LEN);
    /* allocate monitors */
    if((l->hists = calloc(sizeof(rt_mon_histogram_t*),
			  arg->ts_count))==0){
	err = -L4_ENOMEM;
	goto e_utcb;
    }
    for(i=0;i<arg->ts_count;i++){
	char buf[L4CPU_RESERVE_UTCB_NAME_LEN];
	if(arg->ts_count>2){
	    snprintf(buf, sizeof(buf), l4util_idfmt"/%d",
		     l4util_idstr(arg->thread), i);
	} else {
	    snprintf(buf, sizeof(buf), "%s /%s",
		     arg->name, i?"RT":"BE");
	}
	l->hists[i] = rt_mon_hist_create(0, arg->maxtime, 200, buf,
					 "us", "cnt", RT_MON_TSC_TO_US_TIME);
    }       

    l->utcb = arg->utcb;
    l->next = utcb_list;
    utcb_list = l;

    return 0;

  e_utcb:
    free(l);
    return err;
}

/*!\brief Actually remove a thread from the list of watched threads
 */
static int watch_del_utcb_do(const l4cpu_reserve_utcb_t *utcb){
    utcb_list_t *l, *o=0;

    for(l=utcb_list; l; l=l->next){
	if(l->utcb==utcb){
	    /* deallocate monitors */
	    int i;
	    for(i=0; i<l->ts_count;i++){
		rt_mon_hist_free(l->hists[i]);
	    }
	    free(l->hists);
	    if (o)
	      o->next = l->next;
	    else
	      utcb_list = l->next;
	    free(l);
	    return 0;
	}
	o=l;
    }
    return -L4_ENOTFOUND;
}


/*!\brief Helper function to add a sample to a timeslice of a thread
 *
 * \param l		utcb-list describing the thread
 * \param id		timeslice id
 * \param time		consumed time of the timeslice in the current period
 */
static void watch_insert_sample(utcb_list_t *l, int id, long time){
    rt_mon_hist_insert_data(l->hists[id], time, 1);
/*
    printf("Adding sample for "l4util_idfmt"/%d, period %lu, %d us\n",
	   l4util_idstr(l->thread), id, l->period, time);
*/
}

/*!\brief Add a history element: end of reservation/next reservation
 *
 * This function sets the history element of the given id to the
 * given time. For all rt-timeslices below the given one, a history-element
 * with time 0 will be added.
 */
static int watch_add_res_entry(utcb_list_t *l, int id,
			       unsigned long long release,
			       long time){
    int i;

    /* ignore timeslices we do not know about */
    if(id>l->ts_count) return -1;

    if(l->release!=release){
	/* new period */
	l->release = release;
	l->ts_num=0;
	l->period++;
    }
    for(i=l->ts_num; i<id; i++){
	if(i) watch_insert_sample(l, i, 0);
    }
    watch_insert_sample(l, id, time);
    if(id>l->ts_num) l->ts_num = id;
    return 0;
}

/*!\brief Add a history element: end of period/next period
 *
 * This function sets the history element of the given id to the given
 * time. For all rt-timeslices that got no reservation in this period,
 * a history-element with time 0 will be added.
 */
static void watch_add_period_entry(utcb_list_t *l, int id,
				   unsigned long long release,
				   long time){
    int i;

    if(watch_add_res_entry(l, id, release, time)) return;

    for(i=l->ts_num+1; i<l->ts_count; i++){
	watch_insert_sample(l, i, 0);
    }
    l->ts_num = l->ts_count;
}

/*!\brief Add a history element: end of period/next period
 *
 * This function sets the history element of the given id to the given
 * time. For all rt-timeslices that got no reservation in this period,
 * a history-element with time 0 will be added.
 */
static void watch_add_lost(utcb_list_t *l, int count){
    int i;

    for(i=0;i<l->ts_count;i++){
	rt_mon_hist_insert_lost(l->hists[i], count);
    }
}

/*!\brief Dump a scheduling element to the console
 */
static void print_sched_event(int id, l4cpu_reserve_utcb_elem_t*stat)
    __attribute__((unused));
static void print_sched_event(int id, l4cpu_reserve_utcb_elem_t*stat){
    printf("(%2d): type=%s, id=%d, release=%d, used=%d\n",
	   id, l4cpu_reserve_utcb_stat_itoa(stat->type), stat->id,
	   (int)stat->release, (int)stat->left);
}

/*!\brief Poll all watched threads for new events
 */
static void watch_eval_utcbs(void){
    utcb_list_t *l;
    l4cpu_reserve_utcb_t *utcb;
    int i;

    for(l=utcb_list; l; l=l->next){
	utcb = l->utcb;
	for(i = utcb->tail ; i!=utcb->head ;
	    i = (i + 1) % L4CPU_RESERVE_UTCB_STAT_COUNT) {


	    // print_sched_event(i, utcb->stat+i);

	    if(utcb->stat[i].release){
		switch(utcb->stat[i].type){
		case L4_UTCB_PERIOD_OVERRUN:
		case L4_UTCB_NEXT_PERIOD:
		    watch_add_period_entry(l, utcb->stat[i].id,
					   utcb->stat[i].release,
					   utcb->stat[i].left);
		    break;
		case L4_UTCB_RESERVATION_OVERRUN:
		case L4_UTCB_NEXT_RESERVATION:
		    watch_add_res_entry(l, utcb->stat[i].id,
					utcb->stat[i].release,
					utcb->stat[i].left);
		    break;
		}
	    }
	}
	if(utcb->full){
	    watch_add_lost(l, utcb->full);
	    utcb->full=0;
	}
	utcb->tail = i;
    }
}

/*!\brief The main watcher thread
 *
 * This thread waits for requests from other threads to add/remove threads
 * and periodically polls the watched threads for events.
 */
static void watch_thread(void*arg){
    int err=0;
    l4_msgdope_t result;
    l4_threadid_t sender;
    l4_umword_t dw1;
    watch_cmd_t cmd;
    l4_timeout_t to;

    to = l4_timeout(L4_IPC_TIMEOUT_NEVER, l4util_micros2l4to((int)arg));

    while(1){
	if(err==L4_IPC_RETIMEOUT) watch_eval_utcbs();
	err = l4_ipc_wait(&sender, L4_IPC_SHORT_MSG,
			  (l4_umword_t*)&cmd, &dw1, to, &result);
	while(1){
	    if(err) break;
	    if(!l4_task_equal(sender, watch_thread_id)) break;
	    switch(cmd){
	    case WATCH_CMD_ADD:
		dw1=watch_add_utcb_do((watch_arg_t*)dw1);
		break;
	    case WATCH_CMD_DEL:
		dw1=watch_del_utcb_do((l4cpu_reserve_utcb_t*)dw1);
		break;
	    default:
		dw1=-L4_EINVAL;
	    } /* switch cmd.v.cmd */
	    err = l4_ipc_reply_and_wait(sender, L4_IPC_SHORT_MSG,
					dw1, 0,
					&sender,
					L4_IPC_SHORT_MSG,
					(l4_umword_t*)&cmd, &dw1,
					to, &result);
	}
    }
}

