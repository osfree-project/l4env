/*
 * \brief   Overlay Input library - connect to overlay wm server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "input_listener-server.h"
#include "freeport.h"
#include "ovl_input.h"

static struct sockaddr_in ovl_input_sockaddr;
static CORBA_Environment env = dice_default_environment;
CORBA_Object ovl_input_srv = &ovl_input_sockaddr;


/*** DICE MEMORY ALLOCATION FUNCTION ***/
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


/*** INPUT EVENT LISTENER SERVER THREAD ***/
static void *listener_thread(void *arg) {
	CORBA_Environment listener_env = dice_default_environment;
	struct sockaddr listener_sockaddr;
	CORBA_Object listener_srv = (CORBA_Object)&listener_sockaddr;
	
	listener_env.srv_port = get_free_port();
	
	/* define CORBA_Object of the input listener */
	listener_srv->sin_family = AF_INET;
	listener_srv->sin_port   = listener_env.srv_port;
	inet_aton("127.0.0.1", &listener_srv->sin_addr);

	printf("libovlinput(listener_thread): using port %d\n", (int)listener_srv->sin_port);
	
	/* register input listener at the overlay server */
	overlay_input_listener_call(ovl_input_srv, listener_srv, &env);
	
	input_listener_server_loop(&listener_env);
}


/*** INTERFACE: INIT OVERLAY INPUT LIBRARY ***/
int ovl_input_init(char *ovl_name) {
	pthread_t listener_tid;
	
	/* define ovl_input_srv */
	ovl_input_sockaddr.sin_family = AF_INET;
	ovl_input_sockaddr.sin_port = htons(13246);
	inet_aton("127.0.0.1", &ovl_input_sockaddr.sin_addr);

	/* create listener thread and tell the overlay server about it */
	pthread_create(&listener_tid, NULL, listener_thread, NULL);
	
	return 0;
}

