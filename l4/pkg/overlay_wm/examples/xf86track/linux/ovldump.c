/*
 * \brief   Window Event tracker for the X Window System
 * \date    2004-01-19
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Dump all requests to the overlay wm interface.
 */

/*
 * Copyright (C) 2004-2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** OVERLAY INCLUDES ***/
#include "ovl_window.h"


int ovl_window_create(void) {
	static int id_cnt;
	id_cnt++;
	printf("create window %d\n", id_cnt);
	return id_cnt;
}


void ovl_window_place(int id, int x, int y, int w, int h) {
	printf("place window %d to xywh=%d,%d,%d,%d\n", id, x, y, w, h);
}


void ovl_window_open(int id) {
	printf("open window %d\n", id);
}


void ovl_window_destroy(int id) {
	printf("destroy window %d\n", id);
}


void ovl_window_top(int id) {
	printf("top window %d\n", id);
}


int ovl_window_init(char *name) {
	return 0;
}
