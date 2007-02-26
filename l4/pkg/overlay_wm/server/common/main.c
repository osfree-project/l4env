/*
 * \brief   OverlayWM - main program
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>

/*** LOCAL INCLUDES ***/
#include "overlay-server.h"   /* only because definition of CORBA_Object */
#include "startup.h"
#include "thread.h"
#include "serverloop.h"
#include "main.h"

int app_id;   /* DOpE application id of the this server */

int main(int argc, char **argv) {
	native_startup(argc,argv);
	dope_init();
	app_id = dope_init_app("Overlay Screen");
	thread_create(enter_overlay_server_loop,0);
	dope_eventloop(app_id);
	return 0;
}
