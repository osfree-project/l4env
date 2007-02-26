/*!
 * \file   events/server/src/server.c
 *
 * \brief  events server
 *
 * \date   19/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * This is the events server.
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#ifdef USE_OSKIT
#include "l4/oskit/support.h"
#endif
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/kernel.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/util/util.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/l4_macros.h>

#include "dump.h"
#include "task_list.h"
#include "channel_list.h"
#include "ack_list.h"
#include "timeout_queue.h"
#include "globals.h"
#include "types.h"
#include "server.h"
#include "server-lib.h"

/* set to nonzero to enable debugging the malloc function */
static int debug_malloc = 0;

int verbosity = 0;

#ifdef USE_OSKIT
#ifdef STATIC_MEMORY
static char static_memory[MALLOC_POOL_SIZE];
#else
/* the address and the total size of dynamic allocated memory */
static int malloc_pool_addr = MALLOC_POOL_ADDR;
static int malloc_pool_size = MALLOC_POOL_SIZE;
#endif
#endif

/* the  event-nr counter */
static l4events_nr_t event_nr = 1;

/* some collected information for debugging and testing */
static l4_int32_t alloc_size;
static l4_uint32_t alloc_time;
static l4_uint32_t max_alloc_size;

/* simple copy function */
static void
copy_evt_to_evt(const l4events_event_t *evt_src, l4events_event_t *evt_dst)
{

  LOGd(DEBUGLVL(4), "copy evt to evt");

  evt_dst->len = evt_src->len;
  memcpy(evt_dst->str, evt_src->str, evt_src->len);
}


/* wrapper function for debugging */
static void*
my_malloc(int size)
{
  void* addr = NULL;
  l4_uint32_t in=0, out=0;

  if (debug_malloc)
    in = l4_rdtsc();

  addr = malloc(size);

  if (debug_malloc)
    out = l4_rdtsc();

  if (debug_malloc)
    LOG("malloc size: %d", size);

  if (addr == NULL)
  {
    LOG("malloc size: %d", alloc_size);
    enter_kdebug("malloc error");
  }

  if (debug_malloc)
  {
    alloc_size = alloc_size + size;
    if (alloc_size > max_alloc_size)
      max_alloc_size = alloc_size;
    alloc_time = alloc_time + (out-in);
  }

  return addr;
}

/* wrapper function for debugging  */
static void
my_free(void* addr, int size)
{
  l4_uint32_t in=0, out=0;

  if (debug_malloc)
  {
    in = l4_rdtsc();
    LOG("free size: %d", size);
  }

  free(addr);

  if (debug_malloc)
    out = l4_rdtsc();

  if (debug_malloc)
  {
    alloc_size = alloc_size - size;
    alloc_time = alloc_time + (out-in);
  }
}

/****************************************************************************
 *
 * Set a new event into all blocked lists of the registered task.
 * Ok, this is not optimal for performance.
 * 
 ****************************************************************************/
static void 
insert_event_into_blocked_queues(channel_item_t *curr_channel,
    				 event_item_t *curr_event)
{
  task_ref_t *curr_task_ref;
  task_item_t *curr_task;
  event_ref_t *curr_event_ref;
  int prior;
    
  for (prior = L4EVENTS_MAX_PRIORITY; prior >= 0; prior--)
  {
    curr_task_ref = curr_channel->first_task_ref[prior];

    /* run through the registered task list */
    while (curr_task_ref != NULL)
    {
      curr_task = curr_task_ref->task_item;

      /* create event_ref for curr_event */ 
      curr_event_ref = (event_ref_t*) my_malloc( sizeof(event_ref_t));
      curr_event_ref->event_item = curr_event;
      curr_event_ref->next_ref = NULL;

      /* link event into blocked list */
      link_event_into_blocked_queue(curr_task, curr_event_ref);

      /* go to the next registered task in the list */
      curr_task_ref = curr_task_ref->next_ref;
    }
  }
}

/****************************************************************************
 *
 * Send the event to as much as possible lower priority classes, until some
 * task are pending or ack'ing for this event.
 * 
 ****************************************************************************/
