/*
 * \brief   DOpE VScreen demo
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a demonstration of the VScreen widget and the 
 * real-time  capabilities  of  DOpE.  It shows  several 
 * graphical effects that run at a constant frame rate.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>

/*** DOpE includes ***/
#include <dopelib.h>

/*** local includes ***/
#include "voxel.h"
#include "feedback.h"
#include "fountain.h"
#include "bump.h"
#include "thread.h"
#include "startup.h"

#define NUM_EFX 4

static struct efxwin {
	char *win_name;             /* name of the corresponding window widget */
	char *but_text;             /* text of the associated button */
	int flag;                   /* window flag closed (0) or open (1) */
	int  (*init)(void);         /* effect initialisation routine */
	void (*exec)(int);          /* effect execution routine */
} efxwin[NUM_EFX] = {
	{"voxwin",  "Landscape",  0, voxel_init,    voxel_exec},
	{"fntnwin", "Particles",  0, fountain_init, fountain_exec},
	{"feedwin", "Feedback",   0, feedback_init, feedback_exec},
	{"bumpwin", "Bumpmapping",0, bump_init,     bump_exec},
};

long   app_id;                  /* DOpE application id */
static char strbuf[256];        /* buffer for sprintf */


/*** THREAD, THAT IS STARTED FOR EACH EFFECT ***/
static void efx_thread(void *efx_id) {
	int id = (int)efx_id;
	efxwin[id].init();
	while (1) efxwin[id].exec(efxwin[id].flag);
}


/*** CALLBACK FOR BUTTON PRESS EVENTS - SWITCHING EFFECTS ON OR OFF ***/
static void press_callback(dope_event *e,void *arg) {
	int id = (int)arg;

	efxwin[id].flag ^= 1;
	if (efxwin[id].flag) {
		dope_cmdf(app_id, "b%d.set(-state 1)", id);
		dope_cmdf(app_id, "%s.open()", efxwin[id].win_name);
	} else {
		dope_cmdf(app_id,"b%d.set(-state 0)",id);
		dope_cmdf(app_id,"%s.close()",efxwin[id].win_name);
	}
}


int main(int argc,char **argv) {
	int i;

	native_startup(argc,argv);
	
	/* init DOpE library */
	dope_init();
	
	/* register DOpE-application */
	app_id = dope_init_app("VScrtest");

	/* create menu window with one button for each effect */
	dope_cmd(app_id,"mainwin = new Window()");
	dope_cmd(app_id,"mg = new Grid()");
	dope_cmd(app_id,"mainwin.set(-content mg -fitx yes -fity yes)");
	dope_cmd(app_id,"mainwin.set(-w 100 -h 120)");

	for (i=0;i<NUM_EFX;i++) {
		dope_cmdf(app_id, "b%d = new Button()",i);
		dope_cmdf(app_id, "b%d.set(-text \"%s\")",i,efxwin[i].but_text);
		dope_cmdf(app_id, "mg.place(b%d,-column 1 -row %d -padx 2 -pady 2)",i,i);
		sprintf(strbuf,"b%d",i);
		dope_bind(app_id,strbuf,"press", press_callback, (void *)i);
	}
	dope_cmd(app_id,"mainwin.open()");

	/* start effect threads */
	for (i=0;i<NUM_EFX;i++) {
		thread_create(efx_thread,(void *)i);
	}
		
	/* enter mainloop */
	dope_eventloop(app_id);
	return 0;
}
