/*
 * \brief   DOpE mini top
 * \date    2004-01-10
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a small example program for using the
 * LoadDisplay widget of DOpE.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"
#include "getvalues.h"
#include "thread.h"

#define DISPLAY_MAX_THREADS 15

static long app_id;

static void update_thread(void *arg) {
	char reqbuf[32];
	int i, max;
	float update_rate;
	
	while (1) {

		/* request update rate */
		dope_req(app_id, reqbuf, 32, "update.value");
		update_rate = strtod(reqbuf, (char **)NULL);
		
		/* request and sort timing values */
		minitop_get_values();
		minitop_sort_values();
		max = minitop_get_num();
		if (max > DISPLAY_MAX_THREADS) max = DISPLAY_MAX_THREADS;

		/* update bars for available threads */
		for (i=0; i<max; i++) {
			dope_cmdf(app_id, "tid%d.set(-text \"%x.%x\")",
			 i+1, minitop_get_taskid(i), minitop_get_threadid(i));
			dope_cmdf(app_id, "ld%d.barconfig(load, -value %d)",
			 i+1, (int)minitop_get_percent(i));
			dope_cmdf(app_id, "usec%d.set(-text \"%lu usec\")",
			 i+1, minitop_get_usec(i));

		}

		/* set remaining bars to zero */
		for ( ; i<DISPLAY_MAX_THREADS; i++) {
			dope_cmdf(app_id, "ld%d.barconfig(load, -value 0)", i+1);
		}

		/* wait until next update */
		minitop_usleep((long)(update_rate * 1000.0 * 1000.0));
	}
}


int main(int argc,char **argv) {
	int i;

	native_startup(argc,argv);

	if (dope_init()) return -1;

	app_id = dope_init_app("MiniTop");

	#include "minitop.dpi"
	
	/* create list of load displays and place them into a grid */
	for (i=1; i<=DISPLAY_MAX_THREADS; i++) {
		dope_cmdf(app_id, "tid%d = new Label()", i);
		dope_cmdf(app_id, "dg.place(tid%d, -column 1 -row %d -align e)", i,i);
		dope_cmdf(app_id, "tid%d.set(-text \"0.0\")", i);
		
		dope_cmdf(app_id, "ld%d = new LoadDisplay()", i);
		dope_cmdf(app_id, "dg.place(ld%d, -column 2 -row %d)", i,i);
		dope_cmdf(app_id, "ld%d.barconfig(load, -value %d)", i,i*4);
		
		dope_cmdf(app_id, "usec%d = new Label()", i);
		dope_cmdf(app_id, "dg.place(usec%d, -column 3 -row %d)", i,i);
		dope_cmdf(app_id, "usec%d.set(-text \"0\")", i);
	}
	dope_cmd( app_id, "df.set(-content dg)");
	dope_cmdf(app_id, "dispwin.set(-workw 215 -workh 1000)");
	
	/* start update thread */
	minitop_thread_create(update_thread, NULL);
	
	dope_eventloop(app_id);
	return 0;
}
