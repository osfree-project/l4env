/*
 * \brief   Simple overlay window test client
 * \date    2003-08-23
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
#include "ovl_window.h"

static void winpos_callback(int id, int x, int y, int w, int h) {
	printf("replaced window %d: %d,%d,%d,%d\n", id, x, y, w, h);
}


int l4libc_heapsize = 1024*1024;

int main(int argc, char **argv) {
	int w1, w2;
	
	/* init overlay window library */
	ovl_window_init(NULL);

	ovl_winpos_callback(&winpos_callback);
	
	w1 = ovl_window_create();
	ovl_window_place(w1,30,40,260,170);
	ovl_window_open(w1);
	
	w2 = ovl_window_create();
	ovl_window_place(w2,130,50,180,360);
	ovl_window_open(w2);

	while (1);
}
