/**
 * \file   rt_mon/include/types.h
 * \brief  Global types for rt_mon.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_TYPES_H_
#define __RT_MON_INCLUDE_TYPES_H_

#include <l4/dm_generic/types.h>

#include <l4/rt_mon/defines.h>


/*********************************************************************
 * Time sources
 *********************************************************************/

// reserve 0 as illegal value !
#define RT_MON_ILLEGAL_TIME         0 ///< Do not use this, reserved!
#define RT_MON_TSC_TIME             2 /**< Directly use cpu ticks
                                       **  - Absolute time since booting
                                       **/
#define RT_MON_TSC_TO_US_TIME       3 /**< Same as RT_MON_TSC_TIME but convert
                                       **  ticks to 탎 via l4_tsc_to_us()
                                       **  - Absolute time since booting
                                       **/
#define RT_MON_THREAD_TIME          1 /**< Accumulated thread time via
                                       **  fiasco_get_cputime() (in 탎)
                                       **  - Accumulated relative thread time
                                       **/
#define RT_MON_FAST_THREAD_TIME     4 /**< Accumulated thread time via
                                       **  l4util_thread_time() (in ticks)
                                       **  - Accumulated relative thread time
                                       **  - Will only work in Fiasco with
                                       **    fine_grained_cpu_time
                                       **/
#define RT_MON_FAST_THREAD_TIME_TSC 5 /**< Same as RT_MON_FAST_THREAD_TIME but
                                       **  convert ticks to 탎 via
                                       **  l4_tsc_to_탎()
                                       **/

/*********************************************************************
 * Simple types
 *********************************************************************/

typedef signed long long rt_mon_time_t;


/*********************************************************************
 * Structure types
 *********************************************************************/

#define RT_MON_TYPE_UNDEFINED   0
#define RT_MON_TYPE_HISTOGRAM   1
#define RT_MON_TYPE_HISTOGRAM2D 2
#define RT_MON_TYPE_EVENT_LIST  3
#define RT_MON_TYPE_SHARED_LIST 4
#define RT_MON_TYPE_SCALAR      5

/* Allocate with:
 *   'hist = malloc(sizeof(rt_mon_histogram_t) +
 *                  sizeof(rt_mon_histogram_t.data[0]) * bins * layers);'
 */
typedef struct
{
    int           type;      // identifies the type of struct
    int           id;        // identifies the instance
    int           clock;     // clock type to use (e.g. TSC, process time, ...)
    char          name[RT_MON_NAME_LENGTH];
    char          unit[2][RT_MON_NAME_LENGTH]; // time units (e.g., 'ms', '탎')
    l4dm_dataspace_t ds;     // points to self
    double        low;       // lower bound for histogram data (inclusive)
    double        high;      // upper bound for histogram data (exclusive)

    long long	  low_int;   // lower bound as an integer
    long long	  bin_int_scaler; // scaler for integer calculation

    double        bin_size;  // precomputed multiplier
    unsigned int  time_ovh;  // overhead for time measurements
    int           bins;      // number of bins in histogram
    unsigned int  underflow; // number of underflows (value below low) occurred
    unsigned int  overflow;  // number of overflows (value above high) occurred
    unsigned int  lost;	     // number of lost data

    long long     val_min;   // minimum value put in so far
    long long     val_max;   // maximum value put in so far
    unsigned      val_count; // number of values in the histogram
    long long     val_sum;   // sum of the values in the histogram

    rt_mon_time_t start;     // temp. beginning of duration
    rt_mon_time_t end;       // temp. end of duration
    unsigned int  data[0];   // data points
}  rt_mon_histogram_t;

/* Allocate with:
 *   'hist = malloc(sizeof(rt_mon_histogram2d_t) +
 *                  sizeof(rt_mon_histogram2d_t.data[0]) * bins[0] * bins[1]);'
 */
