#include "dopelib-config.h"
#include "dopeapp-server.h"
#include "dopelib.h"
#include "events.h"
#include "app_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void dopelib_usleep(int);

CORBA_long dopeapp_listener_event_component(CORBA_Object *_dice_corba_obj,
                                            dope_event_u *e,
                                            CORBA_char_ptr bindarg,
                                            CORBA_Environment *_dice_corba_env) {
	
	struct dopelib_app_struct *app = dopelib_apps[(int)_dice_corba_env->user_data];
	long first = app->first;
	dope_event *event = &app->event_queue[first];
	
	app->first = (first+1)%EVENT_QUEUE_SIZE;
	
	/* check if we have to deallocate event stuff */
	if (event->type == EVENT_TYPE_COMMAND) {
		free(event->command.cmd);
	}

	strncpy(app->bindarg_queue[first],bindarg,250);
	
	switch (e->_d) {
	/* COMMAND EVENT */
	case 1:
		event->type = EVENT_TYPE_COMMAND;
		event->command.cmd = malloc(256);
		strncpy(event->command.cmd,e->_u.command.cmd,250);
		break;
		
	/* MOTION EVENT */
	case 2:
		event->type = EVENT_TYPE_MOTION;
		event->motion.rel_x = e->_u.motion.rel_x;
		event->motion.rel_y = e->_u.motion.rel_y;
		event->motion.abs_x = e->_u.motion.abs_x;
		event->motion.abs_y = e->_u.motion.abs_y;
		break;

	/* PRESS EVENT */
	case 3:
		event->type = EVENT_TYPE_PRESS;
		event->press.code = e->_u.press.code;
		break;
		
	/* RELEASE EVENT */
	case 4:
		event->type = EVENT_TYPE_RELEASE;
		event->release.code = e->_u.release.code;
		break;

	/* UNDEFINED EVENT */
	default:
		event->type = EVENT_TYPE_UNDEFINED;
		break;
	}

	return 42;
}


/*** WAIT FOR AN EVENT ***/
void dopelib_wait_event(int id, dope_event **e_out,char **bindarg_out) {
	struct dopelib_app_struct *app = dopelib_apps[id];
	long curr = app->last;
	/* !! TEST !! should be made with the use of a mutex later */
//	printf("first=%lu last=%lu\n",first,last);
	while (app->first == app->last) dopelib_usleep(1000);
	app->last = (app->last+1)%EVENT_QUEUE_SIZE;
	if (bindarg_out) *bindarg_out = &app->bindarg_queue[curr][0];
	if (e_out) *e_out=&app->event_queue[curr];
}
