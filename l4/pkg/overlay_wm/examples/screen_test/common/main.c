/*
 * \brief   Simple overlay screen test client
 * \date    2003-08-04
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
#include "ovl_screen.h"

int l4libc_heapsize = 1024*1024;

int main(int argc, char **argv) {
	unsigned short *scr_adr;
	int scr_w = 320, scr_h = 240;
	
	/* init overlay screen library */
	ovl_screen_init(NULL);

	/* open and map overlay screen */
	ovl_screen_open(scr_w, scr_h, 16);
	scr_adr = ovl_screen_map();

	/* paint some crap, again and again */
	while (1) {
		static int i,j;
		for (i=0;i<scr_w*scr_h;i++) scr_adr[i] = i + j;
		ovl_screen_refresh(10,10,200,150);
		j+=scr_w+1;
	}
	return 0;
}
