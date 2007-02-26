/*
 * \brief   VTextScreen Test
 * \date    2004-03-02
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <string.h>

/*** DOpE SPECIFIC INCLUDES ***/
#include "dopestd.h"
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"

#define SCR_W 80
#define SCR_H 256


int main(int argc,char **argv) {
	long app_id;           /* DOpE application id */
	u8 *cbuf_adr;          /* VTextScreen character buffer */
	u8 *abuf_adr;          /* VTextScreen attribute buffer */
	int i, j = 0;

	native_startup(argc,argv);
	
	/* init DOpE library */
	dope_init();
	
	/* register DOpE-application */
	app_id = dope_init_app("VTextScreen Test");
	
	dope_cmd( app_id, "win = new Window()" );
	dope_cmd( app_id, "f   = new Frame()" );
	dope_cmd( app_id, "vts = new VTextScreen()" );
	dope_cmdf(app_id, "vts.setmode(%d, %d, C8A8PLN)", SCR_W, SCR_H );
	dope_cmd (app_id, "f.set(-scrollx yes -scrolly yes -content vts)" );
	dope_cmd( app_id, "win.set(-background off -content f)" );
	dope_cmd( app_id, "win.open()" );

	/* map vscreen buffer to local address space */
	cbuf_adr = vscr_get_fb(app_id, "vts");
	abuf_adr = cbuf_adr + SCR_W*SCR_H;
	
	if (!cbuf_adr) {
		printf("VTextScreen Test: Damn, unable to map text screen buffer!\n");
		return -1;
	}

	/* draw some crap into text buffer */
	for (j=0; j<SCR_H; j++) {
		int bright = (j & 0x3);
		int fgcol  = (j>>2) & 0x7;
		int bgcol  = (j>>5) & 0x7;
		snprintf(&cbuf_adr[j*SCR_W], SCR_W,
		       "This is text with attribute %x (foreground %d, background %d, brightness %d)",
		       j, fgcol, bgcol, bright);
		for (i=0; i<SCR_W; i++) abuf_adr[j*SCR_W + i] = (bright<<6) | (fgcol<<3) | bgcol;
	}
	dope_cmd(app_id, "vts.refresh(-x 5 -y 6 -w 65 -h 15)");

	/* enter mainloop (not really .-) */
	dope_eventloop(app_id);
	return 0;
}

