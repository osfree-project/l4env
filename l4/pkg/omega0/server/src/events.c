/**
 * \file   omega0/server/src/events.c
 * \brief  Event server support
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdlib.h>

#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/events/events.h>

#include "events.h"
#include "globals.h"
#include "omega0_proto.h"

int use_events;

static l4_threadid_t omega0_service_id;
       l4_threadid_t events_thread_id;

static void
events_init_and_wait(void)
{
  l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;

  l4events_init();
  l4events_register(event_ch, 10);

  for (;;)
    {
      int res;
      l4_msgdope_t result;
      l4_threadid_t tid;
      l4_umword_t dw0, dw1;

      res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
	                                  L4_IPC_NEVER, L4EVENTS_RECV_ACK);
      if (res != L4EVENTS_OK)
	continue;

      tid = *(l4_taskid_t*)event.str;

      res = l4_ipc_call(omega0_service_id,
                        L4_IPC_SHORT_MSG, OMEGA0_DETACH_ALL, tid.id.task,
	                L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER, &result);
    }
}

void
init_events(void)
{
  l4_threadid_t pager = L4_INVALID_ID, preempter = L4_INVALID_ID;
  void *stack;
  l4_umword_t dummy;

  omega0_service_id = events_thread_id = l4_myself();

  l4_thread_ex_regs_flags(l4_myself(), -1, -1, &preempter, &pager,
                          &dummy, &dummy, &dummy, L4_THREAD_EX_REGS_NO_CANCEL);
  if (l4_is_invalid_id(pager))
    return;

  if (!(stack = malloc(STACKSIZE)))
    {
      LOGl("error getting %d bytes of memory", STACKSIZE);
      return;
    }

  events_thread_id.id.lthread = 1;
  l4_thread_ex_regs(events_thread_id, 
                    (l4_umword_t)events_init_and_wait,
                    (l4_umword_t)stack + STACKSIZE,
		    &preempter, &pager, &dummy, &dummy, &dummy);

  names_register_thread_weak("omega0.events", events_thread_id);
}
