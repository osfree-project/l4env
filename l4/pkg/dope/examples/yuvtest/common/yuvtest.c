/*
 * \brief   YUV Test
 * \date    2003-05-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** DOpE SPECIFIC INCLUDES ***/
#include "dopestd.h"
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"
#include "bird_yuv.h"

#define SCR_W 128
#define SCR_H 128

static void press_callback (dope_event * e, void *arg) {
	printf("press callback called!\n");
}

int main(int argc,char **argv) {
	long app_id;           /* DOpE application id */
	u8 *scr_adr;           /* VScreen pixel buffer */

	native_startup(argc,argv);
	
	/* init DOpE library */
	if (dope_init()) return -1;
	
	/* register DOpE-application */
	app_id = dope_init_app("YUV-Test");
	
	/* open window with rt-widget */
	dope_cmd( app_id, "yuvwin=new Window()" );
	dope_cmd( app_id, "yuvvscr=new VScreen()" );
	dope_cmdf(app_id, "yuvvscr.setmode(%d,%d,\"YUV420\")", SCR_W, SCR_H );
	dope_cmd( app_id, "yuvwin.set(-background off -content yuvvscr)" );
	dope_cmd( app_id, "yuvwin.open()" );

	/* example event binding */
	dope_bind( app_id, "yuvvscr", "press", press_callback, (void *)0);
	
	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb(app_id, "yuvvscr");
	
	/* copy YUV image to VScreen buffer */
	memcpy(scr_adr, &bird_yuv[0], SCR_W*SCR_H + (SCR_W*SCR_H/2));

	/* update vscreen widget */
	dope_cmd( app_id, "yuvvscr.refresh()" );

	/* enter mainloop */
	dope_eventloop(app_id);
	return 0;
}

