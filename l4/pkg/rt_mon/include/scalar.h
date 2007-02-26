/**
 * \file   rt_mon/include/scalar.h
 * \brief  Functions to operate on the most basic sensors type: scalars.
 *
 * \date   11/30/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_SCALAR_H_
#define __RT_MON_INCLUDE_SCALAR_H_

#include <l4/rt_mon/types.h>

EXTERN_C_BEGIN

/**
 * @brief Creates a new event_list, allocates memory for it and
 *        registers it at the coordinator
 * 
 * @param low   lower bound for valid values
 * @param high  lower bound for valid values
 * @param name  name of this scalar
 * @param unit  measurement unit of data transported in the scalar
 * @param clock type of time source to use
 * 
 * @return pointer to newly malloc'ed, registered and initialized
 *         event_list, otherwise
 *         - NULL in case of error (like out of memory)
 */
rt_mon_scalar_t *
rt_mon_scalar_create(rt_mon_time_t low, rt_mon_time_t high,
                     const char * name, const char * unit, int clock);

/**
 * @brief Destroys a scalar, unregisters it at the coordinator
 *        and frees its memory.
 * 
 * @param scalar scalar to free
 */
void
rt_mon_scalar_free(rt_mon_scalar_t * scalar);

/**
 * @brief Insert value into scalar
 * 
 * @param scalar scalar to operate on
 * @param data   value to insert
 */
inline void
rt_mon_scalar_insert(rt_mon_scalar_t * scalar, rt_mon_time_t data);

/**
 * @brief Get a time stamp using the timesource specified in scalar
 * 
 * @param scalar scalar to get the time source type from
 * 
 * @return the time measured
 */
inline rt_mon_time_t
rt_mon_scalar_measure(rt_mon_scalar_t * scalar);

/**
 * @brief Read current value from scalar
 * 
 * @param scalar scalar to get the data from
 * 
 * @return value read
 */
inline rt_mon_time_t
rt_mon_scalar_read(rt_mon_scalar_t * scalar);

/** 
 * @brief Dump the content of the scalar using printf
 * 
 * @param scalar scalar to dump
 */
void
rt_mon_scalar_dump(rt_mon_scalar_t * scalar);

EXTERN_C_END

#endif