static void 
send_event_to_lower_priority(event_item_t *curr_event)
{
  int wait_for_receive = false;
  int wait_for_ack = false;
  int send_error = false;
  int found = false;
  int prior = curr_event->priority; 
  channel_item_t *curr_channel = curr_event->channel;
   
  LOGd(DEBUGLVL(2), "send event [%i,%i] to lower priority task",
	  curr_event->event_ch, curr_event->event_nr);
  
  while (!wait_for_receive && !wait_for_ack && (prior >= 0))
  {
  /* try to notify all registered all registered tasks for the channel */
  task_ref_t *curr_task_ref = curr_channel->first_task_ref[prior];

  /* run through the registered task list of a priority */
  while (curr_task_ref != NULL)
  {
    task_item_t *curr_task = curr_task_ref->task_item;
    event_ref_t *prev_event_ref = NULL;
    event_ref_t *curr_event_ref = curr_task->first_blocked_event_ref;
    found = false;
    
    /* find the event in the blocked-queue of the task */
    while ((curr_event_ref != NULL) && !found)
    {
      if (curr_event_ref->event_item == curr_event)
      {
	found = true;

        /* unlink event from blocked list */
        unlink_event_from_blocked_queue(
	  curr_task, curr_event_ref, prev_event_ref);
      }
      else
      {
	prev_event_ref = curr_event_ref;
	curr_event_ref = curr_event_ref->next_ref;
      }
    }

    if (found)
    {
    /* check if curr_task is already waiting for an event,
     * and try to send if so */
    if (!l4_is_invalid_id(curr_task->waiting_threadid) &&
        (curr_task->waiting_length >= curr_event->event->len) &&
	((((curr_task->waiting_event_ch != L4EVENTS_NO_CHANNEL) &&
	    (curr_task->waiting_event_ch == curr_event->event_ch))) ||
	 (curr_task->waiting_event_ch == L4EVENTS_NO_CHANNEL)))
    {
      LOGd(DEBUGLVL(2), "try to notify task for event");
	
      /* try to notify the waiting thread */
      send_error = receive_event_reply(curr_task->waiting_threadid,
					curr_event->event_ch, 
					curr_event->event_nr, 
					*curr_event->event);

      curr_task->waiting_threadid = L4_INVALID_ID;
    }
    else
    {
      /* if the event cant be sent immediatly we have to link it
         in the pending-queue */
      send_error = true;
    }

    /* link the event in the wait queue for this task,
       if he has the event not received for some reason */
    if (send_error)
    {
      wait_for_receive = true;
      
      /* append the the new element at the end of the pending wait-queue */
      link_event_into_pending_queue(curr_task, curr_event_ref);
    }
    else
    {
      /* sending was ok but may be, the sending task wants to give ack */
      if (curr_task->waiting_ack)
      {
	wait_for_ack = true;
	
        /* append the the new element at the end of the ack wait-queue */
        link_event_into_ack_queue(curr_task, curr_event_ref);
      }
      else
      {
	/* sending was ok and the task wants not give acknowledge */
	my_free(curr_event_ref, sizeof(event_ref_t));
      }
    }
    }

    /* go to the next registered task in the list */
    curr_task_ref = curr_task_ref->next_ref;
  }
  
    prior--;
  }
 
  /* save the PRIORITY */
  curr_event->priority = prior;
    
  /* there may be a few more blocked tasks with lower priority
       than the current priority */
  if (curr_event->priority >= 0)
    link_event_to_timeout_queue(curr_event);
}

/**************************************************************************** 
 * 
 * Try to give acknowledge a waiting sender task.
 * 
 ****************************************************************************/
static void
send_ack_event(l4events_nr_t enr)
{
  int found = false;
  int send_error;
  ack_item_t *curr_ack = NULL;
  ack_item_t *prev_ack = NULL;

  LOGd(DEBUGLVL(1), "event-nr: %d", enr);

  curr_ack = ack_list;

  while ((curr_ack != NULL) && !found)
  {
    if (curr_ack->event_nr == enr)
    {
      if (l4_is_nil_id(curr_ack->threadid) ||
         (l4_is_invalid_id(curr_ack->threadid)))
      {
	/* set a marker, because sender is not waiting */
	LOGd(DEBUGLVL(2), "no waiting sender");
	curr_ack->threadid = L4_INVALID_ID;
	return;
      }

	/* try to notify waiting sender */
	LOGd(DEBUGLVL(2), "try to give ack waiting sender");

	send_error =
	  get_ack_reply(curr_ack->threadid,
	      		L4EVENTS_OK,
			curr_ack->event_ch,
			curr_ack->event_nr);

	if (send_error)
	{
	  LOGd(DEBUGLVL(3), "giving ack failed");
	  /* notification failed */
	  curr_ack->threadid = L4_INVALID_ID;
	}
	else
	{
	  LOGd(DEBUGLVL(3), "giving ack succeded");
	  /* unlink after notification */
	  unlink_item_from_ack_list(ack_list, curr_ack, prev_ack);
	  my_free(curr_ack, sizeof(ack_item_t));
	}
      found = true;
    }
    else
    {
      /* go to next element in list */
      prev_ack = curr_ack;
      curr_ack = curr_ack->next_item;
    }
  }

  if (!found)
    LOGd(DEBUGLVL(1), "event-nr %d doesnt exist!", enr);
}

/****************************************************************************
 *
 * While deleting the event check for the acknowledge to the sender.
 * 
 ****************************************************************************/
static void
delete_event(event_item_t *curr_event)
{
  if ((curr_event->pending_tasks == 0) && 
      (curr_event->ack_tasks == 0))
  {
    LOGd(DEBUGLVL(3), "delete event [%d,%d]",
	curr_event->event_ch, curr_event->event_nr);

    /* give ack to the sender */
    send_ack_event(curr_event->event_nr);

    if (curr_event->timeout >= 0)
      unlink_event_from_timeout_queue(curr_event);

    my_free(curr_event->event,
	sizeof(curr_event->event->len) + curr_event->event->len);

    my_free(curr_event, sizeof(event_item_t));
  }
}

