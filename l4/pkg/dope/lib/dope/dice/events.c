/*
 * \brief   Event handling of DOpE client library
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "dopeapp-server.h"
#include "dopestd.h"
#include "dopelib.h"
#include "events.h"
#include "sync.h"
#include "app_struct.h"

extern void dopelib_usleep(int);


int dopelib_init_eventqueue(int id) {
	dopelib_apps[id]->queue_sem = dopelib_sem_create(1);
	return dopelib_apps[id]->queue_sem ? 0 : -1;
}


long dopeapp_listener_event_component(CORBA_Object _dice_corba_obj,
                                      const dope_event_u *e,
                                      const char* bindarg,
                                      CORBA_Server_Environment *_dice_corba_env) {
	struct dopelib_app *app = dopelib_apps[(int)_dice_corba_env->user_data];
	long first = app->first;
	dope_event *event = &app->event_queue[first];

	/* do not handle events for non-initialized event queue */
	if (!app->queue_sem) {
		printf("dopeapp_listener_event_component: event queue for app_id=%d no initialized\n",
		       (int)app->app_id);
		return -1;
	}

	app->first = (first + 1) % EVENT_QUEUE_SIZE;

	strncpy(app->bindarg_queue[first], bindarg, 250);

	switch (e->_d) {

	/* command event */
	case 1:
		event->type = EVENT_TYPE_COMMAND;
		strncpy(&app->bindcmd_queue[first][0], e->_u.command.cmd, EVENT_CMD_SIZE);
		break;
		
	/* motion event */
	case 2:
		event->type = EVENT_TYPE_MOTION;
		event->motion.rel_x = e->_u.motion.rel_x;
		event->motion.rel_y = e->_u.motion.rel_y;
		event->motion.abs_x = e->_u.motion.abs_x;
		event->motion.abs_y = e->_u.motion.abs_y;
		break;

	/* press event */
	case 3:
		event->type = EVENT_TYPE_PRESS;
		event->press.code = e->_u.press.code;
		break;
		
	/* release event */
	case 4:
		event->type = EVENT_TYPE_RELEASE;
		event->release.code = e->_u.release.code;
		break;

	/* keyrepeat event */
	case 5:
		event->type = EVENT_TYPE_KEYREPEAT;
		event->keyrepeat.code = e->_u.keyrepeat.code;
		break;

	/* undefined event */
	default:
		event->type = EVENT_TYPE_UNDEFINED;
		break;
	}

	/* signal incoming event */
	if (app->queue_sem) dopelib_sem_post(app->queue_sem);
	return 42;
}


/*** INTERFACE: RETURN NUMBER OF PENDING EVENTS ***/
int dope_events_pending(int id) {
	struct dopelib_app *app = get_app(id);
	return (app ? (app->first - app->last) : 0);
}


/*** WAIT FOR AN EVENT ***/
void dopelib_wait_event(int id, dope_event **e_out,char **bindarg_out) {
	struct dopelib_app *app = get_app(id);
	long curr;

	if (!app) return;
	curr = app->last;

	/* wait for an incoming event */
	dopelib_sem_wait(app->queue_sem);
	app->last = (app->last + 1) % EVENT_QUEUE_SIZE;

	/* fill out the results of the function */
	if (bindarg_out) *bindarg_out = &app->bindarg_queue[curr][0];
	if (e_out) *e_out = &app->event_queue[curr];
}
