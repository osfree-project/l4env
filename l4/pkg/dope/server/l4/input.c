/*
 * \brief   DOpE input module
 * \date    2002-11-13
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
#include <l4/thread/thread.h>
#include <l4/generic_io/libio.h>
#include <l4/input/libinput.h>

/*** LOCAL INCLUDES ***/
#include "dopestd.h"
#include "event.h"
#include "input.h"

/* XXX in startup.c (we need a property mgr?) */
extern l4io_info_t *l4io_page;

#define MAX_INPUT_EVENTS 64
static struct l4input ev[MAX_INPUT_EVENTS];
static long num_ev, curr_ev;

int init_input(struct dope_services *d);


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** GET NEXT EVENT OF EVENT QUEUE ***
 *
 * \return  0 if there is no pending event
 *          1 if there an event was returned in out parameter e
 */
static int get_event(EVENT *e) {

	/* poll some new input events from libinput */
	if (curr_ev == num_ev) {
		curr_ev = 0;
		num_ev  = l4input_flush(ev, MAX_INPUT_EVENTS);
	}

	/* no event in queue -> return */
	if (curr_ev == num_ev) return 0;

	e->type = 0;
	switch (ev[curr_ev].type) {
	
	case EV_REL:
		e->rel_x = e->rel_y = 0;
		e->type  = EVENT_MOTION;

		/* check if motion is vertical or horizontal */
		if (ev[curr_ev].code)
			e->rel_y  = ev[curr_ev].value;
		else
			e->rel_x  = ev[curr_ev].value;
		break;

	case EV_KEY:
		if (ev[curr_ev].value) {

			/* key or button pressed */
			INFO(printf("Mouse(update): pressed\n");)
			e->type = EVENT_PRESS;
			e->code = ev[curr_ev].code;
		} else {

			/* key or button button released */
			INFO(printf("Mouse(update): released\n");)
			e->type = EVENT_RELEASE;
			e->code = ev[curr_ev].code;
		}
		break;
	}
	curr_ev++;
	return 1;
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
	int init_ret;

	/* XXX this works only if OMEGA0 is not running stand-alone */
	init_ret=l4input_init(l4io_page ? l4io_page->omega0 : 0,
	                      L4THREAD_DEFAULT_PRIO, (void *)0);
	INFO(printf("Input(init): l4input_init() returned %d\n", init_ret);)

	d->register_module("Input 1.0", &input);
	return 1;
}
