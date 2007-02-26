/**
 * \file   rt_mon/include/histogram2d.h
 * \brief  Some functions to work with 2d histograms.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_HISTOGRAM2D_H_
#define __RT_MON_INCLUDE_HISTOGRAM2D_H_


#include <l4/rt_mon/types.h>

EXTERN_C_BEGIN

/**
 * @brief Creates a new histogram, allocates memory for it and
 *        registers it at the coordinator
 * 
 * @param low       lower bound for both abscissae
 * @param high      upper bound for both abscissae
 * @param bins      number of bins to use between low[0] and high[0]
 *                  resp. low[0] and high[0]
 * @param layers    number of layers to allocate for histogram
 * @param name      name of the histogram
 * @param unit_ord  measurement units for the ordinate
 * @param unit_abs1 measurement units for the first abscissa
 * @param unit_abs2 measurement units for the second abscissa
 * @param clock     type of time source to use for this sensor
 * 
 * @return pointer to newly malloc'ed, registered and initialized
 *         histogram, otherwise
 *         - NULL in case of error (like out of memory)
 */
rt_mon_histogram2d_t *
rt_mon_hist2d_create(double low[2], double high[2], int bins[2], int layers,
                     const char * name,
                     const char * unit_ord,
                     const char * unit_abs1, const char * unit_abs2,
                     int clock);

/**
 * @brief Inserts an amount into the histogram
 * 
 * @param hist  histogram to insert data to
 * @param x     point on both abscissae
 * @param layer layer to add value to
 * @param val   value to add to the specified point
 */
void
rt_mon_hist2d_insert_data(rt_mon_histogram2d_t * hist, rt_mon_time_t x[2],
                          int layer, unsigned int val);

/**
 * @brief Get a data point from a histogram
 * 
 * @param hist  histogram to get data from
 * @param x     point to get data from
 * @param layer layer to get data from
 * 
 * @return value from histogram
 */
int
inline rt_mon_hist2d_get_data(rt_mon_histogram2d_t * hist, rt_mon_time_t x[2],
                              int layer);

/**
 * @brief Destroys a histogram, unregisters it at the coordinator and
 *        frees its memory.
 * 
 * @param hist histogram to free
 */
void
rt_mon_hist2d_free(rt_mon_histogram2d_t * hist);

/** 
 * @brief completely re-initialize histogram
 * 
 * Reset all the collected date, i.e.:
 *  - underflow and overflow counter
 *  - histogram content itself
 *
 * @param hist histogram to reset
 */
void
rt_mon_hist2d_reset(rt_mon_histogram2d_t * hist);


/**
 * @brief Dump the content of a histogram using printf
 * 
 * @param hist histogram to dump
 */
void
rt_mon_hist2d_dump(rt_mon_histogram2d_t * hist);

/**
 * @brief Set start point in time for a duration measurement
 * 
 * @param hist histogram to operate on
 */
inline void
rt_mon_hist2d_start(rt_mon_histogram2d_t * hist);

/**
 * @brief Set end point in time for a duration measurement, insert
 *        resulting duration into the histogram
 * 
 * @param hist  histogram to operate on
 * @param y     point an second abscissa
 * @param layer layer to put to to
 */
inline void
rt_mon_hist2d_end(rt_mon_histogram2d_t * hist, rt_mon_time_t y, int layer);


/**
 * @brief Get a current time stamp for a histogram
 * 
 * @note  The histogram is not modified.
 *
 * @param hist histogram to operate on
 */
inline rt_mon_time_t
rt_mon_hist2d_measure(rt_mon_histogram2d_t * hist);

/**
 * @brief Compute the average values for a 2d histogram
 * 
 * @param  hist  histogram to operate on
 * @retval avg   average values for both abscissae dimensions
 * @param  layer layer to work on
 */
void
rt_mon_hist2d_avg(rt_mon_histogram2d_t * hist, double (* avg)[2], int layer);

EXTERN_C_END

#endif
