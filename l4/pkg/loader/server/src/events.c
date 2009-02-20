/*****************************************************************************/
/**
 * \file   loader/server/src/events.c
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

#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/log/l4log.h>
#include <l4/loader/loader-client.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/events/events.h>

#include "events.h"

#define DEBUG_EVENTS 0

static l4_threadid_t loader_service_id;

int events_send_kill(l4_threadid_t task)
{
  l4events_event_t event;
  l4events_nr_t eventnr;
  int ret;

  /* sent exit_event */
  event.len = sizeof(l4_threadid_t);
  *(l4_threadid_t*)event.str = task;

  if ((ret = l4events_send(L4EVENTS_EXIT_CHANNEL, &event, &eventnr,
         L4EVENTS_SEND_ACK)))
    return -L4_EUNKNOWN;

  return 0;
}

/* This is tricky: If the loader fails to load an application it asks the
 * task server to kill that task. The task server generates an EXIT event
 * which is sent to all registered resource managers. And therefore _we_
 * also receive such an exit event. After that, we call the main thread
 * to free related resources. And bingo -- that's the deadlock since the
 * main thread is still waiting for the result of the l4ts_task_kill()
 * function.
 * To prevent this deadlock the main thread first sets killing to the task
 * which is killed and we ignore that request here. */
l4_threadid_t killing = L4_INVALID_ID;

static void
events_init_and_wait(void *dummy)
{
  l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;
  CORBA_Environment _env = dice_default_environment;

  /* init event lib and register for event */
  l4events_init();
  l4events_register(event_ch, _LOADER_EVENT_THREAD_PRIORITY);

  LOGdL(DEBUG_EVENTS, "event thread up.");

  /* event loop */
  for (;;)
    {
      l4_taskid_t tid;
      int ret, res;

      /* wait for event */
      res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
					L4_IPC_NEVER, L4EVENTS_RECV_ACK);
      if (res != L4EVENTS_OK)
	{
          LOGdL(DEBUG_EVENTS, "Got bad event (result=%d, %s)",
	      res, l4env_errstr(res));
	  continue;
	}

      tid = *(l4_taskid_t*)event.str;

      /* ignore that event if _we_ gave the order */
      if (l4_task_equal(killing, tid))
	continue;

      LOGdL(DEBUG_EVENTS, "Got exit event for "l4util_idfmt,
            l4util_idstr(tid));

      /* call service thread to free resources, this must be done to
       * synchronize the manipulation of loader data structures */
      ret = l4loader_app_kill_call(&loader_service_id, &tid, 0, &_env);
      if (DICE_HAS_EXCEPTION(&_env))
        LOG_Error("handle exit event: call to service thread failed " \
                  "(exc %d)!", DICE_EXCEPTION_MAJOR(&_env));
    }
}

int
init_events(void)
{
  static int events_init_done;

  if (!events_init_done)
    {
      l4thread_t events_tid;

      loader_service_id = l4_myself();

      events_tid = l4thread_create_long(L4THREAD_INVALID_ID,
					events_init_and_wait, 
					".events", L4THREAD_INVALID_SP,
					L4THREAD_DEFAULT_SIZE,
					L4THREAD_DEFAULT_PRIO,
					0, L4THREAD_CREATE_ASYNC);
      LOGdL(DEBUG_EVENTS, "started event thread at "l4util_idfmt" events_tid=%d",
            l4util_idstr(l4thread_l4_id(events_tid)), events_tid);
      if (events_tid < 0)
        return 0;

      events_init_done = 1;
    }

  return 1;
}
