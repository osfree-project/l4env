/**
 * \file   rt_mon/include/clock.h
 * \brief  macros for getting time values
 *
 * \date   11/15/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_CLOCK_H_
#define __RT_MON_INCLUDE_CLOCK_H_

#include <l4/rt_mon/types.h>

#include <l4/sigma0/kip.h>
#include <l4/util/rdtsc.h>
#include <l4/util/thread_time.h>

#define RT_MON_GET_TIME(clock_type, time_val)                             \
do                                                                        \
{                                                                         \
    switch (clock_type)                                                   \
    {                                                                     \
    case RT_MON_THREAD_TIME:                                              \
    {                                                                     \
        int           ret;                                                \
        l4_threadid_t next;                                               \
        l4_umword_t   prio;                                               \
        /* should not fail, as thread always exists */                    \
        ret = fiasco_get_cputime(l4_myself(), &next, &(time_val), &prio); \
        break;                                                            \
    }                                                                     \
    case RT_MON_TSC_TIME:                                                 \
        (time_val) = l4_rdtsc();                                          \
        break;                                                            \
    case RT_MON_TSC_TO_US_TIME:                                           \
        (time_val) = l4_tsc_to_us(l4_rdtsc());                            \
        break;                                                            \
    case RT_MON_FAST_THREAD_TIME:                                         \
        (time_val) = l4_tsc_to_us(l4util_thread_time(l4sigma0_kip()));    \
        break;                                                            \
    case RT_MON_FAST_THREAD_TIME_TSC:                                     \
        (time_val) = l4util_thread_time(l4sigma0_kip());                  \
        break;                                                            \
    default:                                                              \
        LOG("corrupted CLOCKTYPE");                                       \
    }                                                                     \
} while (0)


/** 
 * @brief Calibrates scalers etc. for the use of the corresponding clock
 * 
 * @param clock clock type to calibrate for
 */
L4_INLINE void rt_mon_calibrate_clock(int clock);
L4_INLINE void rt_mon_calibrate_clock(int clock)
{
    if (clock == RT_MON_TSC_TO_US_TIME || clock == RT_MON_FAST_THREAD_TIME)
        if (l4_scaler_tsc_to_us == 0)  // calibrate on demand
            l4_calibrate_tsc();
}

#endif
