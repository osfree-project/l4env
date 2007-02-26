/**
 * \file   rt_mon/lib/client/event_list.c
 * \brief  Some functions to work with event lists.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/rt_mon/clock.h>
#include <l4/rt_mon/types.h>
#include <l4/rt_mon/event_list.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_generic/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/rdtsc.h>
#include <l4/util/util.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "comm.h"

rt_mon_event_list_t *
rt_mon_list_create(int event_size, int event_type, int event_count,
                   const char * name, const char * unit, int clock,
                   int shared)
{
    rt_mon_event_list_t * temp;
    l4dm_dataspace_t ds;
    int ret, id, instance;

    if ((event_size % 4) != 0)
    {
        LOG("please use a multiple of 4 Byte as event_size");
        return NULL;
    }
    rt_mon_calibrate_clock(clock);

    if (shared == 0)
    {
        temp = l4dm_mem_ds_allocate_named(
            sizeof(rt_mon_event_list_t) + event_size * event_count,
            L4DM_PINNED + L4RM_MAP, name, &ds);
        if (! temp)
            return NULL;

        temp->type = RT_MON_TYPE_EVENT_LIST;
        if (name)
        {
            strncpy(temp->name, name, RT_MON_NAME_LENGTH - 1);
            temp->name[RT_MON_NAME_LENGTH - 1] = 0;
        }
        else
            temp->name[0] = 0;
        if (unit)
        {
            strncpy(temp->unit, unit, RT_MON_NAME_LENGTH - 1);
            temp->unit[RT_MON_NAME_LENGTH - 1] = 0;
        }
        else
        {
            strncpy(temp->unit, "us", RT_MON_NAME_LENGTH - 1);
            temp->unit[RT_MON_NAME_LENGTH - 1] = 0;
        }
        temp->ds = ds;
        temp->event_size = event_size;
        temp->event_type = event_type;
        temp->insert = 0;
        temp->remove = 0;
        temp->max_events = event_count;
        temp->overruns = 0;
        temp->clock = clock;
        bzero(&temp->data, event_size * event_count);

        ret = rt_mon_register_ds(ds, name);
        if (ret < 0)
        {
            LOG("something wrong with registering ds");
        }
        temp->id = ret;
    }
    else
    {
        id = rt_mon_request_shared_ds(
             &ds,
             sizeof(rt_mon_event_list_t) +
               (event_size + sizeof(rt_mon_event_container_t)) * event_count,
             name, &instance, (void **)(&temp));
        if (id < 0)
        {
            LOG("Error getting shared ds, returning NULL!");
            return NULL;
        }

        if (instance == 0)  // we are the first, so setup stuff
        {
            if (name)
            {
                strncpy(temp->name, name, RT_MON_NAME_LENGTH - 1);
                temp->name[RT_MON_NAME_LENGTH - 1] = 0;
            }
            else
                temp->name[0] = 0;
            if (unit)
            {
                strncpy(temp->unit, unit, RT_MON_NAME_LENGTH - 1);
                temp->unit[RT_MON_NAME_LENGTH - 1] = 0;
            }
            else
            {
                strncpy(temp->unit, "us", RT_MON_NAME_LENGTH - 1);
                temp->unit[RT_MON_NAME_LENGTH - 1] = 0;
            }
            temp->ds = ds;
            temp->event_size = event_size;  // size of the internal events
            temp->event_type = event_type;  // type of the internal events
            temp->insert = 0;
            temp->remove = 0;
            temp->max_events = event_count;
            temp->overruns = 0;
            temp->clock = clock;
            bzero(&temp->data, (event_size +
                                sizeof(rt_mon_event_container_t)) *
                               event_count);
            temp->id = id;
            temp->type = RT_MON_TYPE_SHARED_LIST;
        }
        else  // we are not the first, so wait until someone sets up stuff
        {
            while (temp->type == RT_MON_TYPE_UNDEFINED)
            {
                LOG("Waiting for first instance to finish init.");
                l4_sleep(20);
            }
        }
    }
    return temp;
}

void
rt_mon_list_free(rt_mon_event_list_t * list)
{
    int id;

    id = list->id;
    l4rm_detach(list);
    rt_mon_unregister_ds(id);
}

inline void
rt_mon_list_insert(rt_mon_event_list_t * list, const void * event)
{
    if (list->type == RT_MON_TYPE_EVENT_LIST)
    {
        if ((list->insert + 1) % list->max_events != list->remove)
        {
            memcpy(&(list->data[list->insert *
                                list->event_size / sizeof(list->data[0])]),
                   event, list->event_size);
            list->insert = (list->insert + 1) % list->max_events;
        }
        else
            list->overruns++;
    }
    else if (list->type == RT_MON_TYPE_SHARED_LIST)
    {
        int l_i, l_i_old, index, ret;
        rt_mon_event_container_t * ec;

        // atomically advance global insert pointer, as long as not full
        do
        {
            l_i_old = list->insert;
            l_i = (l_i_old + 1) % list->max_events;   // advance
            if (l_i == list->remove)                  // check if full
            {
                l4util_inc32(&(list->overruns));
                return;
            }
            ret = l4util_cmpxchg32(&(list->insert), l_i_old, l_i);
        } while (ret == 0);

        // I can use l_i_old now!, so insert own data
        index = l_i_old * (list->event_size +
                           sizeof(rt_mon_event_container_t)) /
            sizeof(list->data[0]);
        ec = (rt_mon_event_container_t *)&(list->data[index]);
        memcpy(ec->data, event, list->event_size);    // copy event data
        ec->committed = 1;                            // mark buffer as filled

    }
    else
    {
        LOG("Unknown list type, ignored!");
    }
}

inline rt_mon_time_t
rt_mon_list_measure(rt_mon_event_list_t * list)
{
    rt_mon_time_t time;

    RT_MON_GET_TIME(list->clock, time);

    return time;
}

inline int
rt_mon_list_remove(rt_mon_event_list_t * list, void * event)
{
    if (list->type == RT_MON_TYPE_EVENT_LIST)
    {
        if (list->insert != list->remove)
        {
            memcpy(event,
                   &(list->data[list->remove *
                                list->event_size / sizeof(list->data[0])]),
                   list->event_size);
            list->remove = (list->remove + 1) % list->max_events;
            return 0;
        }
        else
            return -1;
    }
    else if (list->type == RT_MON_TYPE_SHARED_LIST)
    {
        rt_mon_event_container_t * ec;
        int index;

        if (list->insert == list->remove)          // check if buffer is empty
            return -1;

        index = list->remove *
                  (list->event_size + sizeof(rt_mon_event_container_t)) /
                  sizeof(list->data[0]);
        ec = (rt_mon_event_container_t *)&(list->data[index]);
        if (ec->committed == 0)                    // buffer committed already?
            return -1;

        memcpy(event, ec->data, list->event_size); // copy event data
        ec->committed = 0;                         // mark buffer as free
        list->remove = (list->remove + 1) %        // advance remove index
            list->max_events;

        return 0;
    }
    else
    {
        LOG("Unknown list type, ignored!");
        return -1;
    }
}

void
rt_mon_list_dump(rt_mon_event_list_t * list)
{
    if (list->type == RT_MON_TYPE_EVENT_LIST)
    {
        int i, j;

        printf("#DS: id = %d, manager = "l4util_idfmt"\n",
               list->ds.id, l4util_idstr(list->ds.manager));
        printf("#Name: %s\n", list->name);
        printf("#Event size: %d\n", list->event_size);
        printf("#Insert index: %d\n", list->insert);
        printf("#Remove index: %d\n", list->remove);
        printf("#Max. events: %d\n", list->max_events);
        printf("#Overruns: %d\n", list->overruns);

        for(i = 0; i < list->max_events; i++)
        {
            // asume that events are 4 byte aligned
            for (j = 0; j < list->event_size / 4; j++)
            {
                printf("%d : ", list->data[i * (list->event_size /
                                                sizeof(list->data[0])) + j]);
            }
            printf("\n");
        }
    }
    else if (list->type == RT_MON_TYPE_SHARED_LIST)
    {
        int i, j, index;
        rt_mon_event_container_t * ec;

        // fixme: care for wrapped events

        printf("#DS: id = %d, manager = "l4util_idfmt"\n",
               list->ds.id, l4util_idstr(list->ds.manager));
        printf("#Name: %s\n", list->name);
        printf("#Event size: %d\n", list->event_size);
        printf("#Insert index: %d\n", list->insert);
        printf("#Remove index: %d\n", list->remove);
        printf("#Max. events: %d\n", list->max_events);
        printf("#Overruns: %d\n", list->overruns);

        for(i = 0; i < list->max_events; i++)
        {
            index = i * (list->event_size + sizeof(rt_mon_event_container_t)) /
                      sizeof(list->data[0]);
            ec = (rt_mon_event_container_t *)&(list->data[index]);
            // assume that the events are 4 byte aligned
            for (j = 0; j < list->event_size / 4; j++)
            {
                printf("%d : ", ec->data[j]);
            }
            printf("\n");
        }
    }
    else
    {
        LOG("Unknown list type, ignored!");
    }
}
