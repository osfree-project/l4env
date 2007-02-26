/*
 * \brief   Overlay Screen library - interface functions
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "ovl_screen.h"
#include "map.h"

extern CORBA_Object ovl_screen_srv;   /* defined in init.c */
static CORBA_Environment env = dice_default_environment;


/*** INTERFACE: OPEN NEW OVERLAY SCREEN ***/
int ovl_screen_open(int w, int h, int depth) {
	return overlay_open_screen_call(ovl_screen_srv, w, h, depth, &env);
}


/*** INTERFACE: CLOSE OVERLAY SCREEN ***/
int ovl_screen_close(void) {
	overlay_close_screen_call(ovl_screen_srv, &env);
	return 0;
}


/*** INTERFACE: MAP FRAME BUFFER OF OVERLAY SCREEN ***/
void *ovl_screen_map(void) {
	char *ds_ident;
	overlay_map_screen_call(ovl_screen_srv, &ds_ident, &env);
	printf("libovlscreen(map): ds_ident = %s\n",ds_ident);
	return ovl_screen_get_framebuffer(ds_ident);
}


/*** INTERFACE: REFRESH OVERLAY SCREEN AREA ***/
void ovl_screen_refresh(int x, int y, int w, int h) {
	overlay_refresh_screen_call(ovl_screen_srv, x, y, w, h, &env);
}

