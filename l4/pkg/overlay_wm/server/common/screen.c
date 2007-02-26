/*
 * \brief   OverlayWM - overlay screen handling
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component provides the overlay screen abstraction.
 * Such a screen is represented on DOpE via a VScreen widget.
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
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"
#include "thread.h"
#include "main.h"
#include "input.h"

extern int ovl_num_windows;   /* from window.c */

int ovl_scr_w, ovl_scr_h, ovl_scr_depth;

static int ovl_scr_initialized;


/*** EVENT CALLBACK: SET NATIVE SIZE OF OVERLAY SCREEN WINDOW ***/
static void ovl_set_native_size_callback(dope_event *e, void *arg) {
	dope_cmdf(app_id, "a.set(-workw %d -workh %d)", ovl_scr_w, ovl_scr_h + 23);
}



/*** IDL INTERFACE: REQUEST INFORMATION ABOUT PHYSICAL SCREEN ***/
int overlay_get_screen_info_component(CORBA_Object _dice_corba_obj,
                                      int *w, int *h, int *mode,
                                      CORBA_Server_Environment *_dice_corba_env) {
	*w = phys_scr_w;
	*h = phys_scr_h;
	*mode = 16;
	return 0;
}


/*** IDL INTERFACE: OPEN OVERLAY SCREEN ***/
int overlay_open_screen_component(CORBA_Object _dice_corba_obj,
                                 int width,
                                 int height,
                                 int depth,
                                 CORBA_Server_Environment *_dice_corba_env) {
	char *pixelformat = NULL;
	
	switch (depth) {
	case 16:
		pixelformat = "RGB16";
		break;
	default:
		printf("OvlWM(init_screen): color depth %d is not supported.\n", depth);
		return -1;
	}
	if (ovl_scr_initialized) {

		/* set new VScreen mode if its dimensions or color depth changed */
		if ((ovl_scr_w != width) || (ovl_scr_h != height)
		 || (ovl_scr_depth != depth)) {
			dope_cmdf(app_id, "vscr.setmode(%d,%d,\"%s\")", width, height, pixelformat);
		}
	} else {
		dope_cmd(app_id,  "a = new Window()" );
		dope_cmd(app_id,  "vscr = new VScreen(-grabfocus yes)" );
		dope_cmd(app_id,  "menuframe = new Frame()" );
		dope_cmd(app_id,  "natsize = new Button()" );
		dope_cmd(app_id,  "natsize.set(-text \"Set Native Size\")" );
		dope_cmd(app_id,  "info = new Label()" );
		dope_cmd(app_id,  "menugrid = new Grid()" );
		dope_cmd(app_id,  "menugrid.place(natsize, -column 1 -row 1)" );
		dope_cmd(app_id,  "menugrid.place(info, -column 2 -row 1 -align ns)" );
		dope_cmd(app_id,  "menugrid.columnconfig(1, -size 128)" );
		dope_cmd(app_id,  "maingrid = new Grid()" );
		dope_cmd(app_id,  "a.set(-content maingrid -background off)" );
		dope_cmd(app_id,  "menubg = new Background()" );
		dope_cmd(app_id,  "menubg.set(-content menugrid)" );
		dope_cmd(app_id,  "maingrid.place(menubg, -column 1 -row 1)" );
		dope_cmd(app_id,  "maingrid.place(vscr, -column 1 -row 2)" );
		dope_cmdf(app_id, "vscr.setmode(%d,%d,\"%s\")", width, height, pixelformat);
		dope_cmdf(app_id, "a.set(-x 100 -y 150 -workw %d -workh %d)", width, height + 23);
		dope_bind(app_id, "vscr", "press"  , ovl_input_press_callback,(void *)0);
		dope_bind(app_id, "vscr", "release", ovl_input_release_callback,(void *)0);
		dope_bind(app_id, "vscr", "motion",  ovl_input_motion_callback,(void *)0);
		dope_bind(app_id, "natsize", "click", ovl_set_native_size_callback,(void *)0);
	}
	dope_cmdf(app_id, "info.set(-text \"%dx%d (%s)\")", width, height, pixelformat);
	dope_cmd(app_id,  "a.open()");

	ovl_scr_w = width;
	ovl_scr_h = height;
	ovl_scr_depth = depth;
	ovl_scr_initialized = 1;
	return 0;
}


/*** IDL INTERFACE: CLOSE OVERLAY SCREEN ***/
void overlay_close_screen_component(CORBA_Object _dice_corba_obj,
                                   CORBA_Server_Environment *_dice_corba_env) {
	int i;
	for (i = 0; i < ovl_num_windows; i++) dope_cmdf(app_id, "win%d.close()", i);

	/*
	 * We just close the window but keep the VScreen to use it for the next
	 * open call. This way, we do not need to reallocate the VScreen buffer for
	 * the common case of restarting the X server.
	 */
	dope_cmd(app_id, "a.close()");
}


/*** IDL INTERFACE: MAP FRAMEBUFFER OF OVERLAY SCREEN ***/
int overlay_map_screen_component(CORBA_Object _dice_corba_obj,
                                char* *ds_ident,
                                CORBA_Server_Environment *_dice_corba_env) {
	static char strbuf[128];
	static char retbuf[256];
	*ds_ident = retbuf;
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
                                    CORBA_Server_Environment *_dice_corba_env) {

	dope_cmdf(app_id, "vscr.refresh(-x %d -y %d -w %d -h %d)", x, y, w, h);
}

