/**
 * \file   rt_mon/examples/list_demo/main.c
 * \brief  Example demonstrating the usage of event lists.
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

#include <l4/rt_mon/event_list.h>

int main(int argc, char* argv[])
{
    int i, j, loops;


    // -=# monitor code start #=-
    rt_mon_event_list_t * list;
    rt_mon_basic_event_t be;
    rt_mon_time_t start, end;

    l4_sleep(1000);
    list = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
                              RT_MON_EVTYPE_BASIC, 100,
                              "rt_mon_demos/list", "duration [us]",
                              RT_MON_THREAD_TIME, 0);
    be.id = 1;  // just   != 0
    // -=# monitor code end #=-

    for (j = 0; j < 1000000; j++)
    {
        // -=# monitor code start #=-
        start = rt_mon_list_measure(list);
        // -=# monitor code end #=-

        // do some work
        loops = 10000 + l4util_rand();
        for (i = 0; i < loops; i++)
            asm volatile ("": : : "memory");

        // -=# monitor code start #=-
        end = rt_mon_list_measure(list);
        be.time = end - start;
        be.data = i;
        rt_mon_list_insert(list, &be);
        // -=# monitor code end #=-
        if (j % 1 == 0)
            l4_sleep(2);
    }

    // -=# monitor code start #=-
    rt_mon_list_dump(list);
    rt_mon_list_free(list);
    // -=# monitor code end #=-

    return 0;
}
