/*
 * \brief   Overlay Window library - connect to overlay wm server
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
#include <pthread.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "window_listener-server.h"
#include "freeport.h"
#include "ovl_window.h"

static struct sockaddr_in ovl_sockaddr;
CORBA_Object ovl_window_srv = &ovl_sockaddr;


/*** DICE MEMORY ALLOCATION FUNCTION ***/
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


/*** WINDOW EVENT LISTENER SERVER THREAD ***/
static void *listener_thread(void *arg) {
	CORBA_Environment listener_env = dice_default_environment;
	CORBA_Environment env = dice_default_environment;
	struct sockaddr listener_sockaddr;
	CORBA_Object listener_srv = (CORBA_Object)&listener_sockaddr;
	
	listener_env.srv_port = get_free_port();
	
	/* define CORBA_Object of the window listener */
	listener_srv->sin_family = AF_INET;
	listener_srv->sin_port   = listener_env.srv_port;
	inet_aton("127.0.0.1", &listener_srv->sin_addr);

	printf("libovlwindow(listener_thread): using port %d\n", (int)listener_srv->sin_port);
	
	/* register window listener at the overlay server */
	overlay_window_listener_call(ovl_window_srv, listener_srv, &env);
	
	window_listener_server_loop(&listener_env);
}


/*** INTERFACE: INIT OVERLAY WINDOW LIBRARY ***/
int ovl_window_init(char *ovl_name) {
	pthread_t listener_tid;
	
	/* define ovl_window_srv */
	ovl_sockaddr.sin_family = AF_INET;
	ovl_sockaddr.sin_port = htons(13246);
	inet_aton("127.0.0.1", &ovl_sockaddr.sin_addr);

	/* create listener thread and tell the overlay server about it */
	pthread_create(&listener_tid, NULL, listener_thread, NULL);
	
	return 0;
}

