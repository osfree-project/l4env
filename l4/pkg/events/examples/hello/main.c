/*!
 * \file   events/examples/hello/main.c
 *
 * \brief  A demo for freeing resources
 *
 * \date   09/30/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/names/libnames.h>

#include <stdio.h>

char LOG_tag[9] = "hello";

int
main(int argc, char **argv)
{
  l4_threadid_t th;
  l4_msgdope_t result;
  l4_umword_t d1, d2;
  l4events_event_t event;
  l4events_nr_t eventnr;
  int i;

  printf("Hello World\n");

  if (names_register("hello"))
    printf("register at names ok.\n");
  else
    printf("register at names failed.\n");

  if (names_query_name("hello", &th))
  {
    printf("query name ok.\n");
  }
  else
    printf("query name failed.\n");

  th = l4_myself();

  for (i=0; i<2; i++)
    {
      printf("hello: My thread-id is "l4util_idfmt"\n", l4util_idstr(th));

      /* wait .5 sec */
      l4_ipc_receive(L4_NIL_ID, 0, &d1, &d2,
		     L4_IPC_TIMEOUT(0,0,122,9,0,0), &result);

    }

  printf("RMGR memory dump before exit event\n"
         "==================================\n");
  rmgr_dump_mem();

  printf("Sending exit event\n");

  // terminate by sending an exit event
  event.len = sizeof(l4_threadid_t);
  *(l4_threadid_t*)event.str = l4_myself();
  l4events_send(1, &event, &eventnr, L4EVENTS_ACK);

  //l4_sleep(1000);

  l4events_get_ack(eventnr, L4_IPC_NEVER);
  
  printf("RMGR memory dump after exit event\n"
         "=================================\n");
  rmgr_dump_mem();
  
  if (names_query_name("hello", &th))
  {
    printf("query name ok.\n");
  }
  else
    printf("query name failed.\n");
  
  l4_sleep_forever();
}

