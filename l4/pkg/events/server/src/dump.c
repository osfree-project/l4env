#include <stdio.h>
#include "dump.h"
#include "types.h"
#include "globals.h"

#include "l4/sys/kdebug.h"
#include "l4/log/l4log.h"
#include "l4/util/l4_macros.h"

#undef putchar

/* dump entries of channel list with registerd tasks
 * in every priority class */
void 
dump_channel_list(channel_item_t *curr_channel)
{
  task_ref_t *curr_task_ref;
  task_item_t *curr_task;
  int pr;

  putchar('\n');
 
  while (curr_channel)
    {
      printf("channel: %i tasks:%li\n", 
	     curr_channel->event_ch,
	     curr_channel->registered_tasks);

      for (pr = L4EVENTS_MAX_PRIORITY; pr>=0; pr--)
	{
	  if (curr_channel->first_task_ref[pr])
	    {
	      printf("  priority: %i\n", pr);
	      curr_task_ref = curr_channel->first_task_ref[pr];

	      while (curr_task_ref)
		{
		  curr_task = curr_task_ref->task_item;
		  printf("    task: %lx\n", curr_task->taskid);
		  curr_task_ref = curr_task_ref->next_ref;
		}
	    }
	}

      curr_channel = curr_channel->next_item;
    }
}

/* dump registed tasks with blocked-queue, pending-queue and
 * ack-queue */
void 
dump_task_list(task_item_t *curr_task)
{
  channel_ref_t *curr_channel_ref;
  event_ref_t *curr_event_ref;
  channel_item_t *curr_channel;
  event_item_t *curr_event;

  putchar('\n');

  while (curr_task)
    {
      printf("task: %lx channels: %li blocked: %li pending: %li ack: %li \n", 
	     curr_task->taskid, 
	     curr_task->registered_channels,
	     curr_task->blocked_events,
	     curr_task->pending_events, 
	     curr_task->ack_events);
      if (!l4_is_invalid_id(curr_task->waiting_threadid))
	{
	  printf("waiting ["l4util_idfmt"] for event_ch:%i length: %i", 
		 l4util_idstr(curr_task->waiting_threadid),
		 curr_task->waiting_event_ch,
		 curr_task->waiting_length);
	  puts(curr_task->waiting_ack ? " with ack" : "");
	}

      curr_channel_ref = curr_task->first_channel_ref;
      while (curr_channel_ref)
	{
	  curr_channel = curr_channel_ref->channel_item;
	  printf("  channel: %i\n", curr_channel->event_ch);
	  curr_channel_ref = curr_channel_ref->next_ref;
	}

      curr_event_ref = curr_task->first_blocked_event_ref;
      while (curr_event_ref)
	{
	  curr_event = curr_event_ref->event_item;
	  printf("  event: (%i,%li) blocked\n", 
	      curr_event->event_ch, curr_event->event_nr);
	  curr_event_ref = curr_event_ref->next_ref;
	}

      curr_event_ref = curr_task->first_pending_event_ref;
      while (curr_event_ref)
	{
	  curr_event = curr_event_ref->event_item;
	  printf("  event: (%i,%li) pending\n", 
	      curr_event->event_ch, curr_event->event_nr);
	  curr_event_ref = curr_event_ref->next_ref;
	}

      curr_event_ref = curr_task->first_ack_event_ref;
      while (curr_event_ref)
	{
	  curr_event = curr_event_ref->event_item;
	  printf("  event: (%i,%li) ack\n",
	      curr_event->event_ch, curr_event->event_nr);
	  curr_event_ref = curr_event_ref->next_ref;
	}

      curr_task = curr_task->next_item;
    }
}

/* dump entries of the ack-list */
void 
dump_ack_list(ack_item_t *curr_notify_item)
{
  putchar('\n');
  
  while (curr_notify_item)
    {
      printf("notify task "l4util_idfmt" for (%i,%li)\n",
	     l4util_idstr(curr_notify_item->threadid),
	     curr_notify_item->event_ch,
	     curr_notify_item->event_nr);

      curr_notify_item = curr_notify_item->next_item;
    }
}

/* dump entries of the timeout-queue */
void 
dump_timeout_queue(event_item_t *timeout_curr_event)
{
  putchar('\n');
  
  while (timeout_curr_event)
    {
      printf("timeout for event: (%i,%li) timeout:%i \n", 
	      timeout_curr_event->event_ch,
	      timeout_curr_event->event_nr,
	      timeout_curr_event->timeout);

      timeout_curr_event = timeout_curr_event->timeout_next_event;
    }
}
