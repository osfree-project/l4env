/**
 * \file   rt_mon/lib/client/scalar.c
 * \brief  Scalar functions.
 *
 * \date   11/30/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/rt_mon/clock.h>
#include <l4/rt_mon/types.h>
#include <l4/rt_mon/scalar.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_generic/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/util/atomic.h>
#include <l4/util/l4_macros.h>
#include <l4/util/rdtsc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "comm.h"

double own_floor(double x);

rt_mon_scalar_t *
rt_mon_scalar_create(rt_mon_time_t low, rt_mon_time_t high,
                     const char * name, const char * unit, int clock)
{
    rt_mon_scalar_t * temp;
    l4dm_dataspace_t ds;
    int ret;

    rt_mon_calibrate_clock(clock);

    temp = l4dm_mem_ds_allocate_named(sizeof(rt_mon_scalar_t),
                                      L4DM_PINNED + L4RM_MAP, name, &ds);
    if (! temp)
        return NULL;

    temp->type = RT_MON_TYPE_SCALAR;
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

    temp->ds        = ds;
    temp->low       = low;
    temp->high      = high;
    temp->clock     = clock;
    temp->data      = 0;
    temp->val_min   = LLONG_MAX;
    temp->val_max   = LLONG_MIN;
    temp->val_sum   = 0;
    temp->val_count = 0;

    ret = rt_mon_register_ds(ds, name);
    if (ret < 0)
    {
        LOG("something wrong with registering ds");
    }
    temp->id = ret;

    return temp;
}

inline void rt_mon_scalar_insert(rt_mon_scalar_t * scalar, rt_mon_time_t data)
{
    scalar->data = data;
}

inline rt_mon_time_t rt_mon_scalar_read(rt_mon_scalar_t * scalar)
{
    return scalar->data;
}

void rt_mon_scalar_free(rt_mon_scalar_t * scalar)
{
    int id;

    id = scalar->id;
    l4rm_detach(scalar);
    rt_mon_unregister_ds(id);
}

void rt_mon_scalar_dump(rt_mon_scalar_t * scalar)
{
    printf("#DS: id = %d, manager = "l4util_idfmt"\n",
           scalar->ds.id, l4util_idstr(scalar->ds.manager));
    printf("#Name: %s\n", scalar->name);
    printf("#Low: %lld\n", scalar->low);
    printf("#High: %lld\n", scalar->high);
    printf("%lld\n", scalar->data);
    // fixme: more data to print here
}

inline rt_mon_time_t rt_mon_scalar_measure(rt_mon_scalar_t * scalar)
{
    rt_mon_time_t time;

    RT_MON_GET_TIME(scalar->clock, time);

    return time;
}
