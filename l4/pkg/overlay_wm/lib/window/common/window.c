/*
 * \brief   Overlay Window library - interface functions
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
#include "overlay-client.h"
#include "window_listener-server.h"
#include "ovl_window.h"

extern CORBA_Object ovl_window_srv;   /* defined in init.c */

static void (*winplace_callback) (int win_id, int x, int y, int w, int h);
static void (*winclose_callback) (int win_id);
static void (*wintop_callback)   (int win_id);


/********************************************
 * IDL EVENT LISTENER SERVER IMPLEMENTATION *
 ********************************************/

/*** IDL INTERFACE: WINDOW TOP EVENT LISTENER ***/
void window_listener_top_component(CORBA_Object _dice_corba_obj,
                                   int win_id,
                                   CORBA_Server_Environment *_dice_corba_env) {
	if (wintop_callback) wintop_callback(win_id);
}


/*** IDL INTERFACE: WINDOW POSITION EVENT LISTENER ***/
void window_listener_place_component(CORBA_Object _dice_corba_obj,
                                     int win_id,
                                     int x, int y, int w, int h,
                                     CORBA_Server_Environment *_dice_corba_env){
	if (winplace_callback) winplace_callback(win_id, x, y, w, h);
}


/*** IDL INTERFACE: WINDOW CLOSE EVENT LISTENER ***/
void window_listener_close_component(CORBA_Object _dice_corba_obj,
                                     int win_id,
                                     CORBA_Server_Environment *_dice_corba_env){
	if (winclose_callback) winclose_callback(win_id);
}


/*********************
 * LIBRARY INTERFACE *
 *********************/

/*** INTERFACE: CREATE NEW WINDOW FOR OVERLAY SCREEN ***/
int ovl_window_create(void) {
	CORBA_Environment env = dice_default_environment;
	return overlay_create_window_call(ovl_window_srv, &env);
}


/*** INTERFACE: DESTROY OVERLAY WINDOW ***/
void ovl_window_destroy(int win_id) {
	CORBA_Environment env = dice_default_environment;
	overlay_destroy_window_call(ovl_window_srv, win_id, &env);
}


/*** INTERFACE: OPEN OVERLAY WINDOW ***/
void ovl_window_open(int win_id) {
	CORBA_Environment env = dice_default_environment;
	overlay_open_window_call(ovl_window_srv, win_id, &env);
}


/*** INTERFACE: CLOSE OVERLAY WINDOW ***/
void ovl_window_close(int win_id) {
	CORBA_Environment env = dice_default_environment;
	overlay_close_window_call(ovl_window_srv, win_id, &env);
}


/*** INTERFACE: SET THE SIZE AND POSITION OF AN OVERLAY WINDOW ***/
void ovl_window_place(int win_id, int x, int y, int w, int h) {
	CORBA_Environment env = dice_default_environment;
	overlay_place_window_call(ovl_window_srv, win_id, x, y, w, h, &env);
}


/*** INTERFACE: TOP THE SPECIFIED OVERLAY WINDOW ***/
void ovl_window_stack(int win_id, int neighbor_id, int behind, int do_redraw) {
	CORBA_Environment env = dice_default_environment;
	overlay_stack_window_call(ovl_window_srv, win_id, neighbor_id, behind,
	                          do_redraw, &env);
}


/*** INTERFACE: SET TITLE OF OVERLAY WINDOW ***/
void ovl_window_title(int win_id, const char *title) {
	CORBA_Environment env = dice_default_environment;
	overlay_title_window_call(ovl_window_srv, win_id, title, &env);
}


/*** INTERFACE: DEFINE BACKGROUND OVERLAY WINDOW ***/
void ovl_set_background(int bg_win_id) {
	CORBA_Environment env = dice_default_environment;
	overlay_set_background_call(ovl_window_srv, bg_win_id, &env);
}


/*** INTERFACE: REGISTER CALLBACK FOR WINDOW PLACEMENT CHANGES ***/
int ovl_winpos_callback(void (*cb)(int,int,int,int,int)) {
	winplace_callback = cb;
	return 0;
}


/*** INTERFACE: REGISTER CALLBACK FOR WINDOW CLOSE EVENTS ***/
int ovl_winclose_callback(void (*cb)(int)) {
	winclose_callback = cb;
	return 0;
}


/*** INTERFACE: REGISTER CALLBACK FOR WINDOW TOP EVENTS ***/
int ovl_wintop_callback(void (*cb)(int)) {
	wintop_callback = cb;
	return 0;
}


/*** !!! THIS IS JUST A QUICK HACK !!! */
static void *priv[1024];

void ovl_window_set_private(int win_id, void *private) {
	priv[win_id] = private;
}

void *ovl_window_get_private(int win_id) {
	return priv[win_id];
}
