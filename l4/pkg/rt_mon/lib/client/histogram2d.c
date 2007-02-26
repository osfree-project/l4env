/**
 * \file   rt_mon/lib/client/histogram2d.c
 * \brief  Some functions for 2d histograms.
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
#include <l4/rt_mon/histogram2d.h>

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

#include "comm.h"

double own_floor(double x);

rt_mon_histogram2d_t *
rt_mon_hist2d_create(double low[2], double high[2], int bins[2], int layers,
                     const char * name,
                     const char * unit_ord,
                     const char * unit_abs1, const char * unit_abs2,
                     int clock)
{
    rt_mon_histogram2d_t * temp;
    l4dm_dataspace_t       ds;
    int                    ret, i;
    double                 avg;
    rt_mon_histogram_t   * temp_avg;

    if (layers < 1)
        return 0;

    rt_mon_calibrate_clock(clock);

    temp = l4dm_mem_ds_allocate_named(
        sizeof(rt_mon_histogram2d_t) +
        sizeof(temp->data[0]) * bins[0] * bins[1] * layers,
        L4DM_PINNED + L4RM_MAP, name, &ds);
    if (! temp)
        return NULL;

    temp->type = RT_MON_TYPE_HISTOGRAM2D;
    if (name)
    {
        strncpy(temp->name, name, RT_MON_NAME_LENGTH - 1);
        temp->name[RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
        temp->name[0] = 0;

    if (unit_ord)
    {
        strncpy(temp->unit[0], unit_ord, RT_MON_NAME_LENGTH - 1);
        temp->unit[0][RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
    {
        strncpy(temp->unit[0], "us", RT_MON_NAME_LENGTH - 1);
        temp->unit[0][RT_MON_NAME_LENGTH - 1] = 0;
    }

    if (unit_abs1)
    {
        strncpy(temp->unit[1], unit_abs1, RT_MON_NAME_LENGTH - 1);
        temp->unit[1][RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
    {
        strncpy(temp->unit[1], "apples", RT_MON_NAME_LENGTH - 1);
        temp->unit[1][RT_MON_NAME_LENGTH - 1] = 0;
    }

    if (unit_abs2)
    {
        strncpy(temp->unit[2], unit_abs2, RT_MON_NAME_LENGTH - 1);
        temp->unit[2][RT_MON_NAME_LENGTH - 1] = 0;
    }
    else
    {
        strncpy(temp->unit[2], "eggs", RT_MON_NAME_LENGTH - 1);
        temp->unit[2][RT_MON_NAME_LENGTH - 1] = 0;
    }

    temp->ds          = ds;
    temp->low[0]      = low[0];
    temp->low[1]      = low[1];
    temp->high[0]     = high[0];
    temp->high[1]     = high[1];
    temp->bin_size[0] = (high[0] - low[0]) / bins[0];
    temp->bin_size[1] = (high[1] - low[1]) / bins[1];
    temp->bins[0]     = bins[0];
    temp->bins[1]     = bins[1];
    temp->layers      = layers;
    temp->clock       = clock;
    rt_mon_hist2d_reset(temp);

    // measuring overhead to take time
    temp_avg = malloc(sizeof(rt_mon_histogram_t) +
                      sizeof(temp->data[0]) * 1000);
    temp_avg->low = 0;
    temp_avg->high = 1000;
    temp_avg->bins = 1000;
    temp_avg->bin_size  = (temp_avg->high - temp_avg->low) / temp_avg->bins;
    temp_avg->underflow = 0;
    temp_avg->overflow = 0;
    temp_avg->type = RT_MON_TYPE_HISTOGRAM;
    bzero(&temp_avg->data, temp_avg->bins * sizeof(temp_avg->data[0]));
    temp_avg->clock = clock;
    for (i = 0; i < 100; i++)
    {
        rt_mon_hist_start(temp_avg);
        rt_mon_hist_end(temp_avg);
    }
    avg = rt_mon_hist_avg(temp_avg);
    LOG("avg * 1000000 = %d", (int)(avg * 1000000));
    LOG("ud-/ovflow = <%d, %d>", temp_avg->underflow, temp_avg->overflow);
    temp->time_ovh = avg;
    free(temp_avg);

    ret = rt_mon_register_ds(ds, name);
    if (ret < 0)
    {
        LOG("something wrong with registering ds");
    }
    temp->id = ret;

    return temp;
}

inline void
rt_mon_hist2d_insert_data(rt_mon_histogram2d_t * hist, rt_mon_time_t x[2],
                          int l, unsigned int val)
{
    int bin[2];

    bin[0] = (int)own_floor(((x[0] - hist->low[0]) / hist->bin_size[0]));
    bin[1] = (int)own_floor(((x[1] - hist->low[1]) / hist->bin_size[1]));

    // use atomic data access, such that it can be used from several places
    // fixme: two var. for storing 8 possible under-/overflows is a
    //        little bit to less, underflow is prefered in the current
    //        implementation
    if (bin[0] < 0 || bin[1] < 0)
        l4util_add32(&hist->underflow, val);
    else if (bin[0] >= hist->bins[0] || bin[1] >= hist->bins[1])
        l4util_add32(&hist->overflow, val);
    else
        l4util_add32(&(hist->data[l * hist->bins[0] * hist->bins[1] +
                                  bin[1] * hist->bins[0] + bin[0]]), val);
}

inline int
rt_mon_hist2d_get_data(rt_mon_histogram2d_t * hist, rt_mon_time_t x[2],
                       int l)
{
    int bin[2];

    bin[0] = (int)own_floor(((x[0] - hist->low[0]) / hist->bin_size[0]));
    bin[1] = (int)own_floor(((x[1] - hist->low[1]) / hist->bin_size[1]));
    if (bin[0] < 0 || bin[1] < 0 ||
        bin[0] >= hist->bins[0] || bin[1] >= hist->bins[1])
        return 0;
    else
        return hist->data[l * hist->bins[0] * hist->bins[1] +
                          bin[1] * hist->bins[0] + bin[0]];
}

inline void rt_mon_hist2d_start(rt_mon_histogram2d_t * hist)
{
    RT_MON_GET_TIME(hist->clock, hist->start);
}

inline void
rt_mon_hist2d_end(rt_mon_histogram2d_t * hist, rt_mon_time_t y, int l)
{
    rt_mon_time_t diff[2];

    RT_MON_GET_TIME(hist->clock, hist->end);

    diff[0] = hist->end - hist->start;
    diff[1] = y;
    rt_mon_hist2d_insert_data(hist, diff, l, 1);
}

inline rt_mon_time_t rt_mon_hist2d_measure(rt_mon_histogram2d_t * hist)
{
    rt_mon_time_t time;

    RT_MON_GET_TIME(hist->clock, time);

    return time;
}

void rt_mon_hist2d_free(rt_mon_histogram2d_t * hist)
{
    rt_mon_unregister_ds(hist->id);
    l4rm_detach(hist);
}

void rt_mon_hist2d_reset(rt_mon_histogram2d_t * hist)
{
    hist->underflow   = 0;
    hist->overflow    = 0;
    bzero(&hist->data, hist->bins[0] * hist->bins[1] * hist->layers *
          sizeof(hist->data[0]));
}

void rt_mon_hist2d_dump(rt_mon_histogram2d_t * hist)
{
    int i, j, l;

    printf("#DS: id = %d, manager = "l4util_idfmt"\n",
           hist->ds.id, l4util_idstr(hist->ds.manager));
    printf("#Name: %s\n", hist->name);
    printf("#Low: <%g, %g>\n", hist->low[0], hist->low[1]);
    printf("#High: <%g, %g>\n", hist->high[0], hist->high[1]);
    printf("#Bin size: <%g, %g>\n", hist->bin_size[0], hist->bin_size[1]);
    printf("#Bins: <%d, %d>\n", hist->bins[0], hist->bins[1]);
    printf("#Underflow: %d\n", hist->underflow);
    printf("#Overflow: %d\n", hist->overflow);
    for(j = 0; j < hist->bins[1]; j++)
    {
        for(i = 0; i < hist->bins[0]; i++)
        {
            printf("%g\t%g", hist->low[0] + hist->bin_size[0] / 2 +
                             i * hist->bin_size[0],
                             hist->low[1] + hist->bin_size[1] / 2 +
                             j * hist->bin_size[1]);
            for (l = 0; l < hist->layers; l++)
            {
                printf("\t%d", hist->data[l * hist->bins[0] * hist->bins[1] +
                                          j * hist->bins[0] + i]);
            }
            printf("\n");
        }
        printf("\n");  // empty line for gnuplot 3d output
    }
}

void rt_mon_hist2d_avg(rt_mon_histogram2d_t * hist, double (* avg)[2], int l)
{
    int i, j;
    int val = 0;

    (*avg)[0] = 0;
    (*avg)[1] = 0;
    for (j = 0; j < hist->bins[1]; j++)
        for (i = 0; i < hist->bins[0]; i++)
        {
            (*avg)[0] += (hist->low[0] + hist->bin_size[0] / 2 +
                          i * hist->bin_size[0]) *
                          hist->data[l * hist->bins[0] * hist->bins[1] +
                                     j * hist->bins[0] + i];
            (*avg)[1] += (hist->low[1] + hist->bin_size[1] / 2 +
                          j * hist->bin_size[1]) *
                          hist->data[l * hist->bins[0] * hist->bins[1] +
                                     j * hist->bins[0] + i];
            val += hist->data[l * hist->bins[0] * hist->bins[1] + i];
        }
    (*avg)[0] /= val;
    (*avg)[1] /= val;
}
