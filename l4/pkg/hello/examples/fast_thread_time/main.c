#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/l4int.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/rt_sched.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/timeout.h>
#include <l4/sys/types.h>
#include <l4/sigma0/kip.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/thread_time.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>
#include <l4/util/rdtsc.h>


l4_ssize_t l4libc_heapsize   = 2*1024*1024;
static l4_threadid_t recv    = L4_INVALID_ID;
static l4_threadid_t main_id = L4_INVALID_ID;
static int recv_stack[4096];

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

static void recv_thread (void)
{
    while (1)
    {
        l4_sleep(100);
    }
}


int main (void)
{
    int i;
    l4_kernel_info_t *kinfo;

    l4_cpu_time_t kernel_thread_time, fast;

    main_id = l4_myself();

    kinfo = l4sigma0_kip_map(L4_INVALID_ID);

    // create another thread, such that thread switches actually happen ...
    recv = l4util_create_thread(1, recv_thread, recv_stack +
                                sizeof(recv_stack) /
                                sizeof(recv_stack[0]));

    l4_calibrate_tsc();

    while(1)
    {
        // burn some CPU time
        for (i = 0; i < 100000000; i++)
            ;

        fast = l4_tsc_to_us(l4util_thread_time(kinfo));

        // get kernel thread time
        {
            int           ret;
            l4_threadid_t next;
            l4_umword_t   prio;
            /* should not fail, as thread always exists */
            ret = fiasco_get_cputime(l4_myself(), &next, &(kernel_thread_time),
                                     &prio);
        }

        my_log("Thread time via: fiasco_get_cputime(): %lld; FAST: %lld;"
               " diff: %lld\n", kernel_thread_time, fast,
               kernel_thread_time - fast);

        l4_sleep(1000);  // be nice to the system
    }
}
