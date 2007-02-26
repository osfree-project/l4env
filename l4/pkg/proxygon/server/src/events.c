/*
 * \brief   Proxygon events support
 * \date    2004-10-04
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/l4con/l4con-client.h>
#include <l4/events/events.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/*** LOCAL INCLUDES ***/
#include "if.h"
#include "events.h"


/*** DROPS EVENTS HANDLER THREAD ***/
static void events_thread(void *arg) {
	CORBA_Environment env = dice_default_environment;
	l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
	l4events_nr_t event_nr = L4EVENTS_NO_NR;
	l4events_event_t event;
	l4_threadid_t tid;
	int res;

	/* init event lib and register for event */
	l4events_init();
	l4events_register(event_ch, 14);

	printf("Server(events_thread): events receiver thread is up.\n");

	/* event loop */
	while (1) {

		/* wait for event */
		res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
		                                    L4_IPC_NEVER, L4EVENTS_RECV_ACK);
		if (res != L4EVENTS_OK) continue;

		/* determine affected application id */
		tid = *(l4_threadid_t*)event.str;

		printf("events_thread: got exit event for "l4util_idfmt"\n", l4util_idstr(tid));

		con_if_close_all_call(&if_tid, &tid, &env);
	}
}


/*** START EVENTS SERVER THREAD ***/
int start_events_server(void) {

	/* create if server thread */
	if (l4thread_create_named(events_thread, "proxygon-events",
	                          NULL, L4THREAD_CREATE_ASYNC) <= 0) {
		printf("Error: could not create events thread.\n");
		return -1;
	}
	return 0;
}