/****************************************************************************
 *
 * We are running throgh the blocked-queue of the task, and unlinking all 
 * events with the associated channel-nr. For every unlinked event we have 
 * to check, if we can delete the event.
 *
 ****************************************************************************/
static void 
remove_events_from_blocked_queue(
    task_item_t *curr_task,
    l4events_ch_t event_ch)
{
      event_ref_t *curr_event_ref = curr_task->first_blocked_event_ref;
      event_ref_t *prev_event_ref = NULL;
      event_ref_t *temp_event_ref = NULL;
      event_item_t *curr_event = NULL;

      LOGd(DEBUGLVL(2), "task:%x channel:%d", curr_task->taskid, event_ch);
      
      /* go through the bloecked-queue of task */
      while (curr_event_ref != NULL)
      {
        curr_event = curr_event_ref->event_item;

	/* check the channel */
	if (curr_event->event_ch == event_ch)
	{
	  /* go to next element */
	  temp_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	  
	  /* unlink event from blocked-queue of task */
	  unlink_event_from_blocked_queue(
	    curr_task, temp_event_ref, prev_event_ref);
	  
	  my_free(temp_event_ref, sizeof(event_ref_t));
	}
	else
	{
	  /* go to next element */
	  prev_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	}
      }

      return;
}

/****************************************************************************
 *
 * We are running through the pending-queue of the task, and unlinking all 
 * events with the associated channel-nr. For every unlinked event we have 
 * to check, if the server is able, to send the event to the next lower 
 * priority class before we can delete the event.
 *
 ****************************************************************************/
static int
remove_events_from_pending_queue(
    task_item_t *curr_task,
    l4events_ch_t event_ch)
{
      event_ref_t *curr_event_ref = curr_task->first_pending_event_ref;
      event_ref_t *prev_event_ref = NULL;
      event_ref_t *temp_event_ref = NULL;
      event_item_t *curr_event = NULL;
      int events_unlinked = false;

      LOGd(DEBUGLVL(2), "task:%x channel:%d", curr_task->taskid, event_ch);

      /* go through the pending-queue of task */
      while (curr_event_ref != NULL)
      {
        curr_event = curr_event_ref->event_item;

	if (curr_event->event_ch == event_ch)
	{
	  events_unlinked = true;
	  
	  /* go to next element */
	  temp_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	  
	  /* unlink event from pending-queue of task */
	  unlink_event_from_pending_queue(
	    curr_task, temp_event_ref, prev_event_ref);
	  
	  my_free(temp_event_ref, sizeof(event_ref_t));

	  send_event_to_lower_priority(curr_event);
	  delete_event(curr_event);
	}
	else
	{
	  /* go to next element */
	  prev_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	}
      }

      return events_unlinked;
}

/****************************************************************************
 *
 * We are running through the ack-queue of the task, and unlinking all 
 * events with the associated channel-nr. For every unlinked event we have 
 * to check, if the server is able, to send the event to the next lower 
 * priority class before we can delete the event.
 *
 ****************************************************************************/
static int 
remove_events_from_ack_queue(
    task_item_t *curr_task,
    l4events_ch_t event_ch)
{
      event_ref_t *curr_event_ref = curr_task->first_ack_event_ref;
      event_ref_t *prev_event_ref = NULL;
      event_ref_t *temp_event_ref = NULL;
      event_item_t *curr_event = NULL;
      int events_unlinked = false;

      LOGd(DEBUGLVL(2), "task:%x channel:%d", curr_task->taskid, event_ch);
      
      /* go through the ack-queue of task */
      while (curr_event_ref != NULL)
      {
        curr_event = curr_event_ref->event_item;

	if (curr_event->event_ch == event_ch)
	{
	  events_unlinked = true;
	  
	  /* go to next element */
	  temp_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	  
	  /* unlink event from the ack-queue ot task */
	  unlink_event_from_ack_queue(
	    curr_task, temp_event_ref, prev_event_ref);
	  
	  my_free(temp_event_ref, sizeof(event_ref_t));

	  send_event_to_lower_priority(curr_event);
	  delete_event(curr_event);
	}
	else
	{
	  /* go to next element */
	  prev_event_ref = curr_event_ref;
	  curr_event_ref = curr_event_ref->next_ref;
	}
      }

      return events_unlinked;
}



/********************************************************************
 *
 * server_register
 *
 * 1. found registered task for channel: give some warning
 * 2. not found registered task: create task item
 * 3. not found channel: create channel item
 * 4. link task and channel in the corresponding lists together
 * 
 ********************************************************************/
