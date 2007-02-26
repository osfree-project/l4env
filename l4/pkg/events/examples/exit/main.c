/*!
 * \file   dm_phys/server/src/events.c
 * \brief  Show threads that exited
 *
 * \date   08/17/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdlib.h>
#include <dice/dice.h>
#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>

static void handle_exit_events(void){
    int res;
    l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
    l4events_nr_t event_nr = L4EVENTS_NO_NR;
    l4events_event_t event;

    while(1){
	l4_threadid_t tid;

	/* wait for event */
	if((res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
						L4_IPC_NEVER,
						L4EVENTS_RECV_ACK))<0){
	    l4env_perror("l4events_give_ack_and_receive()", -res);
	    continue;
	}
	tid = *(l4_threadid_t *)event.str;
	printf("Got exit event for "l4util_idfmt"\n", l4util_idstr(tid));

	/* handle closing of resources of thread tid */
	/* ... */
    }
}

int main(int argc, char**argv){
    int err;

    LOGl("hi");
    if(!l4events_init()){
	LOG_Error("l4events_init() failed");
	exit(1);
    }
    if((err=l4events_register(L4EVENTS_EXIT_CHANNEL,
			      15))!=0){
	l4env_perror("l4events_register(%d)", err,  err);
	l4env_perror("l4events_register(%d)", -err, -err);
	exit(1);
    }
    handle_exit_events();
    return 0;
}
