#include <stdio.h>

#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>

/* event_ch */
l4events_ch_t recv_event_ch;
l4events_nr_t recv_event_nr;

/* callback function */
l4events_recv_function_t recv_function;

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
  l4_threadid_t my;
  l4_threadid_t pager;
  l4_threadid_t preempter;
  l4_umword_t   ignore;
  l4_threadid_t threadid;
  static char recv_thread_stack[1024];

  recv_event_ch = event_ch;
  recv_function = function;

  my = l4_myself();

  /* find the pager */
  l4_thread_ex_regs( my, -1, -1,	     /* id, EIP, ESP */
		     (l4_threadid_t*)&ignore, /* preempter */
		     &pager,		     /* pager */
		     &ignore,		     /* flags */
		     &ignore,		     /* old ip */
		     &ignore		     /* old sp */
		     );

  preempter = L4_INVALID_ID;
  threadid.id.lthread = threadno;

  /* start the receive thread */
  l4_thread_ex_regs( threadid,				/* dest thread */
		     (l4_umword_t)recv_thread,	 	/* EIP */
		     (l4_umword_t)recv_thread_stack+1024,	/* ESP */
		     &preempter,		 	/* preempter */
		     &pager,			 	/* pager */
		     &ignore,			 	/* flags */
		     &ignore,			 	/* old ip */
		     &ignore			 	/* old sp */
		     );
}
