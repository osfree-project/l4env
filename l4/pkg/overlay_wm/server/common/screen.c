/*
 * \brief   OverlayWM - overlay screen handling
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component provides the overlay screen abstraction.
 * Such a screen is represented on DOpE via a VScreen widget.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"
#include "thread.h"
#include "main.h"
#include "input.h"

extern int ovl_num_windows;   /* from window.c */

static int scr_w, scr_h, scr_depth;
static void *vscr_server_id;

//static void scr_update_thread(void *arg) {
//	while (1) {
//		if (vscr_server_id) {
//			vscr_server_refresh(vscr_server_id, 0,0,scr_w-1,scr_h-1);
//		}
//		l4thread_usleep(1000*40);
//	}
//}

/*** IDL INTERFACE: OPEN OVERLAY SCREEN ***/
int overlay_open_screen_component(CORBA_Object _dice_corba_obj,
                                 int width,
                                 int height,
                                 int depth,
                                 CORBA_Environment *_dice_corba_env) {
	char *pixelformat = NULL;
	
	switch (depth) {
	case 16:
		pixelformat = "RGB16";
		break;
	default:
		printf("OvlWM(init_screen): color depth %d is not supported.\n", depth);
		return -1;
	}
	
	dope_cmd(app_id,  "a = new Window()");
	dope_cmd(app_id,  "vscr = new VScreen()");
	dope_cmdf(app_id, "vscr.setmode(%d,%d,\"%s\")", width, height, pixelformat);
//	dope_cmd(app_id,  "vscr.set(-framerate 25)");
	dope_cmdf(app_id,  "a.set(-x 100 -y 150 -w %d -h %d -scrollx yes -scrolly yes -background off -content vscr)",
	      width/2 + 10, height/2 + 27);
//	dope_cmd(app_id,  "a.open()");
	dope_bind(app_id, "vscr", "press"  , ovl_input_press_callback,(void *)0);
	dope_bind(app_id, "vscr", "release", ovl_input_release_callback,(void *)0);
	dope_bind(app_id, "vscr", "motion",  ovl_input_motion_callback,(void *)0);

	vscr_server_id = vscr_get_server_id(app_id,"vscr");
//	
//	thread_create(scr_update_thread,0);
	scr_w = width;
	scr_h = height;
	scr_depth = depth;
	return 0;
}


/*** IDL INTERFACE: CLOSE OVERLAY SCREEN ***/
void overlay_close_screen_component(CORBA_Object _dice_corba_obj,
                                   CORBA_Environment *_dice_corba_env) {
	int i;
	for (i=0; i < ovl_num_windows; i++) {
		dope_cmdf(app_id, "win%d.close()", i);
	}
	dope_cmd(app_id, "a.close()");
	dope_deinit_app(app_id);
	scr_w = scr_h = scr_depth = 0;
}


/*** IDL INTERFACE: MAP FRAMEBUFFER OF OVERLAY SCREEN ***/
int overlay_map_screen_component(CORBA_Object _dice_corba_obj,
                                char* *ds_ident,
                                CORBA_Environment *_dice_corba_env) {
	static char strbuf[128];
	thread2ident(_dice_corba_obj, &strbuf[0]);
	dope_reqf(app_id, *ds_ident, 255, "vscr.map(-thread \"%s\")",&strbuf[0]);
	return 0;
}


/*** IDL INTERFACE: REFRESH OVERLAY SCREEN AREA ***/
void overlay_refresh_screen_component(CORBA_Object _dice_corba_obj,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    CORBA_Environment *_dice_corba_env) {
	if (!vscr_server_id) {
		printf("OvlServer(refresh_screen_component): vscr_server_id undefined\n");
		return;
	}
//	printf("refresh(%d,%d,%d,%d)\n",x,y,w,h);
	vscr_server_refresh(vscr_server_id, x,y,w,h);
//	dope_cmdf(app_id,"vscr.refresh(-x %d -y %d -w %d -h %d)", x, y, w, h);
//	printf("refresh finished\n");
}

void overlay_bla_component(CORBA_Object _dice_corba_obj,
                           CORBA_Environment *_dice_corba_env) {
	static int i;
	printf("bla%d\n",i++);
}
