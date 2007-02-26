/*****************************************************************************/
/**
 * \file   names/server/src/events.c
 * \brief  Event handling, listen for events at event server
 *
 * \date   01/03/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>
#include <string.h>

/* L4/L4Env includes */
#include <l4/events/events.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>
#include <l4/util/thread.h>
#include <l4/util/util.h>

/* names includes */
#include "events.h"
#include "names.h"

#define DEBUG_EVENTS 0

/*****************************************************************************
 *** globals
 *****************************************************************************/

/* event handling initialized */
static int events_init_done = 0;

/* events server id */
extern l4_threadid_t l4events_server;

/* event thread stack */
static char
events_stack[L4_PAGESIZE] __attribute__((aligned(4)));

/*****************************************************************************/
/**
 * \brief Event thread
 */
/*****************************************************************************/
static void
events_init_and_wait(void)
{
  l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;

  /* we need some hack here, because we can't call the l4events_init library
     function to get the events server id, because events is not initialized
     yet!
     So we ask the rmgr.*/
  if (rmgr_get_task_id("events", &l4events_server))
    {
      printf("Event server not found!\n");
      l4_sleep_forever();
    }

  if (DEBUG_EVENTS)
    printf("Found event server at "l4util_idfmt"\n",
	l4util_idstr(l4events_server));

  l4events_register(event_ch, NAMES_EVENT_THREAD_PRIORITY);

  if (DEBUG_EVENTS)
    printf("event thread up.\n");

  /* event loop */
  for (;;)
    {
      l4_threadid_t tid;
      int ret, res;

      /* wait for event */
      res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
					L4_IPC_NEVER, L4EVENTS_RECV_ACK);
      if (res != L4EVENTS_OK)
	{
          if(DEBUG_EVENTS)
	    printf("Got bad event (result=%d)\n", res);
	  continue;
	}

      tid = *(l4_threadid_t*)event.str;

      if(DEBUG_EVENTS)
        printf("Got exit event for "l4util_idfmt"\n", l4util_idstr(tid));

      /* call service thread to free resources, this must be done to
       * synchronize the manipulation of dm_phys data structures */
      ret = names_unregister_task(tid);

      if (!ret)
        printf("handle exit event: call to service thread failed " \
                  "(ret %d)!\n", ret);
    }
}

/*****************************************************************************
 *** names internal API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init event handling
 *
 * overwriting events library function
 */
/*****************************************************************************/
int
l4events_init(void)
{
  int * sp = (int *)&events_stack[L4_PAGESIZE];

  if (!events_init_done)
    {
      l4_threadid_t events_tid =
	l4util_create_thread(NAMES_EVENT_THREAD_NO, events_init_and_wait, sp);

      if(DEBUG_EVENTS)
        printf("started event thread at "l4util_idfmt"\n",
            l4util_idstr(events_tid));

      server_names_register(&events_tid, "names.events", &events_tid, 0);

      events_init_done = 1;
    }

  return 1;
}
