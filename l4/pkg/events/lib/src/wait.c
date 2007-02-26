#include <stdio.h>

#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>

static l4events_ch_t recv_event_ch;
static l4events_nr_t recv_event_nr;
static l4events_recv_function_t recv_function;

static void
recv_thread(void)
{
  for (;;)
    {
      l4events_ch_t id = recv_event_ch;
      l4events_event_t recv_event;
      long res = l4events_receive(&id, &recv_event, &recv_event_nr, 
	  			  L4_IPC_NEVER, 0);

      if (res == L4EVENTS_OK)
	recv_function(id, &recv_event);
    }
}

void
l4events_wait(int threadno, l4events_ch_t event_ch,
		l4events_recv_function_t function)
{
  static char recv_thread_stack[1024];

  recv_event_ch = event_ch;
  recv_function = function;

  l4util_create_thread(threadno, recv_thread,
                       recv_thread_stack+sizeof(recv_thread_stack));
}
