/*
 * \brief   Nitpicker input event handling
 * \date    2004-08-24
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
#include <l4/sys/syscalls.h>                /* for l4_myself()      */
#include <l4/input/libinput.h>              /* input device driver  */
#include <l4/input/macros.h>                /* key and button codes */
#include <l4/generic_io/libio.h>            /* l4io mode for input  */
#include <l4/thread/thread.h>               /* for default prio     */
#include <l4/nitpicker/event.h>             /* event definitions    */
#include <l4/util/rdtsc.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


extern l4io_info_t *l4io_page;              /* from startup.c              */
static unsigned long last_input;            /* last time of input handling */
static struct l4input ev[MAX_INPUT_EVENTS];


/***************************
 *** INPUT EVENT HANDLER ***
 ***************************/

/*** POLL INPUT LIB FOR NEW EVENTS AND HANDLE THEM ***/
void foreach_input_event(void (*handle)(int type, int code, int rx, int ry)) {
	static int num_ev, curr_ev;
	unsigned long now = l4_tsc_to_us(l4_rdtsc());

	/*
	 * Check if the time is right for handling the input
	 * or if the time stamp counter wrapped.
	 */
	if ((now < last_input + 20*1000) && (now > last_input)) return;
	last_input = now;

	while (1) {

		/* poll some new input events from libinput */
		if ((num_ev == curr_ev) && l4input_ispending()) {
			curr_ev = 0;
			num_ev  = l4input_flush(ev, MAX_INPUT_EVENTS);
		}

		/* no event */
		if (curr_ev >= num_ev) return;

		/* merge relative events of same type */
		if (ev[curr_ev].type == EV_REL) {
			int rx = 0, ry = 0, wx = 0, wy = 0;

			/* merge motion events */
			for (; (ev[curr_ev].type == EV_REL) && (curr_ev < num_ev); curr_ev++) {
				rx += (ev[curr_ev].code == REL_X)      ? ev[curr_ev].value : 0;
				ry += (ev[curr_ev].code == REL_Y)      ? ev[curr_ev].value : 0;
				wx += (ev[curr_ev].code == REL_HWHEEL) ? ev[curr_ev].value : 0;
				wy += (ev[curr_ev].code == REL_WHEEL)  ? ev[curr_ev].value : 0;
			}

			if (rx || ry) handle(NITEVENT_TYPE_MOTION, 0, rx, ry);
			if (wx || wy) handle(NITEVENT_TYPE_WHEEL,  0, wx, wy);

			continue;
		}

		if (ev[curr_ev].type == EV_KEY) {

			int type = ev[curr_ev].value ? NITEVENT_TYPE_PRESS : NITEVENT_TYPE_RELEASE;
			int code = ev[curr_ev].code;

			/* l4input delivers wrong keycode for PRINT, patch this */
			if (code == KEY_SYSRQ) code = KEY_PRINT;

			curr_ev++;

			/* handle key press/release event */
			handle(type, code, 0, 0);

			continue;
		}

		/* skip any other event */
		curr_ev++;
	}
}


/*** INITIALIZE INPUT SUBSYSTEM ***/
int input_init(void) {

	TRY(!l4_calibrate_tsc(), "L4_calibrare_tsc failed");

	TRY(l4input_init(l4io_page ? l4io_page->omega0 : 0,
	                 0xc0, NULL),
	                 "Initialization of l4input failed");

	return 0;
}
