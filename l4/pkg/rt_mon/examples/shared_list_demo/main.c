/**
 * \file   rt_mon/examples/shared_list_demo/main.c
 * \brief  Example demonstrating the usage of event lists.
 *
 * \date   12/01/2004
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
#include <l4/thread/thread.h>

#include <l4/rt_mon/event_list.h>

void worker(void * arg);
void worker(void * arg)
{
    int  val;

    // -=# monitor code start #=-
    rt_mon_event_list_t * list;
    rt_mon_basic_event_t be;

    l4_sleep(1000);
    list = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
                              RT_MON_EVTYPE_BASIC, 100,
                              "rt_mon_demos/shared_list", "my number",
                              RT_MON_THREAD_TIME, 1);
    be.id = 1;  // just   != 0
    // -=# monitor code end #=-

    val = (int)arg;

    while (1)
    {
        // -=# monitor code start #=-
        be.time = val;
        rt_mon_list_insert(list, &be);
        // -=# monitor code end #=-
        l4_sleep(val * val * 50);
    }

    // -=# monitor code start #=-
    rt_mon_list_dump(list);
    rt_mon_list_free(list);
    // -=# monitor code end #=-
}

int main(int argc, char* argv[])
{

    l4thread_create(worker, (void *)1, L4THREAD_CREATE_ASYNC);
    l4thread_create(worker, (void *)2, L4THREAD_CREATE_ASYNC);
    l4thread_create(worker, (void *)3, L4THREAD_CREATE_ASYNC);
    l4thread_create(worker, (void *)4, L4THREAD_CREATE_ASYNC);

    while (1)
    {
        l4_sleep_forever();
    }

    return 0;
}
