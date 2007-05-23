/*
 * \brief   Overlay Input library - connect to overlay wm server
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
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "input_listener-server.h"
#include "ovl_input.h"

static l4_threadid_t ovl_tid;
CORBA_Object ovl_input_srv = &ovl_tid;


/*** INTERFACE: INIT OVERLAY INPUT LIBRARY ***/
int ovl_input_init(char *ovl_name) {

	if (!ovl_name) ovl_name = "OvlWM";
	printf("libovlinput(init): ask names for %s\n",ovl_name);
	while (names_waitfor_name(ovl_name, ovl_input_srv, 2000) == 0) {
		printf("libovlinput(init): %s is not registered at names!\n",ovl_name);
	}
	return 0;
}


/*** INTERFACE: REGISTER AS LISTENER AND PROCESS INPUT EVENTS ***/
int ovl_input_eventloop(void) {
	l4_threadid_t listener_tid = l4_myself();
	CORBA_Environment env = dice_default_environment;

	printf("libovlinput(init): register listener at overlay server\n");
	overlay_input_listener_call(ovl_input_srv,
	                            &listener_tid,
	                            &env);

	input_listener_server_loop(NULL);
	return 0;
}

