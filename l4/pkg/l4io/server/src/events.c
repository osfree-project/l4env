/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/events.c
 * \brief  L4Env l4io I/O Server Events Support
 *
 * \date   10/2005
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include <l4/events/events.h>
#include <l4/generic_io/generic_io-client.h>
#include <l4/thread/thread.h>
#include <l4/sys/syscalls.h> // l4_myself

#include "events.h"
#include "omega0lib.h"

static l4_threadid_t l4io_service_id = L4_INVALID_ID;
       l4_threadid_t events_thread_id = L4_INVALID_ID;

static void
events_init_and_wait(void *dummy)
{
  l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;
  CORBA_Environment _env = dice_default_environment;

  /* init event lib and register for event */
  l4events_init();
  l4events_register(event_ch, 14);

  /* event loop */
  for (;;)
    {
      l4_taskid_t tid;
      int ret, res;

      /* wait for event */
      res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
					L4_IPC_NEVER, L4EVENTS_RECV_ACK);
      if (res != L4EVENTS_OK)
	continue;

      tid = *(l4_taskid_t*)event.str;

      /* call service thread to free resources, this must be done to
       * synchronize the manipulation of loader data structures */
      ret = l4_io_release_client_call(&l4io_service_id, &tid, &_env);
      if (DICE_HAS_EXCEPTION(&_env))
        LOG_Error("handle exit event: call to service thread failed " \
                  "(exc %d)!", DICE_EXCEPTION_MAJOR(&_env));
      ret = l4_io_unregister_client_call(&l4io_service_id, &tid, &_env);
      if (DICE_HAS_EXCEPTION(&_env))
        LOG_Error("handle exit event: call to service thread failed " \
                  "(exc %d)!", DICE_EXCEPTION_MAJOR(&_env));

#ifndef ARCH_arm
      /* call the OMEGA0lib service thread to detach all interrupts of
       * that client. */
      OMEGA0_free_resources(tid);
#endif
    }
}

void
init_events(void)
{
  static int events_init_done;

  if (!events_init_done)
    {
      l4io_service_id = l4_myself();
      l4thread_t t = l4thread_create_long(L4THREAD_INVALID_ID,
					  events_init_and_wait, 
					  ".events", L4THREAD_INVALID_SP,
					  L4THREAD_DEFAULT_SIZE,
					  L4THREAD_DEFAULT_PRIO,
					  0, L4THREAD_CREATE_ASYNC);

      events_thread_id = l4thread_l4_id(t);
      events_init_done = 1;
    }
}
