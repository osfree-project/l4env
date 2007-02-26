/*
 * \brief   OverlayWM - main program
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
#include <string.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"   /* only because definition of CORBA_Object */
#include "startup.h"
#include "thread.h"
#include "serverloop.h"
#include "main.h"

int app_id;   /* DOpE application id of the this server */
char *overlay_name = "OvlWM";

int config_scale = 0;
int phys_scr_w;
int phys_scr_h;

int main(int argc, char **argv) {
	char reqbuf[32];
	char dopeapp_name[64];
	int i;
	native_startup(argc,argv);
	
	/* set identifier of overlay server if specified as argument */
	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "--name") && (i+1 < argc))
			overlay_name = argv[i+1];
	}
	
	dope_init();
	sprintf(dopeapp_name, "Fiasco->DOpE->%s", overlay_name);
	app_id = dope_init_app(dopeapp_name);
	
	dope_req(app_id, reqbuf, sizeof(reqbuf), "screen.w");
	phys_scr_w = atoi(reqbuf);
	dope_req(app_id, reqbuf, sizeof(reqbuf), "screen.h");
	phys_scr_h = atoi(reqbuf);
	
	thread_create(enter_overlay_server_loop,0);
	dope_eventloop(app_id);
	return 0;
}
