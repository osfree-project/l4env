#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/rt_sched.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/sigma0/kip.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>

static l4_threadid_t main_thread_id = L4_INVALID_ID;
static l4_threadid_t preempter      = L4_INVALID_ID;
static l4_threadid_t pager          = L4_INVALID_ID;
static int preempter_stack[4096];

l4_ssize_t l4libc_heapsize = 2*1024*1024;

int count_mand = 0;
int count_opt  = 0;

static void __attribute__((format (printf, 1, 2)))
my_log(const char *format, ...)
{
    va_list list;
    char buf[120];

    va_start(list, format);
    vsnprintf(buf, sizeof(buf), format, list);
    outstring(buf);
    va_end(list);
}

static void
set_prio (l4_threadid_t thread, int prio)
{
    l4_sched_param_t sched;
    l4_threadid_t s = L4_INVALID_ID;

    l4_thread_schedule (thread, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
    sched.sp.prio  = prio;
    sched.sp.state = 0;
    sched.sp.small = 0;
    s = L4_INVALID_ID;
    l4_thread_schedule (thread, sched, &s, &s, &sched);
}

static void
preempter_thread (void)
{
    l4_umword_t word1, word2;
    l4_msgdope_t result;

    my_log("Waiting for Preemption-IPCs from %u.%u\n",
           main_thread_id.id.task, main_thread_id.id.lthread);

    while (1)
    {
        // wait for preemption IPC
        if (l4_ipc_receive(l4_preemption_id(main_thread_id),
                           L4_IPC_SHORT_MSG, &word1, &word2,
                           L4_IPC_NEVER, &result) == 0)
        {
            unsigned int type = (word2 & 0x80000000U) >> 31;
            unsigned int id   = (word2 & 0x7f000000U) >> 24;
            my_log("Received P-IPC (Type:%u ID:%u, Time:%llu)\n",
                    type, id,
                    ((unsigned long long) word2 << 32 | word1) &
                    0x00ffffffffffffffULL);

            if (type == 1)
            {
                if (id == 1)
                    l4util_inc32 (&count_mand);
                else if (id == 2)
                    l4util_inc32 (&count_opt);
            }
        }
        else
            my_log("Receive retuned %lx\n", L4_IPC_ERROR(result));
    }
}

int main (void)
{
    int period, j, ret;
    int loops_mand, loops_opt;
    l4_kernel_info_t *kinfo;
    l4_threadid_t next_period_id;
    l4_msgdope_t result;
    l4_umword_t word1, word2;

    next_period_id = l4_next_period_id(l4_myself());
    kinfo = l4sigma0_kip_map(L4_INVALID_ID);

    main_thread_id = l4_myself();

    // create preempter
    preempter = l4util_create_thread(1, preempter_thread, 
                                     preempter_stack + 
                                     sizeof(preempter_stack)/
                                     sizeof(preempter_stack[0]));
    set_prio(preempter, 255);

    // set preemter
    pager = L4_INVALID_ID;
    l4_thread_ex_regs(main_thread_id, -1, -1, &preempter, &pager,
                      &word1, &word1, &word1);

    // mand.
    if((ret = l4_rt_add_timeslice(main_thread_id, 50, 3000))!=0)
    {
        printf("l4_rt_timeslice(): %d\n", ret);
        return 1;
    }

    // opt.
    if((ret = l4_rt_add_timeslice(main_thread_id, 40, 5000))!=0)
    {
        printf("l4_rt_timeslice(): %d\n", ret);
        return 1;
    }

    // period
    l4_rt_set_period (main_thread_id, 15000);

    // commit to periodic work
    if((ret = l4_rt_begin_strictly_periodic(main_thread_id,
                                            kinfo->clock + 20000))!=0)
    {
        printf("l4_rt_begin_strictly_periodic(): %d\n", ret);
        return 1;
    }

    period = 0;
    loops_mand = 3000000;
    loops_opt  = 3000000;
    while(1)
    {
        l4_kernel_clock_t left;

        // next period
        ret = l4_ipc_receive(next_period_id, L4_IPC_SHORT_MSG,
                             &word1, &word2, L4_IPC_NEVER, &result);

        if (ret != 0)
            my_log("next_period result = %x\n", ret);

        for (j = 0; j < loops_mand; j++)
            ;  // do some work ...

        // next reservation
        ret = l4_rt_next_reservation(1, &left);
        if (ret != 0)
            my_log( "next_reservation result = %x\n", ret);
        // printf("mandatory left: %ld micros, ret=%d\n", (long)left, ret);

        for (j = 0; j < loops_opt; j++)
            ;  // do some work ...

        period++;
        if (period % 100 == 0)
        {
        // next reservation
            ret = l4_rt_next_reservation(2, &left);
            if (ret != 0)
                my_log("next_reservation result = %x\n", ret);
            // printf("optional left: %ld micros\n", (long)left);

            my_log("Period = %d, loops = (%d, %d), count = (%d, %d)\n",
                   period, loops_mand, loops_opt, count_mand, count_opt);

            // adapt work load
            if (count_mand == 0)
                loops_mand *= 1.01;
            else
                loops_mand *= 0.98;
            if (count_opt == 0)
                loops_opt *= 1.01;
            else
                loops_opt *= 0.98;
            // reset counters
            count_mand  = 0;
            count_opt   = 0;
        }
    }
}
