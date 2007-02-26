/*
 * \brief   Overlay Input library - interface functions
 * \date    2003-08-19
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
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
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "input_listener-server.h"
#include "ovl_input.h"


static void (*button_callback)(int type, int code);
static void (*motion_callback)(int mx, int my);


/********************************************
 * IDL EVENT LISTENER SERVER IMPLEMENTATION *
 ********************************************/

/*** IDL INTERFACE: BUTTON EVENT LISTENER ***
 *
 * \param type  type of event (1=press, 2=release)
 * \param code  key or button code
 */
void input_listener_button_component(CORBA_Object _dice_corba_obj,
                                     int type,
                                     int code,
                                     CORBA_Server_Environment *_dice_corba_env) {
//	printf("input_listener_button_component(%d,%d)\n", type, code);
	if (button_callback) button_callback(type, code);
}


/*** IDL INTERFACE: MOTION EVENT LISTENER ***
 *
 * \param abs_x, abs_y   new absolute mouse position
 * \param rel_x, rel_y   relative position change
 */
void input_listener_motion_component(CORBA_Object _dice_corba_obj,
                                     int abs_x, int abs_y,
                                     int rel_x, int rel_y,
                                     CORBA_Server_Environment *_dice_corba_env) {
//	printf("input_listener_motion_component(%d,%d,%d,%d)\n",abs_x,abs_y,rel_x,rel_y);
	if (motion_callback) motion_callback(abs_x, abs_y);
}


/*********************
 * LIBRARY INTERFACE *
 *********************/

/*** INTERFACE: REGISTER CALLBACK FUNCTION FOR BUTTON INPUT EVENTS ***/
int ovl_input_button(void (*cb)(int type, int code)) {
	button_callback = cb;
	return 0;
}


/*** INTERFACE: REGISTER CALLBACK FUNCTION FOR MOTION INPUT EVENTS ***/
int ovl_input_motion(void (*cb)(int mx, int my)) {
	motion_callback = cb;
	return 0;
}

