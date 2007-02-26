/**
 * \file   rt_mon/include/event_list.h
 * \brief  Some functions to work with event lists.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_EVENT_LIST_H_
#define __RT_MON_INCLUDE_EVENT_LIST_H_

#include <l4/rt_mon/types.h>

EXTERN_C_BEGIN

/**
 * @brief Creates a new event_list, allocates memory for it and
 *        registers it at the coordinator
 * 
 * @param event_size  Size in bytes of all events in this event_list
 * @param event_type  typecode for the events, can be used to
 *                    distinguish lists with different event types in
 *                    monitors
 * @param event_count number of events to allocate memory buffer for
 * @param name        name of this event_list
 * @param unit        measurement unit of data transported in the
 *                    events in this event_list
 * @param clock       type of time source to use for this sensor
 * @param shared      boolean flag whether to use a shared datastructure
 * 
 * @return pointer to newly malloc'ed, registered and initialized
 *         event_list, otherwise
 *         - NULL in case of error (like out of memory)
 */
rt_mon_event_list_t *
rt_mon_list_create(int event_size, int event_type, int event_count,
                   const char * name, const char * unit, int clock,
                   int shared);

/**
 * @brief Destroys an event_list, unregisters it at the coordinator
 *        and frees its memory.
 * 
 * @param list event_list to free
 */
void
rt_mon_list_free(rt_mon_event_list_t * list);

/**
 * @brief Insert event into event_list
 * 
 * @param list  event_list to insert to
 * @param event event to insert
 */
inline void
rt_mon_list_insert(rt_mon_event_list_t * list, const void * event);

/**
 * @brief Get a time stamp using the timesource specified in list
 * 
 * @param list event_list to get the time source type from
 * 
 * @return the time measured
 */
inline rt_mon_time_t
rt_mon_list_measure(rt_mon_event_list_t * list);

/**
 * @brief Get (move) an event from list
 * 
 * @param list event_list to get the event from
 * @param event pointer to event struct to move event to
 * 
 * @return 0 on success,
 *         - otherwise: no event available
 */
inline int
rt_mon_list_remove(rt_mon_event_list_t * list, void * event);

/** 
 * @brief Dump the content of an event_list using printf
 * 
 * @param list event_list to dump
 */
void
rt_mon_list_dump(rt_mon_event_list_t * list);

EXTERN_C_END

#endif
