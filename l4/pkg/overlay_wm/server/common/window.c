/*
 * \brief   OverlayWM - overlay window handling
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This file provides  the interface to create and manage
 * overlay windows. Additionally, it provides a mechanism
 * for  notifying  the client  about reconfigurations  of
 * overlay windows.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"
#include "window_listener-client.h"
#include "main.h"
#include "input.h"


static CORBA_Environment env = dice_default_environment;
static CORBA_Object_base window_event_listener_base;
static CORBA_Object window_event_listener;
int ovl_num_windows; /* number of created windows */


#define WIN_PAD_LEFT    5
#define WIN_PAD_RIGHT   5
#define WIN_PAD_TOP     22
#define WIN_PAD_BOTTOM  5


/*** EVENT CALLBACK: NEW SIZE AND POSITION OF OVERLAY WINDOW ***/
static void winplace_callback(dope_event *e, void *arg) {
	char retbuf[16];
	int win_id = (int)arg;
	int x,y,w,h;
	
	dope_reqf(app_id, retbuf, 15, "win%d.x", win_id);
	x = atoi(retbuf);
	dope_reqf(app_id, retbuf, 15, "win%d.y", win_id);
	y = atoi(retbuf);
	dope_reqf(app_id, retbuf, 15, "win%d.w", win_id);
	w = atoi(retbuf);
	dope_reqf(app_id, retbuf, 15, "win%d.h", win_id);
	h = atoi(retbuf);

//	printf("ovlsrv: winmove_callback called.\n");
	
	if (window_event_listener) {
		window_listener_place_call(window_event_listener, win_id,
		                           x + WIN_PAD_LEFT, y + WIN_PAD_TOP,
		                           w - WIN_PAD_LEFT - WIN_PAD_RIGHT,
		                           h - WIN_PAD_TOP  - WIN_PAD_BOTTOM, &env);
	}
	dope_cmdf(app_id,"frm%d.set(-xview %d -yview %d)",
              win_id, x + WIN_PAD_LEFT, y + WIN_PAD_TOP);
}


/*** EVENT CALLBACK: OVERLAY WINDOW GOT TOPPED ***/
static void wintop_callback(dope_event *e, void *arg) {
	int win_id = (int)arg;
	if (window_event_listener) {
		window_listener_top_call(window_event_listener, win_id, &env);
	}
}


/*** IDL INTERFACE: CREATE NEW WINDOW FOR OVERLAY SCREEN ***/
int overlay_create_window_component(CORBA_Object _dice_corba_obj,
                                    CORBA_Environment *_dice_corba_env) {
	static char strbuf[128];
	int win_id = ovl_num_windows++;
	
	dope_cmdf(app_id,"win%d = new Window()", win_id);
	dope_cmdf(app_id,"vscr%d = new VScreen()",win_id);
	dope_cmdf(app_id,"vscr%d.share(vscr)",win_id);
	dope_cmdf(app_id,"frm%d = new Frame()", win_id);
	dope_cmdf(app_id,"frm%d.set(-content vscr%d)", win_id, win_id);
	dope_cmdf(app_id,"win%d.set(-content frm%d -y 2000)",win_id, win_id);
	
	sprintf(strbuf,"win%d",win_id);
	dope_bind(app_id,strbuf,"moved",  winplace_callback, (void *)win_id);
	dope_bind(app_id,strbuf,"resized",winplace_callback, (void *)win_id);
	dope_bind(app_id,strbuf,"top",    wintop_callback,   (void *)win_id);
	
	sprintf(strbuf,"vscr%d",win_id);
	dope_bind(app_id, strbuf, "press"  , ovl_input_press_callback,   (void *)0);
	dope_bind(app_id, strbuf, "release", ovl_input_release_callback, (void *)0);
	dope_bind(app_id, strbuf, "motion",  ovl_input_motion_callback,  (void *)0);
	return win_id;
}


/*** IDL INTERFACE: DESTROY OVERLAY WINDOW ***/
void overlay_destroy_window_component(CORBA_Object _dice_corba_obj,
                                      int win_id,
                                      CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.close()",win_id);
}


/*** IDL INTERFACE: OPEN OVERLAY WINDOW ***/
void overlay_open_window_component(CORBA_Object _dice_corba_obj,
                                   int win_id,
                                   CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.open()",win_id);
}


/*** IDL INTERFACE: CLOSE OVERLAY WINDOW ***/
void overlay_close_window_component(CORBA_Object _dice_corba_obj,
                                  int win_id,
                                  CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.close()",win_id);
}


/*** IDL INTERFACE: SET THE SIZE AND POSITION OF AN OVERLAY WINDOW ***/
void overlay_place_window_component(CORBA_Object _dice_corba_obj,
                                  int win_id,
                                  int x, int y, int w, int h,
                                  CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.set(-x %d -y %d -w %d -h %d)", win_id,
	          x - WIN_PAD_LEFT, y - WIN_PAD_TOP,
	          w + WIN_PAD_LEFT + WIN_PAD_RIGHT,
	          h + WIN_PAD_TOP  + WIN_PAD_BOTTOM);
	dope_cmdf(app_id,"frm%d.set(-xview %d -yview %d)", win_id, x, y);
}


/*** IDL INTERFACE: TOP THE SPECIFIED OVERLAY WINDOW ***/
void overlay_top_window_component(CORBA_Object _dice_corba_obj,
                                int win_id,
                                CORBA_Environment *_dice_corba_env) {
	dope_cmdf(app_id,"win%d.top()",win_id);
}


/*** IDL INTERFACE: REGISTER WINDOW EVENT LISTENER THREAD ***/
void overlay_window_listener_component(CORBA_Object _dice_corba_obj,
                                       const CORBA_Object listener,
                                       CORBA_Environment *_dice_corba_env) {
//	printf("overlay_window_listener_component: called\n");
	window_event_listener_base = *listener;
	window_event_listener = &window_event_listener_base;
}
