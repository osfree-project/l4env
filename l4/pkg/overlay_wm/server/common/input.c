/*
 * \brief   OverlayWM - user input handling
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This file  contains the handling of user input events  which
 * apply on the overlay  screen or one of the  overlay windows.
 * User input events are  the pressing and releasing of key and
 * mouse buttons and mouse movements.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"
#include "input_listener-client.h"
#include "main.h"
#include "input.h"


static CORBA_Environment env = dice_default_environment;
static CORBA_Object_base input_event_listener_base;
static CORBA_Object input_event_listener;


/******************************
 * DOPE INPUT EVENT CALLBACKS *
 ******************************/

/*** DOPE PRESS EVENT CALLBACK ***/
void ovl_input_press_callback(dope_event *e, void *arg) {
	if (!input_event_listener) return;
	input_listener_button_call(input_event_listener, 1, e->press.code, &env);
}


/*** DOPE RELEASE EVENT CALLBACK ***/
void ovl_input_release_callback(dope_event *e, void *arg) {
	if (!input_event_listener) return;
	input_listener_button_call(input_event_listener, 2, e->release.code, &env);
}


/*** DOPE MOTION EVENT CALLBACK ***/
void ovl_input_motion_callback(dope_event *e, void *arg) {
	if (!input_event_listener) return;
	input_listener_motion_call(input_event_listener,
	                           e->motion.abs_x, e->motion.abs_y,
	                           e->motion.rel_x, e->motion.rel_y,
	                           &env);
}


/*****************
 * IDL INTERFACE *
 *****************/

/*** IDL INTERFACE: REGISTER INPUT EVENT LISTENER THREAD ***/
void overlay_input_listener_component(CORBA_Object _dice_corba_obj,
                                      const_CORBA_Object listener,
                                      CORBA_Server_Environment *_dice_corba_env) {
	printf("overlay_input_listener_component: called\n");
	input_event_listener_base = *listener;
	input_event_listener = &input_event_listener_base;
}

