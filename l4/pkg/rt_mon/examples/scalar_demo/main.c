/**
 * \file   rt_mon/examples/scalar_demo/main.c
 * \brief  Example demonstrating the usage of scalars.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/rand.h>
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>

#include <l4/rt_mon/scalar.h>

int main(int argc, char* argv[])
{
    int i, j, loops, pm;

    // -=# monitor code start #=-
    rt_mon_scalar_t * s1, * s2, * s3, * s4;
    rt_mon_time_t start, end;

    l4_sleep(1000);
    s1 = rt_mon_scalar_create(0, 100, "rt_mon_demos/scalar/%",
                              "%", RT_MON_THREAD_TIME);
    s2 = rt_mon_scalar_create(0, 1000, "rt_mon_demos/scalar/duration",
                              "time [us]", RT_MON_THREAD_TIME);
    s3 = rt_mon_scalar_create(0, 100000, "rt_mon_demos/scalar/progress",
                              "progress", RT_MON_THREAD_TIME);
    s4 = rt_mon_scalar_create(0, 67, "rt_mon_demos/scalar/bandwidth",
                              "Bandwidth [MB/s]", RT_MON_THREAD_TIME);
    // -=# monitor code end #=-

    pm = 0;
    j = 0;
    while (1)
    {
        pm = (pm + 1) % 1000;
        // -=# monitor code start #=-
        // simply insert a local value to transport it to a monitor
        rt_mon_scalar_insert(s1, pm / 10);
        // -=# monitor code end #=-

        // -=# monitor code start #=-
        start = rt_mon_scalar_measure(s2);
        // -=# monitor code end #=-

        // do some work
        loops = 10000 + l4util_rand();
        for (i = 0; i < loops; i++)
            asm volatile ("": : : "memory");

        // -=# monitor code start #=-
        // measure some duration to transport it to a monitor
        end = rt_mon_scalar_measure(s2);
        rt_mon_scalar_insert(s2, end - start);
        // -=# monitor code end #=-

        // -=# monitor code start #=-
        // another simple value transported
        rt_mon_scalar_insert(s3, j);
        // -=# monitor code end #=-

        // -=# monitor code start #=-
        // make some internal statistic available externally
        // bandwidth will be between 30 and 65 MB/s
        rt_mon_scalar_insert(s4, 30  + l4util_rand() % 35);
        // -=# monitor code end #=-

        l4_sleep(10);
        j++;
    }

    // -=# monitor code start #=-
    /* will never be reached do to the infinite loop above, but demonstrates
     * cleanup.
     *
     * The sensor will be freed after all parties released it, that
     * is, the creator and all monitors.
     */
    rt_mon_scalar_dump(s1);  // not necessary, better done in monitor,
                             // but possible nevertheless
    rt_mon_scalar_free(s1);
    rt_mon_scalar_free(s2);
    rt_mon_scalar_free(s3);
    rt_mon_scalar_free(s4);
    // -=# monitor code end #=-

    return 0;
}
