/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/src/events.c
 * \brief  Socket server 'events' support.
 *
 * \date   16/01/2006
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/thread/thread.h>
#include <l4/events/events.h>
#include <l4/sys/ipc.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>

/* *** LOCAL INCLUDES *** */
#include "events.h"
#include "server.h"

static void events_thread(void *_arg) {
    
    l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
    l4events_nr_t event_nr = L4EVENTS_NO_NR;
    l4events_event_t event;

    l4thread_started(NULL);
    
    while (1) {
	l4_threadid_t tid;

	/* wait for event */
	int ret = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
                                                L4_IPC_NEVER, L4EVENTS_RECV_ACK);
        if (ret < 0) {
	    LOG_Error("l4events_give_ack_and_receive() failed: %d", ret);
	    continue;
	}
        
	tid = *((l4_threadid_t *) event.str);
        shutdown_session_of_client(tid);
    }
}


int init_events(void) {

    int ret;
    l4thread_t th;

    if ( !l4events_init()) {
	LOG_Error("l4events_init() failed");
	return -1;
    }
    
    ret = l4events_register(L4EVENTS_EXIT_CHANNEL, 15);
    if (ret != L4EVENTS_OK) {        
	LOG_Error("l4events_register() failed: %d", ret);
	return -1;
    }
    
    th = l4thread_create_named(events_thread, ".events", NULL, L4THREAD_CREATE_SYNC);
    if (th < 0) {
	LOG_Error("l4thread_create() failed: %d", ret);
	return -1;
    }
    
    return 0;
}
