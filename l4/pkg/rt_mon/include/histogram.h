/**
 * \file   rt_mon/include/histogram.h
 * \brief  Some functions to work with histograms.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_HISTOGRAM_H_
#define __RT_MON_INCLUDE_HISTOGRAM_H_

#include <l4/sys/compiler.h>
#include <l4/rt_mon/types.h>

EXTERN_C_BEGIN

/**
 * @brief Creates a new histogram, allocates memory for it and
 *        registers it at the coordinator
 *
 * @param low      lower bound for histogram
 * @param high     upper bound for histogram
 * @param bins     number of bins to use between low and high
 * @param name     the name of this histogram
 * @param unit_abs measurement unit for the abscissa
 * @param unit_ord measurement unit for the ordinate
 * @param clock    type of time source to use for this sensor
 *
 * @return pointer to newly malloc'ed, registered and initialized
 *         histogram, otherwise
 *         - NULL in case of error (like out of memory)
 */
rt_mon_histogram_t *
rt_mon_hist_create(double low, double high, int bins,
                   const char * name,
                   const char * unit_abs, const char * unit_ord, int clock);

/**
 * @brief Inserts an amount into the histogram
 *
 * @param hist histogram to insert data to
 * @param x    point on the abscissa
 * @param val  value to add to the specified point
 */
void
rt_mon_hist_insert_data(rt_mon_histogram_t * hist, rt_mon_time_t x,
                        unsigned int val);

/**
 * @brief Inserts an amount into the histogram, integer version
 *
 * This function behaves like rt_mon_hist_insert_data(), but uses
 * only integer operations internally.
 *
 * \bug for non-integer lower bounds, the bin needs an additional offset,
 *      which is not implemented as of today.
 */
void
rt_mon_hist_insert_data_int(rt_mon_histogram_t * hist, rt_mon_time_t x,
			    unsigned int val);


/**
 * @brief Take note of lost data points
 *
 * @param hist	histogram to insert data to
 * @param count	number of lost events
 */
void
rt_mon_hist_insert_lost(rt_mon_histogram_t * hist, unsigned int count);

/**
 * @brief Get a data point from a histogram
 *
 * @param hist  histogram to get data from
 * @param x     point to get data from
 *
 * @return value from histogram
 */
inline int
rt_mon_hist_get_data(rt_mon_histogram_t * hist, rt_mon_time_t x);

/**
 * @brief Destroys a histogram, unregisters it at the coordinator and
 *        frees its memory.
 *
 * @param hist histogram to free
 */
void
rt_mon_hist_free(rt_mon_histogram_t * hist);

/**
 * @brief completely re-initialize histogram
 *
 * Reset all the collected date, i.e.:
 *  - underflow and overflow counter
 *  - current minimum and maximum
 *  - accumulated sum and count
 *  - histogram content itself
 *
 * @param hist histogram to reset
 */
void
rt_mon_hist_reset(rt_mon_histogram_t * hist);

/**
 * @brief Dump the content of a histogram using printf
 *
 * @param hist histogram to dump
 */
void
rt_mon_hist_dump(rt_mon_histogram_t * hist);

/**
 * @brief Set start point in time for a duration measurement
 *
 * @param hist histogram to operate on
 */
inline void
rt_mon_hist_start(rt_mon_histogram_t * hist);

/**
 * @brief Set end point in time for a duration measurement, insert
 *        resulting duration into the histogram
 *
 * @param hist histogram to operate on
 */
inline void
rt_mon_hist_end(rt_mon_histogram_t * hist);

/**
 * @brief Get a current time stamp for a histogram
 *
 * @note  The histogram is not modified.
 *
 * @param hist histogram to operate on
 *
 * @return measured time stamp
 */
inline rt_mon_time_t
rt_mon_hist_measure(rt_mon_histogram_t * hist);

/**
 * @brief Compute the average value for a histogram
 *
 * @param hist histogram to operate on
 *
 * @return average value for the histogram
 */
double
rt_mon_hist_avg(rt_mon_histogram_t * hist);

EXTERN_C_END

#endif