l4_uint8_t
server_register(l4_threadid_t *client, 
    		l4events_ch_t event_ch, 
		l4events_pr_t prior)
{
  channel_item_t *curr_channel = NULL;
  channel_item_t *prev_channel = NULL;
  task_item_t *curr_task = NULL;
  task_item_t *prev_task = NULL;
  channel_ref_t *curr_channel_ref = NULL;
  channel_ref_t *prev_channel_ref = NULL;
  task_ref_t *curr_task_ref = NULL;
  task_ref_t *prev_task_ref = NULL;
  int priority = 0;

  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> channel:%d priority:%d",
	l4util_idstr(*client), event_ch, prior);
 
  if ((prior > L4EVENTS_MAX_PRIORITY) || (prior < 0))
    return L4EVENTS_ERROR_INVALID_COMMAND;

  /* find task and channel */
  find_channel(event_ch, curr_channel, prev_channel);
  find_task(client->id.task, curr_task, prev_task);

  /* if channel already exists, try also to find the task */
  if (curr_channel != NULL)
    {
      find_task_for_channel(curr_channel, client->id.task, priority,
			  curr_task_ref, prev_task_ref);
    }

  /* task already exists, try also to find the channel */
  if (curr_task != NULL)
    {
      find_channel_for_task(curr_task, event_ch, 
			  curr_channel_ref, prev_channel_ref);
    }

  /* make some consistency check of our data structures */
  Assert 
    (((curr_channel_ref == NULL) && (curr_task_ref == NULL)) ||
     ((curr_channel_ref != NULL) && (curr_task_ref != NULL)));
 
  /* don't register twice a task for an event */
  if ((curr_channel_ref != NULL) && (curr_task_ref != NULL))
  {
    LOGd(DEBUGLVL(2), "task already for channel registered");
    return L4EVENTS_WARNING_TASK_REGISTERED;
  }
  
  /* create an new channel and/or task-struct */
  if ((curr_channel_ref == NULL) && (curr_task_ref == NULL))
  {
    LOGd(DEBUGLVL(2), "task NOT yet registered for event");

    /* create a new channel */
    if (curr_channel == NULL)
    {
      int pr;

      LOGd(DEBUGLVL(2), "create a new channel: %d", event_ch);

      curr_channel = (channel_item_t*) my_malloc( sizeof(channel_item_t));
      curr_channel->event_ch = event_ch;
      curr_channel->registered_tasks = 0;
      for (pr=0; pr<=L4EVENTS_MAX_PRIORITY; pr++)
        curr_channel->first_task_ref[pr] = NULL;
     
      link_item_into_channel_list(channel_list, curr_channel);
    }

    /* create a new task */
    if (curr_task == NULL)
    {
      LOGd(DEBUGLVL(2), "create a new task: %x", client->id.task);

      curr_task = my_malloc( sizeof(task_item_t));
      curr_task->taskid = client->id.task;
      curr_task->registered_channels = 0;
      curr_task->blocked_events = 0;
      curr_task->pending_events = 0;
      curr_task->ack_events = 0;
      curr_task->waiting_threadid = L4_INVALID_ID;
      curr_task->first_channel_ref = NULL;
      curr_task->first_blocked_event_ref = NULL;
      curr_task->last_blocked_event_ref = NULL;
      curr_task->first_pending_event_ref = NULL;
      curr_task->last_pending_event_ref = NULL;
      curr_task->first_ack_event_ref = NULL;
      curr_task->last_ack_event_ref = NULL;
    
      link_task_into_task_list(task_list, curr_task);
    }

    LOGd(DEBUGLVL(2), "link task and channel together");

    /* create a new list element for channel */
    curr_channel_ref = (channel_ref_t*) my_malloc( sizeof(channel_ref_t));
    curr_channel_ref->channel_item = curr_channel;
    
    /* link channel into list of registered channels for the task */
    link_channel_to_task(curr_task, curr_channel_ref);

    /* create a new list element for the task */
    curr_task_ref = (task_ref_t*) my_malloc( sizeof(task_ref_t));
    curr_task_ref->task_item = curr_task;
    
    /* link task into list of registered tasks for the channel */
    link_task_to_channel(curr_channel, prior, curr_task_ref);
  }

  return L4EVENTS_OK;
};


/****************************************************************************
 *
 * server_unregister
 *
 * 1. not found registered task: give some warning
 * 2. not found channel: give some warning
 * 3. found registered task for channel: do cleanup
 * 3.1. delete blocked events
 * 3.2. delete pending events
 * 3.3. delete not ack'ed events
 * 3.4. remove channel from task
 * 3.4.1. if task has no other registered channels: delete task
 * 3.5. remove task from channel
 * 3.5.1. if channel has no other registered tasks: delete channel
 * 
 ****************************************************************************/
