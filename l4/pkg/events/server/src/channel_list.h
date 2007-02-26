
/*!
 * \file   events/server/src/channel_list.h
 *
 * \brief  functions and makros for the channel-list
 *
 * \date   19/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "l4/log/l4log.h"

#include "globals.h"
#include "types.h"

#ifndef __L4EVENTS_CHANNEL_LIST
#define __L4EVENTS_CHANNEL_LIST


/* this is the list of registered channels */
channel_item_t*	channel_list = NULL;

#define link_item_into_channel_list(channel_list, channel)		\
	LOGd(DEBUGLVL(3), "link channel into channel-list");		\
	channel->next_item = channel_list;				\
	channel_list = channel
     
#define unlink_item_from_channel_list(curr_channel, prev_channel)	\
	LOGd(DEBUGLVL(3), "unlink channel from channel-list");		\
	if (prev_channel != NULL)					\
	  prev_channel->next_item = curr_channel->next_item;		\
	else								\
	  channel_list = curr_channel->next_item

#define link_task_to_channel(curr_channel, pr, curr_task_ref)		\
	curr_task_ref->next_ref = curr_channel->first_task_ref[pr];	\
	curr_channel->first_task_ref[pr] = curr_task_ref;		\
	curr_channel->registered_tasks++

#define unlink_task_from_channel(curr_channel, pr, 			\
    				 curr_task_ref, prev_task_ref)		\
	if (prev_task_ref != NULL)					\
	  prev_task_ref->next_ref = curr_task_ref->next_ref;		\
	else								\
	  curr_channel->first_task_ref[pr] = curr_task_ref->next_ref;	\
	curr_channel->registered_tasks--
     
/* searching for the channel event_ch in the channel_list */
#define find_channel(event_ch, curr_channel, prev_channel)		\
  do									\
    {									\
      curr_channel = channel_list;					\
      prev_channel = NULL;						\
      while (curr_channel != NULL)					\
        {								\
	  if (curr_channel->event_ch == event_ch)			\
	    break;							\
	  prev_channel = curr_channel;					\
	  curr_channel = curr_channel->next_item;			\
    	}								\
      LOGd(DEBUGLVL(3), "channel:%d %sfound", 				\
	   event_ch, curr_channel ? "" : "NOT "); 			\
    }									\
  while (0)

/* searching for a registered task in a channel */
#define find_task_for_channel(curr_channel, task, pr,			\
			      curr_task_ref, prev_task_ref)		\
  do									\
    {									\
      for (pr = L4EVENTS_MAX_PRIORITY; pr >= 0; pr--)			\
	{								\
	  prev_task_ref = NULL;						\
	  curr_task_ref = curr_channel->first_task_ref[pr];		\
	  while (curr_task_ref != NULL)					\
	    {								\
	      if (curr_task_ref->task_item->taskid == task)		\
		break;							\
	      prev_task_ref = curr_task_ref;				\
	      curr_task_ref = curr_task_ref->next_ref;			\
	    }								\
	  if (curr_task_ref &&						\
	      curr_task_ref->task_item->taskid == task)			\
	    break;							\
	}								\
      LOGd(DEBUGLVL(3), "task:%d %sfound for channel:%d", task,		\
    	   curr_task_ref ? "" : "NOT ", curr_channel->event_ch);	\
    }									\
  while (0)

#endif
