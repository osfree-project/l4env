/*!
 * \file   events/server/src/timeout_queue.h
 *
 * \brief  Functions for inserting and removing timeouts.
 *
 * \date   19/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "l4/log/l4log.h"

#include "globals.h"
#include "types.h"
#include "dump.h"

#ifndef __L4EVENTS_TIMEOUT_QUEUE
#define __L4EVENTS_TIMEOUT_QUEUE

/* this the first event in the timeout queue */
event_item_t*		timeout_first_event = NULL;
/* this is the last event in the timeout queue */
event_item_t*		timeout_last_event = NULL;


/**
 * unlink timeout from list, be careful by adjusting
 * the timeout
 */
static void 
unlink_event_from_timeout_queue(event_item_t *i)
{
  LOGd(DEBUGLVL(2), "unlink event (%i.%li) from timeout queue",
      i->event_ch, i->event_nr);

  /* don't make double unlink */
  Assert (i->timeout >= 0);

  /* if the current timeout > 0 we add it to the next event */
  if (i->timeout > 0)
    {
      LOGd(DEBUGLVL(4), "adjust timeout for next event in queue");

      if (i->timeout_next_event != NULL)
	{
	  i->timeout_next_event->timeout =
	    i->timeout_next_event->timeout + i->timeout;
	}
    }

  LOGd(DEBUGLVL(4), "unlink event from timeout-queue");

  /* unlink */
  if (i->timeout_prev_event != NULL)
    i->timeout_prev_event->timeout_next_event = i->timeout_next_event;
  else
    timeout_first_event = i->timeout_next_event;

  if (i->timeout_next_event != NULL)
    i->timeout_next_event->timeout_prev_event = i->timeout_prev_event;
  else
    timeout_last_event = i->timeout_prev_event;

  i->timeout = -1;
  i->timeout_prev_event = NULL;
  i->timeout_next_event = NULL;

  /* some insanity check */
  Assert 
    (((timeout_first_event==NULL) && (timeout_last_event==NULL)) ||
     ((timeout_first_event!=NULL) && (timeout_last_event!=NULL)));
}


/**
 * insert event in timeout_queue 
 * run through timout-list while subtracting the ticks
 * and find the apropriate place to insert or append
 * at the end of the list
 **/
static void 
link_event_to_timeout_queue(event_item_t *i)
{
  event_item_t *j;
  int insert = false;

  LOGd(DEBUGLVL(2), "link event (%i.%li) to timeout queue",
      i->event_ch, i->event_nr);

  /* before we link , do unlink */
  if (i->timeout >= 0)
    {
      LOGd(DEBUGLVL(2), "unlink first");
      unlink_event_from_timeout_queue(i);
    }

  /* set timeout value */
  i->timeout = TIMEOUT_TICKS;
  j = timeout_first_event;

  while (j && !insert)
    {
      if (i->timeout < j->timeout)
	{
	  LOGd(DEBUGLVL(4), "insert event into timeout list");
	  insert = true;

	  j->timeout = 
	    j->timeout - i->timeout;

	  if (j->timeout_prev_event == NULL)
	    {
	      timeout_first_event = i;

	      j->timeout_prev_event = i;
	      i->timeout_next_event = j;
	    }
	  else
	    {
	      j->timeout_prev_event->timeout_next_event = i;
	      i->timeout_prev_event = 
		j->timeout_prev_event;

	      j->timeout_prev_event = i;
	      i->timeout_next_event = j;
	    }
	}
      else
	{
	  i->timeout = i->timeout - j->timeout;
	  j = j->timeout_next_event;
	}
    }

  /* special case: appending timeout */
  if (!insert)
    {
      LOGd(DEBUGLVL(4), "append event into timeout list");

      if (timeout_first_event == NULL)
	{
	  timeout_first_event = i;
	  timeout_last_event = i;
	}
      else
	{
	  timeout_last_event->timeout_next_event = i;
	  i->timeout_prev_event = timeout_last_event;
	  timeout_last_event = i;
	}
    }
}

#endif