l4_uint8_t
server_unregister(l4_threadid_t *client, l4events_ch_t event_ch)
{
  channel_item_t *curr_channel = NULL;
  channel_item_t *prev_channel = NULL;
  task_item_t *curr_task = NULL;
  task_item_t *prev_task = NULL;
  channel_ref_t *curr_channel_ref = NULL;
  channel_ref_t *prev_channel_ref = NULL;
  task_ref_t *curr_task_ref = NULL;
  task_ref_t *prev_task_ref = NULL;
  int events_unlinked = false;
  int priority = 0;
  l4_uint8_t result = L4EVENTS_OK;

  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> channel:%d",
  	l4util_idstr(*client), event_ch);

  /* find task and channel */
  find_channel(event_ch, curr_channel, prev_channel);
  find_task(client->id.task, curr_task, prev_task);

  /* if channel already exists, try also to find the task */
  if (curr_channel != NULL)
    {
      find_task_for_channel(curr_channel, client->id.task, priority, 
			  curr_task_ref, prev_task_ref);
    }

  /* if task aleady exists, try also to find the channel */
  if (curr_task != NULL)
    {
      find_channel_for_task(curr_task, event_ch, 
			  curr_channel_ref, prev_channel_ref);
    }

  /* make some consistency check of our data structures */
  Assert 
    (((curr_channel_ref == NULL) && (curr_task_ref == NULL)) ||
     ((curr_channel_ref != NULL) && (curr_task_ref != NULL)));

  /* don't unregister if this task is not already registered */
  if ((curr_channel_ref == NULL) && (curr_task_ref == NULL))
  {
    LOGd(DEBUGLVL(2), "task NOT registered for channel %d", event_ch);
    return L4EVENTS_WARNING_TASK_NOT_REGISTERED;
  }

  /* the task is really registered for this channel */
  if ((curr_channel_ref != NULL) && (curr_task_ref != NULL))
  {

    /* delete some (hopfully not important) blocked events */
    if (curr_task->blocked_events > 0)
      remove_events_from_blocked_queue(curr_task, event_ch);
    
    /* delete some (hopfully not important) events the task has
       not yet received */
    if (curr_task->pending_events > 0)
      events_unlinked = 
	remove_events_from_pending_queue(curr_task, event_ch);

    /* delete some (hopfully not important) events the task has
       not yet acknowledged */
    if (curr_task->ack_events > 0)
      events_unlinked =
	remove_events_from_ack_queue(curr_task, event_ch);

    LOGd(DEBUGLVL(3), "unlink channel from task");

    /* unlink the channel the task wants to unregister */
    unlink_channel_from_task(curr_task,
			     curr_channel_ref, prev_channel_ref);
    my_free(curr_channel_ref, sizeof(channel_ref_t));

    /* make some consistency checks on our data structures */
    Assert 
       (!((curr_task->first_channel_ref == NULL) &&
         (curr_task->pending_events > 0)));

    /* if the task is not registered for any other channel
       delete it from the task list */
    if (curr_task->registered_channels == 0)
    {
      unlink_task_from_task_list(task_list, curr_task, prev_task);
      my_free(curr_task, sizeof(task_item_t));
    }

    LOGd(DEBUGLVL(3), "unlink task from channel");

    /* unlink task from the list of registered tasks in the channel */
    unlink_task_from_channel(curr_channel, priority, 
			     curr_task_ref, prev_task_ref);
    my_free(curr_task_ref, sizeof(task_ref_t));

    /* if the channel has no registered tasks any more, delete it from the
       channel list */
    if (curr_channel->registered_tasks == 0)
    {
      unlink_item_from_channel_list(curr_channel, prev_channel);
      my_free(curr_channel, sizeof(channel_item_t));
    }
  }

  /* give a warning if some events get lost */
  if (events_unlinked)
    result = L4EVENTS_WARNING_EVENTS_DELETED;
  else
    result = L4EVENTS_OK;

  return result;
};


/********************************************************************
 *
 * server_unregister_all
 *
 * An iterative call of server_unregister until all channels are
 * for this task are unregistered.
 *
 ********************************************************************/
l4_uint8_t
server_unregister_all(l4_threadid_t *client)
{
  task_item_t *curr_task = NULL;
  task_item_t *prev_task = NULL;
  l4_uint8_t result = L4EVENTS_OK;

  LOGd(DEBUGLVL(1), l4util_idfmt, l4util_idstr(*client));
  
  find_task(client->id.task, curr_task, prev_task);

  if (curr_task == NULL)
    {
      LOGd(DEBUGLVL(2), "task NOT registered");
      return L4EVENTS_WARNING_TASK_NOT_REGISTERED;
    }

  /* loop trough the registered channel-list */
  while (curr_task->first_channel_ref != NULL)
    result = result | server_unregister(client,
    		curr_task->first_channel_ref->channel_item->event_ch);

  return result;
}


/********************************************************************
 *
 * server_send_event
 *
 * 1. no receiver for channel: give some warning
 * 2. some reveiver for channel: insert the event into blocked-queue
 * 				  of all receivers and send event to
 * 				  highest priority class
 * 2.1. not all tasks received event: give some warning
 * 2.2. all tasks received event: delete event
 * 				  
 ********************************************************************/
