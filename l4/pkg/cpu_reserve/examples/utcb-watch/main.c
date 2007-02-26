#include <l4/thread/thread.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/sys/syscalls.h>
#include <l4/util/rdtsc.h>
#include <l4/sys/rt_sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <l4/cpu_reserve/utcb_watch.h>

l4_ssize_t l4libc_heapsize = 64*1024;

static void thread(void*arg){
    l4cpu_reserve_utcb_t **utcb = (l4cpu_reserve_utcb_t**)arg;
    int err, id;
    int wcet = 1000;

    *utcb = (l4cpu_reserve_utcb_t*)l4_utcb_get();
    LOGL("utcb at %p", *utcb);
    l4thread_started(0);

    if((err = l4cpu_reserve_add(l4_myself(), ".res", 0x80, 10*1000,
				&wcet, 10*1000, &id))!=0){
	l4env_perror("l4cpu_reserve_add()", -err);
	return;
    }
    if((err = l4cpu_reserve_begin_strictly_periodic_self(L4_INVALID_ID))!=0){
	l4env_perror("l4cpu_reserve_begin_strictly_periodic_self()", -err);
	return;
    }
    l4_usleep(1);
    for(id=0;;id=(id+1)%1000){
	l4_busy_wait_ns(id*id);
	l4_rt_next_period();
    }
}

int main(void){
    int err;
    l4thread_t t;
    l4cpu_reserve_utcb_t *utcb;

    l4_tsc_init(L4_TSC_INIT_KERNEL);
    if((err=l4cpu_reserve_utcb_watch_init(100*1000))!=0) exit(1);
    t = l4thread_create_named(thread, ".rt", &utcb, L4THREAD_CREATE_SYNC);
    l4cpu_reserve_utcb_watch_add(l4thread_l4_id(t), utcb,
				 "utcb-watch", 1000, 2);
    l4_sleep_forever();
    return 0;
}

