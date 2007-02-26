#include <l4/sys/ipc.h>
#include <l4/sys/rt_sched.h>
#include <l4/util/setjmp.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/log/l4log.h>

/*!
 * \file   cpu_reserve/lib/src/startperiodic.c
 * \brief  Helper functions to start periodic execution of threads
 *
 * \date   11/30/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

int l4cpu_reserve_wait_periodic(l4_threadid_t parent){
    l4_msgdope_t result;
    int err;
    l4_thread_jmp_buf jmpbuf;

    if((err=l4_thread_setjmp(jmpbuf))==0){
#if 0
	l4_umword_t dummy;
	err = l4_ipc_call(l4_next_period_id(parent), L4_IPC_SHORT_MSG,
			  0, (l4_umword_t)&jmpbuf,
			  L4_IPC_SHORT_MSG, &dummy,&dummy,
			  L4_IPC_RECV_TIMEOUT_0, &result);
#else
	err = l4_ipc_send(l4_next_period_id(parent), L4_IPC_SHORT_MSG,
			  0, (l4_umword_t)&jmpbuf,
			  L4_IPC_RECV_TIMEOUT_0, &result);
#endif
	if(err==L4_IPC_RETIMEOUT) return 0;
	return -err;
    } else {
	/* periodic mode start failed. */
	return err;
    }
}

int l4cpu_reserve_wait_periodic_ready(int mode,
				      l4_threadid_t child,
				      l4_kernel_clock_t clock){
    l4_msgdope_t result;
    l4_umword_t dummy;
    l4_thread_jmp_buf *jmpbuf;
    int err;

    if((err = l4_ipc_receive(child, L4_IPC_SHORT_MSG, &dummy,
			     (l4_umword_t*)&jmpbuf,
			     L4_IPC_NEVER, &result))!=0){
	return -err;
    }
    /* child is ready. Start periodic mode. */
    if(mode==L4_RT_BEGIN_PERIODIC)
	err = l4cpu_reserve_begin_strictly_periodic(child, clock);
    else err = l4cpu_reserve_begin_minimal_periodic(child, clock);
    if(err!=0){
	/* periodic mode not started. release the client from its NP IPC. */
	l4_thread_longjmp(child, *jmpbuf, err);
	return err;
    }
    return 0;
}
