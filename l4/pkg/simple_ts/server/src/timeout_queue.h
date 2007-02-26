#include "l4/log/l4log.h"
#include "l4/events/events.h"

#ifndef __SIMPLE_TS_TIMEOUT_QUEUE
#define __SIMPLE_TS_TIMEOUT_QUEUE

/* The timeout list contains entries with relative timeouts */

#define TIMEOUT_DFL 10

typedef struct timeout_item
{
  l4_int32_t		index;    /**< backward reference into __tasks array */
  l4_int32_t 		timeout;
  l4events_nr_t		eventnr;
  l4_threadid_t		client;
  l4_uint8_t		options;
  struct timeout_item	*prev;
  struct timeout_item 	*next;
} timeout_item_t;

timeout_item_t *timeout_first; /**< first event in timeout queue */


static void 
unlink_item_from_timeout_queue(timeout_item_t *i)
{
  if (i->timeout == -1)
    {
      enter_kdebug("DOUBLE UNLINK");
      return;
    }

  if (i->next && i->timeout > 0)
    i->next->timeout += i->timeout;

  i->timeout = -1;

  if (i->prev)
    i->prev->next = i->next;
  else
    timeout_first = i->next;

  if (i->next)
    i->next->prev = i->prev;

  i->prev = i->next = 0;
}


/** Insert event in timeout_queue. Walk through the timout list, subtract
 *  the ticks and find the appropriate place.
 */
static void 
link_item_to_timeout_queue(timeout_item_t *i)
{
  timeout_item_t *j;

  if (i->timeout >= 0)
    unlink_item_from_timeout_queue(i);

  /* set timeout value */
  i->timeout = TIMEOUT_DFL;

  for (j=timeout_first; j && j->next; j=j->next)
    {
      if (i->timeout < j->timeout)
	{
	  /* insert */
	  j->timeout -= i->timeout;
	  if (!j->prev)
	    timeout_first = i;
	  else
	    {
	      j->prev->next = i;
	      i->prev = j->prev;
	    }
	  j->prev = i;
	  i->next = j;
	  return;
	}

      i->timeout -= j->timeout;
    }

  /* append: first and only element in list */
  if (!timeout_first)
    {
      timeout_first = i;
      return;
    }

  /* append */
  j->next = i;
  i->prev = j;
}

#endif
