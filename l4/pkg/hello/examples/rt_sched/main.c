#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/rt_sched.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>
#include <l4/util/parse_cmd.h>
#include <l4/sigma0/kip.h>
#include <l4/rmgr/librmgr.h>
#include <l4/log/l4log.h>

int show_mand_preempts;
int show_opt_preempts;
int show_nextres_error;
int show_left;
int show_change;

static l4_threadid_t main_thread_id = L4_INVALID_ID;
static l4_threadid_t preempter      = L4_INVALID_ID;
static l4_threadid_t pager          = L4_INVALID_ID;
static int preempter_stack[4096];

l4_ssize_t l4libc_heapsize = 64*1024;

l4_uint32_t count_mand, count_opt;

static void out(const char*);
void (*LOG_outstring)(const char*log_message)=out;
static void out(const char*s){
    outstring(s);
}

static void preempter_thread (void){
    l4_rt_preemption_t dw;
    l4_msgdope_t result;

    LOGL("Waiting for Preemption-IPCs from "l4util_idfmt,
	l4util_idstr(main_thread_id));

    while (1) {
        // wait for preemption IPC
        if (l4_ipc_receive(l4_preemption_id(main_thread_id),
                           L4_IPC_SHORT_MSG, &dw.lh.low, &dw.lh.high,
                           L4_IPC_NEVER, &result) == 0){

            if (dw.p.type == L4_RT_PREEMPT_TIMESLICE) {
                if (dw.p.id == 1){
		    LOGd(show_mand_preempts,
			 "Preemption-IPC, mandatory (Type %u, Time:%llu)",
			 dw.p.type, (unsigned long long)dw.p.time);
                    l4util_inc32 (&count_mand);
		} else if (dw.p.id == 2){
		    LOGd(show_opt_preempts,
			 "Preemption-IPC, optional (Type %u, Time:%llu)",
			 dw.p.type, (unsigned long long)dw.p.time);
                    l4util_inc32 (&count_opt);
		}
            }
        } else
	    LOG("Preempt-receive returned %lx", L4_IPC_ERROR(result));
    }
}

int main (int argc, const char**argv)
{
    int period, j, ret;
    l4_umword_t word1;
    int loops_mand, loops_opt;
    l4_kernel_info_t *kinfo;
    l4_threadid_t next_period_id;
    int time_mand = 3000;
    double fact=1.1;

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
		     0, 0)) return 1;

    next_period_id = l4_next_period_id(l4_myself());
    kinfo = l4sigma0_kip_map(L4_INVALID_ID);

    main_thread_id = l4_myself();

    // create preempter
    preempter = l4util_create_thread(1, preempter_thread, 
				     preempter_stack + 
				     sizeof(preempter_stack)/
				     sizeof(preempter_stack[0]));
    rmgr_set_prio(preempter, 255);

    // set preemter
    pager = L4_INVALID_ID;
    l4_thread_ex_regs(main_thread_id, -1UL, -1UL, &preempter, &pager,
                      &word1, &word1, &word1);

    // add mandatory timeslice
    if((ret = l4_rt_add_timeslice(main_thread_id, 50, time_mand))!=0) {
        LOG("l4_rt_add_timeslice(): %d", ret);
	return 1;
    }

    // add optional mandatory timeslice
    if((ret = l4_rt_add_timeslice(main_thread_id, 40, 5000))!=0) {
	LOG("l4_rt_add_timeslice(): %d", ret);
	return 1;
    }

    // set period
    l4_rt_set_period (main_thread_id, 15000);

    // commit to periodic work
    if((ret = l4_rt_begin_strictly_periodic(main_thread_id,
					    kinfo->clock + 20000))!=0) {
	LOG("l4_rt_begin_strictly_periodic(): %d", ret);
	return 1;
    }

    period = 0;
    loops_mand = 3000000;
    loops_opt  = 3000000;
    while(1) {
	l4_kernel_clock_t left;

        // next period
#if 0
#if 0
        ret = l4_ipc_receive(l4_next_period_id(main_thread_id),
			     L4_IPC_SHORT_MSG,
                             &word1, &word2, L4_IPC_NEVER, &result);
#else
        ret = l4_ipc_receive(l4_next_period_id(L4_NIL_ID),
			     L4_IPC_SHORT_MSG,
                             &word1, &word2, L4_IPC_BOTH_TIMEOUT_0,
			     &result);
#endif
#else
        ret = l4_rt_next_period();
#endif

        if (ret != 0)
	    LOG("next_period result = %x", ret);

        for (j = 0; j < loops_mand; j++)
            ;  // do some work ...

        // next reservation
        ret = l4_rt_next_reservation(1, &left);
        if (ret != 0)
	    LOGd(show_nextres_error,"next_reservation result = %d", ret);
	LOGd(show_left, "mandatory left: %lld micros, ret=%d",
	     left, ret);

        for (j = 0; j < loops_opt; j++)
            ;  // do some work ...

        period++;
        if (period % 100 == 0)
        {
	    // fall back to timesharing timeslice
            ret = l4_rt_next_reservation(2, &left);
            if (ret != 0)
		LOGd(show_nextres_error,
		     "next_reservation result = %d", ret);
	    LOGd(show_left, "optional left: %lld micros", left);

            printf("Period = %d, mand time %d, opt loops %d, count (%d, %d)\n",
		   period, time_mand, loops_opt, count_mand, count_opt);

	    /* modify the length of the timeslices */
	    if (count_mand == 0){
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
	    if((ret = l4_rt_change_timeslice(main_thread_id,
					     1, 50, time_mand))!=0) {
		LOG("l4_rt_change_timeslice(): %d", ret);
		return 1;
	    }

            // adapt optional work load
            if (count_opt == 0){
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
            count_mand  = 0;
            count_opt   = 0;
	}
    }
}
