/*!
 * \file   cpu_reserve/examples/resv/main.c
 * \brief  Example showing how to start periodic mode
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
#include <l4/sys/rt_sched.h>
#include <l4/sigma0/kip.h>
#include <l4/util/parse_cmd.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>
#include <l4/util/irq.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <stdio.h>
#include <stdlib.h>

static l4semaphore_t end_sem;
static l4_kernel_info_t *kip;

static void thread_fn(void*arg){
    l4_threadid_t parent = l4thread_l4_id(l4thread_get_parent());
    int err;

    err = l4cpu_reserve_wait_periodic(parent);
    LOGL("l4cpu_reserve_wait_periodic(parent): %s", l4env_strerror(-err));
    LOG_flush();
    enter_kdebug("Periodic mode?");
    l4semaphore_up(&end_sem);
    l4_sleep_forever();
}

static l4thread_t start_thread(void){
    int wcet, err, id;
    l4thread_t thread;

    end_sem = L4SEMAPHORE_LOCKED;
    if((thread = l4thread_create_named(thread_fn, ".rt", 0, 0))<0){
	l4env_perror("l4_thread_create_named()", -thread);
	return thread;
    }
    wcet = 10000;
    if((err = l4cpu_reserve_add(l4thread_l4_id(thread), "reservation",
				0x90, 100000, &wcet, 0,
				&id))!=0){
	l4env_perror("l4cpu_reserve_add()", -err);
	return err;
    }
    return thread;
}

static void stop_thread(l4thread_t thread){
    int err;
    l4_threadid_t l4 = l4thread_l4_id(thread);

    l4semaphore_down(&end_sem);
    if((err=l4thread_shutdown(thread))!=0){
	l4env_perror("l4thread_shutdown()", -err);
    }
    if((err=l4cpu_reserve_end_periodic(l4))!=0){
	l4env_perror("l4cpu_reserve_end_periodic(()", -err);
    }
    if((err=l4cpu_reserve_delete_thread(l4))!=0){
	l4env_perror("l4cpu_reserve_delete_thread()", -err);
    }
}

int main(int argc, const char**argv){
    int err;
    l4thread_t thread;

    rmgr_init();
    kip = l4sigma0_kip_map(L4_INVALID_ID);

    printf("Starting thread in periodic mode ASAP. This must not fail.\n");
    LOGk("periodic, ASAP");
    if((thread=start_thread())<0){
	l4env_perror("start_thread()", -thread);
	return 1;
    }
    /* start periodic mode, with a time of 0. */
    if((err=l4cpu_reserve_wait_periodic_ready(L4_RT_BEGIN_PERIODIC,
					      l4thread_l4_id(thread),
					      0))!=0){
	LOGl("Periodic start failed!");
    } else {
	printf("periodic execution of "l4util_idfmt" started\n",
	       l4util_idstr(l4thread_l4_id(thread)));
    }
    stop_thread(thread);

    /**********************************************************************/

    printf("\nStarting thread 1 second in the future. This should work.\n");
    LOGk("periodic, +1s");
    if((thread=start_thread())<0){
	l4env_perror("start_thread()", -thread);
	return 1;
    }
    /* start periodic mode, with a time of 0. */
    if((err=l4cpu_reserve_wait_periodic_ready(L4_RT_BEGIN_PERIODIC,
					      l4thread_l4_id(thread),
					      kip->clock+1000000))!=0){
	LOGl("Periodic start failed!");
    } else {
	printf("periodic execution of "l4util_idfmt" started\n",
	       l4util_idstr(l4thread_l4_id(thread)));
    }
    stop_thread(thread);

    /**********************************************************************/

    printf("\nStarting thread in the past. This must fail.\n");
    LOGk("periodic, -1s");
    if((thread=start_thread())<0){
	l4env_perror("start_thread()", -thread);
	return 1;
    }
    /* start periodic mode, with a time of 0. */
    if((err=l4cpu_reserve_wait_periodic_ready(L4_RT_BEGIN_PERIODIC,
					      l4thread_l4_id(thread),
					      kip->clock-10000))!=0){
	LOGl("Periodic start failed!");
    } else {
	printf("periodic execution of "l4util_idfmt" started\n",
	       l4util_idstr(l4thread_l4_id(thread)));
    }
    stop_thread(thread);
    return 0;
}
