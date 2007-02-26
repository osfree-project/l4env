/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/events.c
 * \brief  Event handling, listen for events at event server
 *
 * \date   01/03/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>

/* L4/L4Env includes */
#include <l4/dm_generic/dm_generic-client.h>
#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/thread.h>
#include <l4/names/libnames.h>

/* DMphys includes */
#include "__debug.h"
#include "__dm_phys.h"
#include "__events.h"

/*****************************************************************************
 *** globals
 *****************************************************************************/

/* event handling initialized */
static int events_init_done = 0;

/* event thread id */
static l4_threadid_t events_tid;

/* event thread stack */
static char
events_stack[DMPHYS_EVENT_THRAD_STACK_SIZE] __attribute__((aligned(4)));

/*****************************************************************************/
/**
 * \brief Event thread
 */
/*****************************************************************************/
static void
__events_init_and_wait(void)
{
  l4events_ch_t	event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t	event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;

  /* init event lib and register for event */
  l4events_init();
  l4events_register(event_ch, DMPHYS_EVENT_THREAD_PRIORITY);

  LOGdL(DEBUG_EVENTS, "event thread up.");

  /* event loop */
  for (;;)
    {
      l4_threadid_t tid;
      int ret;
      long res;
      CORBA_Environment _env = dice_default_environment;

      /* wait for event */
      res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
      					L4_IPC_NEVER, L4EVENTS_RECV_ACK);
      if (res != L4EVENTS_OK)
	{
          LOGdL(DEBUG_EVENTS, "Got bad event (result=%ld, %s)",
	      res, l4env_errstr(res));
          continue;
	}

      tid = *(l4_threadid_t *)event.str;

      LOGdL(DEBUG_EVENTS, "Got exit event for "l4util_idfmt,
            l4util_idstr(tid));

      /* call service thread to free resources, this must be done to
       * synchronize the manipulation of dm_phys data structures */
      ret = if_l4dm_generic_close_all_call(&dmphys_service_id, &tid,
                                           L4DM_SAME_TASK, &_env);
      if (ret || DICE_HAS_EXCEPTION(&_env))
        LOG_Error("handle exit event: call to service thread failed " \
                  "(ret %d, exc %d)!", ret, DICE_EXCEPTION_MAJOR(&_env));
    }
}

/*****************************************************************************
 *** DMphys internal API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init event handling
 */
/*****************************************************************************/
void
init_events(void)
{
  int * sp = (int *)&events_stack[DMPHYS_EVENT_THRAD_STACK_SIZE];

  if (!events_init_done)
    {
      /* start event thread */
      events_tid = l4util_create_thread(DMPHYS_EVENT_THREAD_NO,
                                        __events_init_and_wait, sp);
      names_register_thread_weak("dm_phys.events", events_tid);
      LOGdL(DEBUG_EVENTS, "started event thread at "l4util_idfmt,
            l4util_idstr(events_tid));

      events_init_done = 1;
    }

  /* done */
  return;
}


