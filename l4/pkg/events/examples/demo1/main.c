/*!
 * \file   events/examples/demo1/demo1.c
 *
 * \brief  A simple single-tasked demo for the event-server
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/getopt.h>
#include <l4/util/util.h>

char LOG_tag[9]="ev_demo1";

/*! \brief report errors from events server in a nice way */
static void eval_request(int value)
{
  value = -value;
  if (value == L4EVENTS_OK)
    printf("Ok.\n");

  if (value == L4EVENTS_WARNING_TASK_REGISTERED)
    printf("Task is already registered for eventtyp.\n");

  if (value == L4EVENTS_WARNING_TASK_NOT_REGISTERED)
    printf("Task is not registered for eventtyp.\n");

  if (value == L4EVENTS_WARNING_EVENTS_DELETED)
    printf("Some events are lost because of unregistering.\n");

  if (value == L4EVENTS_WARNING_EVENTTYP_NOT_REGISTERED)
    printf("Eventtyp is not registered.\n");

  if (value == L4EVENTS_WARNING_NO_EVENT_TO_RECEIVE)
    printf("There was no event to receive.\n");

  if (value == L4EVENTS_WARNING_EVENT_PENDING)
    printf("This event is pending for some receiving tasks.\n");

  if (value == L4EVENTS_ERROR_IPC)
    printf("There was an IPC error!\n");

  if (value == L4EVENTS_ERROR_INTERNAL)
    printf("There was an internal event-server error!\n");

  if (value == L4EVENTS_ERROR_TIMEOUT)
    printf("There was an IPC Timeout!\n");

  if (value == L4EVENTS_ERROR_INVALID_COMMAND)
    printf("There was an invalid command!\n");

  printf("\n");
}


int main(int argc, char* argv[])
{
  int res;
  l4events_ch_t event_ch;
  l4events_nr_t event_nr;
  l4events_event_t send_evt, recv_evt;

  /* try to register some event_chs*/

  printf("registering eventtyp 10.\n");
  eval_request(l4events_register(10, 0));

  printf("registering eventtyp 11.\n");
  eval_request(l4events_register(11, 2));

  printf("registering eventtyp 11 again.\n");
  eval_request(l4events_register(11, 3));

  printf("registering eventtyp 12.\n");
  eval_request(l4events_register(12, 4));

  printf("registering eventtyp 13.\n");
  eval_request(l4events_register(13, 5));

  printf("registering eventtyp 14.\n");
  eval_request(l4events_register(14, 6));

  /* now try some unregister */

  printf("unregistering eventtyp 13.\n");
  eval_request(l4events_unregister(13));

  printf("unregistering event 15.\n");
  eval_request(l4events_unregister(15));

  l4events_dump();

  /* try some send and receive */

  strcpy(send_evt.str,"ABCDEFGHIJKLMNOPQRSTVW");
  send_evt.len = strlen(send_evt.str)+1;

  printf("sending an event.\n");
  eval_request(l4events_send(10, &send_evt, &event_nr, 
		L4EVENTS_ASYNC | L4EVENTS_SEND_ACK));
  printf("event_nr: %d\n", event_nr);
 
  l4events_dump();

  event_ch = L4EVENTS_NO_CHANNEL;

  printf("receiving an event.\n");
  res = l4events_receive(&event_ch, &recv_evt, &event_nr, L4_IPC_NEVER, 
      							L4EVENTS_RECV_ACK);
  

  if (res == L4EVENTS_OK)
  {
    printf("event_ch: %d", event_ch);
    printf("\n");
    printf("event: ");
    printf(recv_evt.str);
    printf("\n");
    printf("\n");
  }
  else
  {
    eval_request(res);
  }

  l4events_dump();
  printf("give ack");
  res = l4events_give_ack(event_nr);
  l4events_dump();

  /* try some unregister with pending events */

  strcpy(send_evt.str,"hello");
  send_evt.len = strlen(send_evt.str)+1;

  printf("sending an event.\n");
  eval_request(l4events_send(12, &send_evt, &event_nr, 
		L4EVENTS_ASYNC | L4EVENTS_SEND_ACK));
  printf("event_nr: %d\n", event_nr);

  strcpy(send_evt.str,"world");
  send_evt.len = strlen(send_evt.str)+1;

  l4_sleep(50);

  printf("sending an event.\n");
  eval_request(l4events_send(12, &send_evt, &event_nr, L4EVENTS_ASYNC));
  printf("event_nr: %d\n", event_nr);

  l4events_dump();
  
  printf("unregister eventtyp 12 with loosing pending events.\n");
  eval_request(l4events_unregister(12));

  l4events_dump();

  /* send and receive a short event */

  strcpy(send_evt.str,"ab");
  send_evt.len = strlen(send_evt.str)+1;

  printf("sending short event.\n");
  eval_request(l4events_send(14, &send_evt, &event_nr, 
		L4EVENTS_ASYNC | L4EVENTS_SEND_ACK));

  event_ch = 14;
  
  printf("receiving a short event.\n");
  res = l4events_receive(&event_ch, &recv_evt, &event_nr, 
      			L4_IPC_NEVER, L4EVENTS_RECV_SHORT);

  if (res == L4EVENTS_OK)
  {
    printf("event_ch: %d", event_ch);
    printf("\n");
    printf("event: ");
    printf(recv_evt.str);
    printf("\n");
  }
  else
  {
    eval_request(res);
  }

  /* cleanup the event server */
  printf("try to get notification for all sent events.\n");
  printf("event_nr:1\n");
  event_nr = 1;
  res = l4events_get_ack(&event_nr, L4_IPC_NEVER);
  printf("event_nr:2\n");
  event_nr = 2;
  res = l4events_get_ack(&event_nr, L4_IPC_NEVER);
  printf("event_nr:4\n");
  event_nr = 3;
  res = l4events_get_ack(&event_nr, L4_IPC_NEVER);
  printf("event_nr:5\n");
  event_nr = 4;
  res = l4events_get_ack(&event_nr, L4_IPC_NEVER);
  printf("res: %i\n", res);

  printf("try to unregister all previous registered eventtyps.\n");
  eval_request(l4events_unregister_all());

  /* show the clean memory */
  l4events_dump();

  printf("stop.\n");

  while (1) ;

  return 0;
};