l4_uint8_t
server_send_event(l4_threadid_t *client,
		  l4events_ch_t event_ch,
		  const l4events_event_t *event,
		  int async, int ack)
{
  channel_item_t *curr_channel = NULL;
  channel_item_t *prev_channel = NULL;
  event_item_t *curr_event = NULL;
  l4_uint8_t result = L4EVENTS_OK;

  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> channel:%d",
  	l4util_idstr(*client), event_ch);

  find_channel(event_ch, curr_channel, prev_channel);

  if (curr_channel == NULL)
  {
    LOGd(DEBUGLVL(2), "NO task registered for channel");
    send_event_reply(L4EVENTS_WARNING_EVENTTYP_NOT_REGISTERED, 
			*client, L4EVENTS_NO_NR);
    return L4EVENTS_OK;
  }

  /* reply to the client immediatly, if it is wanted */
  if (async)
    send_event_reply(L4EVENTS_OK, *client, event_nr);

  LOGd(DEBUGLVL(3), "create event [%d,%d]", event_ch, event_nr);

  /* create an event */
  curr_event = (event_item_t*) my_malloc( sizeof(event_item_t));

  curr_event->event_ch = event_ch;
  curr_event->event_nr = event_nr;
  curr_event->blocked_tasks = 0;
  curr_event->pending_tasks = 0;
  curr_event->ack_tasks = 0;
  curr_event->channel = curr_channel;
  curr_event->priority = L4EVENTS_MAX_PRIORITY;
  curr_event->timeout = -1;
  curr_event->timeout_prev_event = NULL;
  curr_event->timeout_next_event = NULL;
  
  curr_event->event = my_malloc(sizeof(event->len) + event->len);
  copy_evt_to_evt(event, curr_event->event);

  /* XXX handling event-nr overflow XXX */
  event_nr++;
  if (event_nr >= MAX_EVENT_NR)
    {
      LOGd(DEBUGLVL(0), "WARNING: event-nr overflow");
      event_nr = 1;
    }

  insert_event_into_blocked_queues(curr_channel, curr_event);
  send_event_to_lower_priority(curr_event);

  /* reply to client now, if it is wanted */
  if (!async)
    send_event_reply(result, *client, curr_event->event_nr);

  /* mark event for acknowledge, if sending thread wants it */
  if (ack)
  {
    ack_item_t *curr_ack_item = NULL;

    LOGd(DEBUGLVL(1), "add ack-item for sender "l4util_idfmt" -> event-nr:%d",
      		l4util_idstr(*client), curr_event->event_nr);

    curr_ack_item = my_malloc(sizeof(ack_item_t));
   
    curr_ack_item->senderid = *client;
    /* if some tasks waiting for this event don't give notification 
       to the sending thread */
    if ((curr_event->pending_tasks > 0) || (curr_event->ack_tasks > 0))
      curr_ack_item->threadid = L4_NIL_ID;
    else
      curr_ack_item->threadid = L4_INVALID_ID;
    
    curr_ack_item->event_ch = curr_event->event_ch;
    curr_ack_item->event_nr = curr_event->event_nr;
   
    /* link event into the global ack-list */
    link_item_into_ack_list(ack_list, curr_ack_item);
  }

  /* don't delete event */
  if ((curr_event->pending_tasks > 0) || (curr_event->ack_tasks > 0))
  {
    result = L4EVENTS_WARNING_EVENT_PENDING;
  }
  /* delete event event because all registered tasks are noticed */
  else
  {
    LOGd(DEBUGLVL(3), "delete event [%d,%d]", 
	curr_event->event_ch, curr_event->event_nr);

    my_free(curr_event->event,
    	    sizeof(curr_event->event->len) + curr_event->event->len);

    my_free(curr_event, sizeof(event_item_t));
  }

  /* XXX for DICE implementation XXX */
  return result;
}


/********************************************************************
 *
 * server_receive_event
 *
 * 1. receiving thread not registered: give warning
 * 2. receiving thread registered: search for proper event in pending 
 * 				   queue of receiving task
 * 2.1. no proper event exists: mark receiving thread as waiting
 * 2.2. proper event exists: try send event to receviving thread
 * 2.2.1. receiving ok:	unlink event from pending-queue
 * 2.2.1.1. if receiving without ack: check for lower priority tasks
 * 2.2.1.2. if receiving with ack: link event to ack-list andwait 
 * 				   for ack from receiving thread
 * 2.2.2. receiving not ok: do nothing	
 * 
 ********************************************************************/
