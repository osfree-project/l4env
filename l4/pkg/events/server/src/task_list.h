/*!
 * \file   events/server/src/task_list.h
 *
 * \brief  functions and makros for the task-list
 *
 * \date   03/19/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "l4/log/l4log.h"

#include "globals.h"
#include "types.h"

#ifndef __L4EVENTS_TASK_LIST
#define __L4EVENTS_TASK_LIST


/* this is the list of registered tasks */
static task_item_t*		task_list = NULL;


#define link_task_into_task_list(task_list, task)			\
	task->next_item = task_list;					\
	task_list = task

#define unlink_task_from_task_list(task_list, curr_task, prev_task)	\
	if (prev_task != NULL)						\
	  prev_task->next_item = curr_task->next_item;			\
	else								\
	  task_list = curr_task->next_item

#define link_channel_to_task(curr_task, curr_channel_ref)		\
	curr_channel_ref->next_ref = curr_task->first_channel_ref;	\
	curr_task->first_channel_ref = curr_channel_ref;		\
	curr_task->registered_channels++
    
#define unlink_channel_from_task(curr_task, curr_channel_ref,		\
    				 prev_channel_ref)			\
	if (prev_channel_ref != NULL)					\
	  prev_channel_ref->next_ref = curr_channel_ref->next_ref;	\
	else								\
	  curr_task->first_channel_ref = curr_channel_ref->next_ref;	\
	curr_task->registered_channels--
	  
