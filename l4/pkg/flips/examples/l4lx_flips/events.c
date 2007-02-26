/**
 * \file   flips/examples/l4lx_flips/events.c
 * \brief  Event handling, listen for events at event server
 *
 * \date   2006-05-30
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author Christian helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/events/events.h>
#include <flips-client.h>

/*** GENERAL INCLUDES ***/
#include <stdio.h>

#include "local.h"

static l4_threadid_t     main_l4thread;

static slist_t         **sessions;
static pthread_mutex_t  *slock;

static int debug_events = 0;

/******************
 ** Event thread **
 ******************/

static void events_thread(sem_t *started)
{
	l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
	l4events_nr_t event_nr = L4EVENTS_NO_NR;
	l4events_event_t event;

	/* init event lib and register for event */
	l4events_init();
	l4events_register(event_ch, 14);

	sem_post(started);

	/* event loop */
	for (;;) {
		l4_taskid_t tid;
		int /*ret,*/ res;

		/* wait for event */
		res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
		                                    L4_IPC_NEVER, L4EVENTS_RECV_ACK);
		if (res != L4EVENTS_OK) continue;

		tid = *(l4_taskid_t*)event.str;

		if (debug_events)
			printf("potential client <%x> exited...\n", tid.id.task);

		/* lookup potential client in list */
		slist_t *p = *sessions;
		pthread_mutex_lock(slock);
		while (p) {
			struct session_thread_info *info = (struct session_thread_info *)p->data;
			if (l4_task_equal(info->partner, tid)) {
				DICE_DECLARE_ENV(env);
				if (debug_events) printf("  closing session @ %p\n", info);

				/* request main thread to shutdown connection */
				pthread_mutex_unlock(slock);
				l4vfs_connection_close_connection_call(&main_l4thread,
				                                       &info->session_l4thread, &env);
				pthread_mutex_lock(slock);

				/* start over */
				p = *sessions;
			} else
				p = p->next;
		}
		pthread_mutex_unlock(slock);

		if (debug_events)
			printf("  %d sessions remaining\n", list_elements(*sessions));
	}
}

/*************************
 ** INIT EVENTS SUPPORT **
 *************************/

void init_events(slist_t **list, pthread_mutex_t *lock)
{
	static int init;
	pthread_t t;
	sem_t started;

	if (init) return;

	sem_init(&started, 0, 0);
	pthread_create(&t, 0, (void*(*)(void*))events_thread, (void *)&started);
	sem_wait(&started);

	main_l4thread = l4_myself();

	init = 1;
	sessions = list;
	slock    = lock;
}