l4_uint8_t
server_receive_event(l4_threadid_t *client,
			l4events_ch_t *event_ch,
			l4events_event_t *event,
			int ack)
{
  task_item_t *curr_task = NULL;
  task_item_t *prev_task = NULL;
  event_item_t *curr_event = NULL;
  event_ref_t *curr_event_ref = NULL;
  event_ref_t *prev_event_ref = NULL;
  int found = false;
  int send_error = true;
  
  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> channel:%d with(out) ack:%d",
    l4util_idstr(*client), *event_ch, ack);

  find_task(client->id.task, curr_task, prev_task);

  if (curr_task == NULL)
  {
    LOGd(DEBUGLVL(2), "client "l4util_idfmt" NOT registered",
	l4util_idstr(*client));
    return L4EVENTS_WARNING_TASK_NOT_REGISTERED;
  }

  prev_event_ref = NULL;
  curr_event_ref = curr_task->first_pending_event_ref;

  /* find some event to reveive for this task */
  while ((curr_event_ref != NULL) && !found)
  {
    curr_event = curr_event_ref->event_item;

    if ((curr_event->event->len <= event->len) &&
        ((((*event_ch != L4EVENTS_NO_CHANNEL) && 
	   (curr_event->event_ch == *event_ch))) ||
	   (*event_ch == L4EVENTS_NO_CHANNEL)))
    {
      LOGd(DEBUGLVL(2), "receive event [%d,%d] for task %x",
	  curr_event->event_ch, curr_event->event_nr,
	  curr_task->taskid);

      found = true;
      /* XXX  for DICE implementation  XXX */
      *event_ch = curr_event->event_ch;
      copy_evt_to_evt(curr_event->event, event);

      /* try to send the event to the receiving task */
      send_error = receive_event_reply(*client, 
	  				curr_event->event_ch,
					curr_event->event_nr,
					*curr_event->event); 

      /* no thread is waiting now, of course */
      curr_task->waiting_threadid = L4_INVALID_ID;
    }
    else
      curr_event_ref = curr_event_ref->next_ref;
  }

  /* found no event for this task */
  if (!found)
  {
    LOGd(DEBUGLVL(2), 
	"NO event to receive for task %x, wait...",
	curr_task->taskid);

    /* of course, there is no event to send */
    send_error = true;

    /* leave the thread waiting */
    curr_task->waiting_threadid = *client;
    curr_task->waiting_length = event->len;
    curr_task->waiting_event_ch = *event_ch;
    curr_task->waiting_ack = ack;

    return L4EVENTS_WARNING_NO_EVENT_TO_RECEIVE;
  }

  /* if sending the pending event was successful then
     unlink the event from task's pending-queue */
  if (!send_error)
  {
    LOGd(DEBUGLVL(2),
	"sending event was successfull for task %x",
	curr_task->taskid);

    /* unlink the event from pending-queue of the task */
    unlink_event_from_pending_queue(
	curr_task, curr_event_ref, prev_event_ref);
   
    if (!ack)
    {
      my_free(curr_event_ref, sizeof(event_ref_t));

      /* try to send the event to a lower priority class */
      send_event_to_lower_priority(curr_event);
    
      /* try to delete the event */
      delete_event(curr_event);
    }
    else
      /* link event into ack-queue of the task */
      link_event_into_ack_queue(
	curr_task, curr_event_ref);
  }

  return L4EVENTS_OK;
}



/********************************************************************
 *
 * server_give_ack
 *
 * A receiving thread gives notification for an received event.
 * 1. task  not registered: give some warning
 * 2. task registered: search for event_nr
 * 2.1. event_nr not found: give some warning
 * 2.2. event_nr found: unlink event from ack-list
 * 2.2.a. check for sending event to lower priority tasks
 * 2.2.b. check for deleting event
 * 
 ********************************************************************/
l4_uint8_t
server_give_ack(l4_threadid_t *threadid, l4events_nr_t levent_nr)
{
  task_item_t *curr_task = NULL;
  task_item_t *prev_task = NULL;
  event_item_t *curr_event = NULL;
  event_ref_t *curr_event_ref = NULL;
  event_ref_t *prev_event_ref = NULL;
 
  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> event-nr:%d",
      l4util_idstr(*threadid), levent_nr);
 
  find_task(threadid->id.task, curr_task, prev_task);

  if (curr_task == NULL)
  {
    LOGd(DEBUGLVL(2), "client "l4util_idfmt" NOT registered",
	l4util_idstr(*threadid));
    
    return L4EVENTS_WARNING_TASK_NOT_REGISTERED;
  }

  /* search for proper event_nr in ack list */
  find_event_for_event_nr(levent_nr, curr_task,
			  curr_event_ref, prev_event_ref);
 
  /* event-nr not found */
  if (curr_event_ref == NULL)
    return L4EVENTS_WARNING_INVALID_EVENTNR;

  curr_event = curr_event_ref->event_item;

  /* unlink event from ack-queue of the task */
  unlink_event_from_ack_queue(
	curr_task, curr_event_ref, prev_event_ref);
  
  my_free(curr_event_ref, sizeof(event_ref_t));
    
  /* try to send the event to a lower priority class */
  send_event_to_lower_priority(curr_event);
  
  /* try to delete the event */
  delete_event(curr_event);

  return L4EVENTS_OK;
}


/*********************************************************************
 *
 * server_get_ack
 *
 * If the sending task wants ack for some event.
 * 1. found event for event_nr: 
 * 1.1. event is NOT ready received: let sender weiting
 * 1.2. event is ready received: try sender to give ack
 * 2. found not event for event_nr: give some warning back.
 *
 *********************************************************************/