/* append an event to blocked-queue of a task */
static void
link_event_into_blocked_queue(
    task_item_t *curr_task, 
    event_ref_t *curr_event_ref)
{
  LOGd(DEBUGLVL(3), 
	"link event [%d,%d] into blocked-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);

  if (curr_task->last_blocked_event_ref != NULL)
  {
    curr_task->last_blocked_event_ref->next_ref = curr_event_ref;
    curr_task->last_blocked_event_ref = curr_event_ref;
  }
  else
  {
    curr_task->first_blocked_event_ref = curr_event_ref;
    curr_task->last_blocked_event_ref = curr_event_ref;
  }

  curr_event_ref->event_item->blocked_tasks++;
  curr_task->blocked_events++;
}


/* append an event to pending-queue of a task */
static void
link_event_into_pending_queue(
    task_item_t *curr_task,
    event_ref_t *curr_event_ref)
{
  LOGd(DEBUGLVL(3), 
	"link event [%d,%d] into pending-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);
  
  if (curr_task->last_pending_event_ref != NULL)
  {
    curr_task->last_pending_event_ref->next_ref = curr_event_ref;
    curr_task->last_pending_event_ref = curr_event_ref;
  }
  else
  {
    curr_task->first_pending_event_ref = curr_event_ref;
    curr_task->last_pending_event_ref = curr_event_ref;
  }
  
  curr_event_ref->event_item->pending_tasks++;
  curr_task->pending_events++;
}

/* append an event to ack-queue of a task */
static void
link_event_into_ack_queue(
    task_item_t *curr_task,
    event_ref_t *curr_event_ref)
{   
  LOGd(DEBUGLVL(3), 
	"link event [%d,%d] into ack-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);
  
  if (curr_task->last_ack_event_ref != NULL)
  {
    curr_task->last_ack_event_ref->next_ref = curr_event_ref;
    curr_task->last_ack_event_ref = curr_event_ref;
  }
  else
  {
    curr_task->first_ack_event_ref = curr_event_ref;
    curr_task->last_ack_event_ref = curr_event_ref;
  }

  curr_event_ref->event_item->ack_tasks++;
  curr_task->ack_events++;
}

/* unlink an event from the blocked-queue of a task */
static void 
unlink_event_from_blocked_queue(
    task_item_t *curr_task,
    event_ref_t *curr_event_ref,
    event_ref_t *prev_event_ref)
{
  LOGd(DEBUGLVL(3), 
	"unlink event [%d,%d] from blocked-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);
  
  if (curr_task->last_blocked_event_ref == curr_event_ref)
  {
    curr_task->last_blocked_event_ref = prev_event_ref;
  }

  if (prev_event_ref != NULL)
  {
    prev_event_ref->next_ref = curr_event_ref->next_ref;
  }
  else
  {
    curr_task->first_blocked_event_ref = curr_event_ref->next_ref;
  }
    
  curr_event_ref->next_ref = NULL;
  
  curr_event_ref->event_item->blocked_tasks--;
  curr_task->blocked_events--;
}

/* unlink an event from the pending-queue of a task */
static void 
unlink_event_from_pending_queue(
    task_item_t *curr_task,
    event_ref_t *curr_event_ref,
    event_ref_t *prev_event_ref)
{
  LOGd(DEBUGLVL(3), 
	"unlink event [%d,%d] from pending-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);
  
  if (curr_task->last_pending_event_ref == curr_event_ref)
  {
    curr_task->last_pending_event_ref = prev_event_ref;
  }

  if (prev_event_ref != NULL)
  {
    prev_event_ref->next_ref = curr_event_ref->next_ref;
  }
  else
  {
    curr_task->first_pending_event_ref = curr_event_ref->next_ref;
  }

  curr_event_ref->next_ref = NULL;

  curr_event_ref->event_item->pending_tasks--;
  curr_task->pending_events--;
}

/* unlink an event from the ack-queue of a task */
static void 
unlink_event_from_ack_queue(
    task_item_t *curr_task,
    event_ref_t *curr_event_ref,
    event_ref_t *prev_event_ref)
{
  LOGd(DEBUGLVL(3), 
	"unlink event [%d,%d] from ack-queue of task: %x",
	curr_event_ref->event_item->event_ch, 
	curr_event_ref->event_item->event_nr,
	curr_task->taskid);
  
  if (curr_task->last_ack_event_ref == curr_event_ref)
  {
    curr_task->last_ack_event_ref = prev_event_ref;
  }

  if (prev_event_ref != NULL)
  {
    prev_event_ref->next_ref = curr_event_ref->next_ref;
  }
  else
  {
    curr_task->first_ack_event_ref = curr_event_ref->next_ref;
  }

  curr_event_ref->next_ref = NULL;

  curr_event_ref->event_item->ack_tasks--;
  curr_task->ack_events--;
}

/* searching for a task with taskid in the task-list */
#define find_task(task, curr_task, prev_task)				\
									\
  curr_task = task_list;						\
  prev_task = NULL;							\
									\
  while (curr_task != NULL)						\
    if (curr_task->taskid == task)					\
    {									\
      break;								\
    }									\
    else								\
    {									\
      prev_task = curr_task;						\
      curr_task = curr_task->next_item;					\
    }									\
  if (curr_task != NULL)						\
  {  LOGd(DEBUGLVL(3), "task %d found", task); }				\
  else									\
    LOGd(DEBUGLVL(3), "task %d NOT found", task)

/* searching for an registered channel in a task */
#define find_channel_for_task(curr_task, channel, 			\
    			      curr_channel_ref, prev_channel_ref)	\
  curr_channel_ref = curr_task->first_channel_ref;			\
  prev_channel_ref = NULL;						\
  while (curr_channel_ref != NULL)					\
    if (curr_channel_ref->channel_item->event_ch == channel)		\
    {									\
      break;								\
    }									\
    else								\
    {									\
      prev_channel_ref = curr_channel_ref;				\
      curr_channel_ref = curr_channel_ref->next_ref;			\
    }									\
  if (curr_channel_ref != NULL)						\
  {  LOGd(DEBUGLVL(3), "channel:%d found for task:%d",			\
	channel, curr_task->taskid); }					\
  else									\
    LOGd(DEBUGLVL(3), "channel:%d NOT found for task:%d",		\
	channel, curr_task->taskid)	


/* searching for an event with a specified event-nr in ack-queue */
#define find_event_for_event_nr(levent_nr, curr_task,			\
				curr_event_ref, prev_event_ref)		\
  curr_event_ref = curr_task->first_ack_event_ref;			\
  prev_event_ref = NULL;						\
  while (curr_event_ref != NULL)					\
    if (curr_event_ref->event_item->event_nr == levent_nr)		\
    {									\
      break;								\
    }									\
    else								\
    {									\
      prev_event_ref = curr_event_ref;					\
      curr_event_ref = curr_event_ref->next_ref;			\
    }									\
  if (curr_event_ref != NULL)						\
  { LOGd(DEBUGLVL(3), "event-nr:%d found in ack-queue of task:%d",	\
	levent_nr, curr_task->taskid); }					\
  else									\
    LOGd(DEBUGLVL(3), "event-nr:%d NOT found in ack-queue of task:%d",	\
	levent_nr, curr_task->taskid)
  
#endif
