/**
 * \file   rt_mon/examples/hist_demo/main.c
 * \brief  Example demonstrating the usage of histograms.
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

#include <l4/rt_mon/histogram.h>

int main(int argc, char* argv[])
{
    int i, j, loops;

    // -=# monitor code start #=-
    rt_mon_histogram_t * hist;

    l4_sleep(1000);
    hist = rt_mon_hist_create(0, 200, 200, "rt_mon_demos/hist",
                              "duration [us]", "abs. frq.",
                              RT_MON_THREAD_TIME);
    // -=# monitor code end #=-

    for (j = 0; j < 1000000; j++)
    {
        // -=# monitor code start #=-
        rt_mon_hist_start(hist);
        // -=# monitor code end #=-

        // do some work
        loops = 10000 + l4util_rand();
        for (i = 0; i < loops; i++)
            asm volatile ("": : : "memory");

        // -=# monitor code start #=-
        rt_mon_hist_end(hist);
        // -=# monitor code end #=-
        if (j % 100 == 0)
            l4_sleep(1);
    }

    // -=# monitor code start #=-
    rt_mon_hist_dump(hist);
    rt_mon_hist_free(hist);
    // -=# monitor code end #=-

    return 0;
}
