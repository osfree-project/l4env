/* (c) 2008 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdlib.h>
#include <dice/dice.h>
#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>

#include "local.h"

#include <tpm_commands.h>

void vtpmemu_event_loop(void *data) {
  int res;
  l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;
  l4_taskid_t *waitfor = &((*(struct slocal *)data).shutdown_on_exit);

  if (!l4events_init()) {
    LOG_Error("l4events_init() failed");
    exit(1);
  }

  if ((res=l4events_register(L4EVENTS_EXIT_CHANNEL, 15))!=0) {
    l4env_perror("l4events_register(%d)", res,  res);
    exit(1);
  }

  l4thread_started(data);
  while(1) {
    l4_threadid_t tid;

    /* wait for event */
    if ((res = l4events_receive(&event_ch, &event, &event_nr, L4_IPC_NEVER, 0)) < 0)
    {
      l4env_perror("l4events_give_ack_and_receive()", -res);
      continue;
    }
    tid = *(l4_threadid_t *)event.str;

    // ignore exit events when we have no one we are waiting for
    if (!l4_is_invalid_id(*waitfor) && l4_task_equal(*waitfor, tid))
    {
      LOG("Got exit event of task "l4util_idfmt". Try to save TPM state ...\n", l4util_idstr(tid));
      tpm_emulator_shutdown();
      res = l4events_unregister(L4EVENTS_EXIT_CHANNEL);
      if (res != 0)
        l4env_perror("unregistration of events service failed", res);

      LOG("Shutdown myself");
      exit(0);
    }

  }
}
