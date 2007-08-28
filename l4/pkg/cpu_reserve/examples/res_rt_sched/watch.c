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
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/rt_sched.h>
#include <l4/sys/types.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include "watch.h"

int *count_mand;	// id==1
int *count_opt;		// id==2
int show_mand_preempts;
int show_opt_preempts;

void preempter_thread (void*arg){
    l4_rt_preemption_t dw;
    l4_msgdope_t result;
    int mand=0, opt=0;
    l4_threadid_t worker_id = *(l4_threadid_t*)arg;
    count_mand = &mand;
    count_opt = &opt;
    l4thread_started(0);

    LOGL("Waiting for Preemption-IPCs from "l4util_idfmt,
	l4util_idstr(worker_id));

    while (1) {
        // wait for preemption IPC
        if (l4_ipc_receive(l4_preemption_id(worker_id),
                           L4_IPC_SHORT_MSG,
                           (l4_umword_t*)&dw.lh.low, (l4_umword_t*)&dw.lh.high,
                           L4_IPC_NEVER, &result) == 0){

            if (dw.p.type == L4_RT_PREEMPT_TIMESLICE) {
                if (dw.p.id == 1){
		    LOGd(show_mand_preempts,
			 "Preemption-IPC, mandatory (Type %u, Time:%llu)",
			 dw.p.type, dw.p.time);
                    l4util_inc32 (count_mand);
		} else if (dw.p.id == 2){
		    LOGd(show_opt_preempts,
			 "Preemption-IPC, optional (Type %u, Time:%llu)",
			 dw.p.type, dw.p.time);
                    l4util_inc32 (count_opt);
		}
            }
        } else
	    LOG("Preempt-receive returned %lx", L4_IPC_ERROR(result));
    }
}
