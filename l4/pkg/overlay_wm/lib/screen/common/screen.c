/*
 * \brief   Overlay Screen library - interface functions
 * \date    2003-08-18
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
#include <stdlib.h>
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "ovl_screen.h"
#include "map.h"

extern CORBA_Object ovl_screen_srv;   /* defined in init.c */
extern int ovl_phys_width, ovl_phys_height, ovl_phys_mode;

static CORBA_Environment env = dice_default_environment;
static void *fb_addr;


/*** INTERFACE: REQUEST WIDTH OF PHYSICAL SCREEN ***/
int ovl_get_phys_width(void) {
	return ovl_phys_width;
}


/*** INTERFACE: REQUEST HEIGHT OF PHYSICAL SCREEN ***/
int ovl_get_phys_height(void) {
	return ovl_phys_height;
}


/*** INTERFACE: REQUEST COLOR MODE OF PHYSICAL SCREEN ***/
int ovl_get_phys_mode(void) {
	return ovl_phys_mode;
}


/*** INTERFACE: OPEN NEW OVERLAY SCREEN ***/
int ovl_screen_open(int w, int h, int depth) {
	return overlay_open_screen_call(ovl_screen_srv, w, h, depth, &env);
}


/*** INTERFACE: CLOSE OVERLAY SCREEN ***/
int ovl_screen_close(void) {
	if (fb_addr) ovl_screen_release_framebuffer(fb_addr);
	overlay_close_screen_call(ovl_screen_srv, &env);
	return 0;
}


/*** INTERFACE: MAP FRAME BUFFER OF OVERLAY SCREEN ***/
void *ovl_screen_map(void) {
	char *ds_ident;
	env.malloc = (dice_malloc_func)&malloc;
	overlay_map_screen_call(ovl_screen_srv, &ds_ident, &env);
	printf("libovlscreen(map): ds_ident = %s\n",ds_ident);
	fb_addr = ovl_screen_get_framebuffer(ds_ident);
	return fb_addr;
}


/*** INTERFACE: REFRESH OVERLAY SCREEN AREA ***/
void ovl_screen_refresh(int x, int y, int w, int h) {
	overlay_refresh_screen_call(ovl_screen_srv, x, y, w, h, &env);
}