typedef struct
{
    int           type;      // identifies the type of struct
    int           id;        // identifies the instance
    int           clock;     // clock type to use (e.g. TSC, process time, ...)
    char          name[RT_MON_NAME_LENGTH];
    char          unit[3][RT_MON_NAME_LENGTH]; // time units (e.g., 'ms', '탎')
    l4dm_dataspace_t ds;     // points to self
    double        low[2];    // lower bound for histogram data (inclusive)
    double        high[2];   // upper bound for histogram data (exclusive)
    double        bin_size[2]; // precomputed multiplier
    unsigned int  time_ovh;  // overhead for time measurements
    int           bins[2];   // number of bins in histogram
    int           layers;    // number of layers in the histogram
    unsigned int  underflow; // number of underflows (value below low) occurred
    unsigned int  overflow;  // number of overflows (value above high) occurred
    rt_mon_time_t start;     // temp. beginning of duration
    rt_mon_time_t end;       // temp. end of duration
    unsigned int  data[0];   // data points
}  rt_mon_histogram2d_t;


/* Allocate with:
 *     'list = malloc(sizeof(rt_mon_event_list_t) + sizeof(event_t) * events);'
 * Specific event types are to be defined elsewhere
 */
typedef struct
{
    int              type;         // identifies the type of struct
    int              id;           // identifies the instance
    int              clock;        // clock type to use (e.g. TSC, ...)
    char             name[RT_MON_NAME_LENGTH];
    char             unit[RT_MON_NAME_LENGTH]; // time units (e.g., 'ms', '탎')
    l4dm_dataspace_t ds;           // points to self
    int              event_type;   // type of all events
    int              event_size;   // size of a single event in bytes
    unsigned int     insert;       // position of next insertion
    unsigned int     remove;       // position of next removal
    int              max_events;   // maximal number of events in list
    int              overruns;     // # of events lost due to memory shortage
    int              data[0];      // the events to be stored, must be casted
}  rt_mon_event_list_t;

typedef struct
{
    int              type;         // identifies the type of struct
    int              id;           // identifies the instance
    int              clock;        // clock type to use (e.g. TSC, ...)
    char             name[RT_MON_NAME_LENGTH];
    char             unit[RT_MON_NAME_LENGTH]; // time units (e.g., 'ms', '탎')
    l4dm_dataspace_t ds;           // points to self

    rt_mon_time_t    low;          // lower bound for valid range
    rt_mon_time_t    high;         // upper bound for valid range
    rt_mon_time_t    val_min;      // minimum value put in so far
    rt_mon_time_t    val_max;      // maximum value put in so far
    long long        val_count;    // number of values inserted
    rt_mon_time_t    val_sum;      // sum of all values inserted
    rt_mon_time_t    data;         // finally the data itself
}  rt_mon_scalar_t;


/* Generic data type to be monitored, useful for type detection.
 * Union of all structure types
 */
typedef union
{
    int                       type;
    rt_mon_histogram_t        hist;
    rt_mon_histogram2d_t      hist2d;
    rt_mon_event_list_t       list;
    rt_mon_scalar_t           scalar;
}  rt_mon_data;


/*********************************************************************
 * Event types
 *********************************************************************/

#define RT_MON_EVTYPE_UNDEFINED  0
#define RT_MON_EVTYPE_BASIC      1
#define RT_MON_EVTYPE_VIDEO      2

typedef struct  // RT_MON_EVTYPE_BASIC
{
    int           id;     // event type, defined elsewhere
    rt_mon_time_t time;   // time stamp of events occurrence, or duration
    int           data;   // a little bit of event-specific data
}  rt_mon_basic_event_t;

typedef struct  // RT_MON_EVTYPE_VIDEO
{
    int           id;     // frame type
    rt_mon_time_t time;   // processing time
    int           data1;
    int           data2;
    int           data3;
}  rt_mon_video_event_t;

typedef struct
{
    int    committed;     // flag if packet is committed (== 1) or not (== 0)
    int    data[0];       // place holder for actual event
}  rt_mon_event_container_t;


/*********************************************************************
 * Other stuff
 *********************************************************************/

// transfer type for idl
typedef struct
{
    int  id;
    char name[RT_MON_NAME_LENGTH];
} rt_mon_dss;

#endif