l4_uint8_t
server_get_ack(l4_threadid_t *threadid, l4events_nr_t levent_nr)
{
  ack_item_t *curr_ack = ack_list;
  ack_item_t *prev_ack = NULL;
  int found = false;
  int send_error;
  
  LOGd(DEBUGLVL(1), "client "l4util_idfmt" -> eventnr:%d",
      	l4util_idstr(*threadid), levent_nr);

  while ((curr_ack != NULL) && !found)
  {
    if ((curr_ack->event_nr == levent_nr) ||
       ((levent_nr == 0) && 
        l4_thread_equal(curr_ack->senderid, *threadid)))
    {
      LOGd(DEBUGLVL(2),"event-nr:%d found for sender", 
	  curr_ack->event_nr);
      found = true;
      
      if (!l4_is_invalid_id(curr_ack->threadid))
	{
	  /* no notification because event is pending */
	  LOGd(DEBUGLVL(3), "wait to give ack sender for event");
	  curr_ack->threadid = *threadid;
	  if (levent_nr == 0) 
	    continue;
	  else
	    return L4EVENTS_OK;
	}
      
	/* try notification of because event is ready */
	LOGd(DEBUGLVL(3), "try to give ack sender for event");
	
	send_error =
	  get_ack_reply(*threadid,
	      		L4EVENTS_OK,
      			curr_ack->event_ch,
      			curr_ack->event_nr);

	if (send_error)
	{
	  /* notification failed */
	  curr_ack->threadid = L4_INVALID_ID;
	}
	else
	{
	  /* unlink after notification of the sender */
	  if (prev_ack != NULL)
	    prev_ack->next_item = curr_ack->next_item;
	  else
	    ack_list = curr_ack->next_item;

      	  my_free(curr_ack, sizeof(ack_item_t));
	}
	return L4EVENTS_OK;
    }
    else
    {
      /* go to next element in list */
      prev_ack = curr_ack;
      curr_ack = curr_ack->next_item;
    }
  }

  if (!found)
  {
    LOGd(DEBUGLVL(2), "event-nr:%d NOT found", levent_nr);
    
    send_error =
	  get_ack_reply(*threadid,
	      		L4EVENTS_WARNING_INVALID_EVENTNR,
	      		L4EVENTS_NO_CHANNEL, 
			L4EVENTS_NO_NR);
  }

  return L4EVENTS_OK;
}


/********************************************************************
 *
 * server_handle_timeout
 *
 * INTERNAL API function
 * 
 * We are running to the timeout list and searching for the first
 * elements with relative timeout 0. For every timeout we send the
 * event to the next priority class and if necessary set a new 
 * timeout.
 *
 ********************************************************************/
l4_uint8_t
server_handle_timeout(void)
{
  /* if timeout list is empty, we have nothing to do */
  if (timeout_first_event == NULL)
    return 0;
 
  /* decrement counter of first element  */
  timeout_first_event->timeout--;
 
  while ((timeout_first_event != NULL) && 
	 (timeout_first_event->timeout == 0))
  {
    event_item_t *curr_event = timeout_first_event;

    LOGd(DEBUGLVL(2), "unlink event [%i,%i]",
	  curr_event->event_ch, curr_event->event_nr);
      
    /* unlink from timeout queue, first */
    unlink_event_from_timeout_queue(curr_event);

    /* if neccessary event with new timeout in queue */
    send_event_to_lower_priority(curr_event);

    /* delete the event if no more acknowledging or pending 
       tasks for this event */
    delete_event(curr_event);
  }
  
  return L4EVENTS_OK;
}


/*********************************************************************
 *
 * server_dump
 *
 * This lists the contents of the various wait queues from
 * the registered tasks.
 * 
 *********************************************************************/
void server_dump(void)
{
  printf("----- start of event-server dump -----\n");

  dump_channel_list(channel_list);
  dump_task_list(task_list);
  dump_ack_list(ack_list);
  dump_timeout_queue(timeout_first_event);

  printf("----- end of event-server dump -------\n");

  if (debug_malloc)
  {
    printf("\n");
    printf("malloc size: %u\n", alloc_size);
    printf("malloc time: %u\n\n", alloc_time);
    printf("\n");
  }

  return;
}

/*********************************************************************
 *
 * command line parameter handling 
 *
 *********************************************************************/
static void parse_args(int argc, const char *argv[])
{
  int error;

  if ((error = parse_cmdline(&argc, &argv,
#ifdef USE_OSKIT
#ifndef STATIC_MEMORY
		'a', "malloc_pool_addr", "specify malloc pool base address",
		PARSE_CMD_INT, MALLOC_POOL_ADDR, &malloc_pool_addr,
		's', "malloc_pool_size", "specify malloc pool size",
		PARSE_CMD_INT, MALLOC_POOL_SIZE, &malloc_pool_size,
#endif
#endif
		'd', "debug_malloc", "measure time for malloc/free",
		PARSE_CMD_SWITCH, 1, debug_malloc,
		'v', "verbose", "specify verbosity",
		PARSE_CMD_INT, 0, &verbosity,
		0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()\n"); break;
	case -2: printf("Out of memory in parse_cmdline()\n"); break;
	default: printf("Error %d in parse_cmdline()\n", error); break;
	}
    }
}

/*********************************************************************
 *
 * main 
 *
 *********************************************************************/
char LOG_tag[9] = "events";

int main(int argc, const char *argv[])
{
  /* register to LOG-Server */
  //LOG_init("events");

  /* parse command line */
  parse_args(argc, argv);

#ifdef USE_OSKIT
#ifdef STATIC_MEMORY
  init_OSKit_malloc_from_memory((l4_umword_t)static_memory, 
				sizeof(static_memory));
#else
  /* init OSKit */
  init_OSKit_malloc(malloc_pool_addr, malloc_pool_size,
                    malloc_pool_size, L4_PAGESIZE);
#endif
#endif

  /* register to event-server */
  names_register(L4EVENTS_SERVER_NAME);

  LOGd(DEBUGLVL(1), "event server started.");
  LOGd(DEBUGLVL(1), "waiting for event...");

  server_loop(0);

  return 0;
};

