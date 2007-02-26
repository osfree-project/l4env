/*
 * \brief   DOpE nitpicker input module
 * \date    2004-09-03
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

/*** L4 INCLUDES ***/
#include <l4/nitpicker/nitevent-server.h>
#include <l4/thread/thread.h>
#include <l4/input/libinput.h>
#include <l4/nitpicker/event.h>

/*** LOCAL INCLUDES ***/
#include "dopestd.h"
#include "event.h"
#include "input.h"

#define MAX_INPUT_EVENTS 64

static EVENT ev_queue[MAX_INPUT_EVENTS];
static int first, last;


int init_input(struct dope_services *d);

CORBA_Object_base nitevent_thread;


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** GET NEXT EVENT OF EVENT QUEUE ***
 *
 * \return  0 if there is no pending event
 *          1 if there an event was returned in out parameter e
 */
static int get_event(EVENT *e) {

	/* return if there is no event in event queue */
	if (first == last) return 0;

	/* take last element from event queue */
	memcpy(e, &ev_queue[last], sizeof(EVENT));
	last = (last+1) % MAX_INPUT_EVENTS;

	return 1;
}


/*** FUNCTION THAT IS CALLED FOR EACH INPUT EVENT ***/
void nitevent_event_component(CORBA_Object _dice_corba_obj, unsigned long token,
                         int type, int keycode, int rx, int ry, int ax, int ay,
                         CORBA_Server_Environment *_dice_corba_env) {

	EVENT *e = &ev_queue[first];

	switch (type) {
		case NITEVENT_TYPE_PRESS:
			e->type = EVENT_PRESS;
			e->code = keycode;
			break;
		case NITEVENT_TYPE_RELEASE:
			e->type = EVENT_RELEASE;
			e->code = keycode;
			break;
		case NITEVENT_TYPE_MOTION:
			e->type  = EVENT_ABSMOTION;
			e->abs_x = ax;
			e->abs_y = ay;
			break;
	}
	first = (first+1) % MAX_INPUT_EVENTS;
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct input_services input = {
	get_event,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_input(struct dope_services *d) {
	l4thread_t new_thread = l4thread_create(nitevent_server_loop, NULL, L4THREAD_CREATE_ASYNC);
	nitevent_thread = l4thread_l4_id(new_thread);
	d->register_module("Input 1.0",&input);
	return 1;
}
