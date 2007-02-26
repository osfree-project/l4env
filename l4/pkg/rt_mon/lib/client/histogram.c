/**
 * \file   rt_mon/lib/client/histogram.c
 * \brief  Some histogram functions.
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
#include <l4/rt_mon/histogram.h>

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

/* Create a new histogram, resulting pointer may be freed
 */
rt_mon_histogram_t *
rt_mon_hist_create(double low, double high, int bins,
                   const char * name,
                   const char * unit_abs, const char * unit_ord, int clock)
{
    rt_mon_histogram_t * temp;
    l4dm_dataspace_t     ds;
    int                  ret, i;

    rt_mon_calibrate_clock(clock);

    temp = l4dm_mem_ds_allocate_named(
        sizeof(rt_mon_histogram_t) + sizeof(temp->data[0]) * bins,
        L4DM_PINNED + L4RM_MAP, name, &ds);
    if (! temp)
        return NULL;

    temp->type = RT_MON_TYPE_HISTOGRAM;
    if (name)
    {
        strncpy(temp->name, name, RT_MON_NAME_LENGTH - 1);
        temp->name[RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
        temp->name[0] = 0;
    if (unit_abs)
    {
        strncpy(temp->unit[0], unit_abs, RT_MON_NAME_LENGTH - 1);
        temp->unit[0][RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
    {
        strncpy(temp->unit[0], "us", RT_MON_NAME_LENGTH - 1);
        temp->unit[0][RT_MON_NAME_LENGTH - 1] = 0;
    }
    if (unit_ord)
    {
        strncpy(temp->unit[1], unit_ord, RT_MON_NAME_LENGTH - 1);
        temp->unit[1][RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
    {
        strncpy(temp->unit[1], "eggs", RT_MON_NAME_LENGTH - 1);
        temp->unit[1][RT_MON_NAME_LENGTH - 1] = 0;
    }
    temp->ds        = ds;
    temp->low       = low;
    temp->high      = high;
    temp->bin_size  = (high - low) / bins;
    temp->bins      = bins;
    temp->clock     = clock;
    rt_mon_hist_reset(temp);

    // measuring overhead to take time
    for (i = 0; i < 100; i++)
    {
        rt_mon_hist_start(temp);
        rt_mon_hist_end(temp);
    }
    temp->time_ovh = rt_mon_hist_avg(temp);
    rt_mon_hist_reset(temp);

    ret = rt_mon_register_ds(ds, name);
    if (ret < 0)
    {
        LOGl("something wrong with registering ds");
    }
    temp->id = ret;

    return temp;
}

void rt_mon_hist_insert_data(rt_mon_histogram_t * hist, rt_mon_time_t x,
                             unsigned int val)
{
    int bin;

    bin = (int)own_floor(((x - hist->low) / hist->bin_size));

    // use atomic data access, such that it can be used from several places
    if (bin < 0)
        l4util_add32(&hist->underflow, val);
    else if (bin >= hist->bins)
        l4util_add32(&hist->overflow, val);
    else
        l4util_add32(&(hist->data[bin]), val);

    if (x < hist->val_min)
        hist->val_min = x;
    if (x > hist->val_max)
        hist->val_max = x;
    hist->val_sum += val * x;
    l4util_add32(&hist->val_count, val);
}

void rt_mon_hist_insert_data_int(rt_mon_histogram_t * hist, rt_mon_time_t x,
				 unsigned int val)
{
    int bin;

    bin = (x - hist->low_int) * hist->bins / hist->bin_int_scaler;

    // use atomic data access, such that it can be used from several places
    if (bin < 0)
        l4util_add32(&hist->underflow, val);
    else if (bin >= hist->bins)
        l4util_add32(&hist->overflow, val);
    else
        l4util_add32(&(hist->data[bin]), val);

    if (x < hist->val_min)
        hist->val_min = x;
    if (x > hist->val_max)
        hist->val_max = x;
    hist->val_sum += val * x;
    l4util_add32(&hist->val_count, val);
}

void rt_mon_hist_insert_lost(rt_mon_histogram_t * hist, unsigned int count)
{
    l4util_add32(&hist->lost, count);
}

int inline rt_mon_hist_get_data(rt_mon_histogram_t * hist, rt_mon_time_t x)
{
    int bin;

    bin = (int)own_floor(((x - hist->low) / hist->bin_size));
    if (bin < 0 || bin >= hist->bins)
        return 0;
    else
        return hist->data[bin];
}

inline void rt_mon_hist_start(rt_mon_histogram_t * hist)
{
    RT_MON_GET_TIME(hist->clock, hist->start);
}

inline void rt_mon_hist_end(rt_mon_histogram_t * hist)
{
    rt_mon_time_t diff;

    RT_MON_GET_TIME(hist->clock, hist->end);

    diff = hist->end - hist->start;
    rt_mon_hist_insert_data(hist, diff, 1);
}

inline rt_mon_time_t rt_mon_hist_measure(rt_mon_histogram_t * hist)
{
    rt_mon_time_t time;

    RT_MON_GET_TIME(hist->clock, time);

    return time;
}

void
rt_mon_hist_reset(rt_mon_histogram_t * hist)
{
    hist->underflow = 0;
    hist->overflow  = 0;
    hist->lost      = 0;
    bzero(&hist->data, hist->bins * sizeof(hist->data[0]));
    hist->val_min   = LLONG_MAX;
    hist->val_max   = LLONG_MIN;
    hist->val_sum   = 0;
    hist->val_count = 0;

    /* do the calculation for the integer-scalers here */
    hist->low_int   = hist->low;
    hist->bin_int_scaler = hist->high-hist->low;
}

void rt_mon_hist_free(rt_mon_histogram_t * hist)
{
    int id;

    id = hist->id;
    l4rm_detach(hist);
    rt_mon_unregister_ds(id);
}

void rt_mon_hist_dump(rt_mon_histogram_t * hist)
{
    int i;

    printf("#DS: id = %d, manager = "l4util_idfmt"\n",
           hist->ds.id, l4util_idstr(hist->ds.manager));
    printf("#Name: %s\n", hist->name);
    printf("#Low: %g\n", hist->low);
    printf("#High: %g\n", hist->high);
    printf("#Bin size: %g\n", hist->bin_size);
    printf("#Bins: %d\n", hist->bins);
    printf("#Underflow: %d\n", hist->underflow);
    printf("#Overflow: %d\n", hist->overflow);
    printf("#Lost: %d\n", hist->lost);
    for(i = 0; i < hist->bins; i++)
    {
        printf("%g\t%d\n",
	       hist->low + hist->bin_size / 2 + i * hist->bin_size,
               hist->data[i]);
    }
}

double rt_mon_hist_avg(rt_mon_histogram_t * hist)
{
    int i;
    double avg = 0;
    int val = 0;

    for (i = 0; i < hist->bins; i++)
    {
        avg += (hist->low + hist->bin_size / 2 + i * hist->bin_size) *
            hist->data[i];
        val += hist->data[i];
    }
    return avg / val;
}
