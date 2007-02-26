/*
 * \brief   Overlay Window library - connect to overlay wm server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "window_listener-server.h"
#include "ovl_window.h"

static CORBA_Environment env = dice_default_environment;
static l4_threadid_t ovl_tid;
CORBA_Object ovl_window_srv = &ovl_tid;


/*** DICE MEMORY ALLOCATION FUNCTION ***/
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


/*** WINDOW EVENT LISTENER THREAD ***/
static pthread_t listener_pid;
static void *listener_pthread(void *arg) {
	l4_threadid_t mytid = l4_myself();

	printf("libovlwindow(listener_pthread): register listener (%x.%x) ar Overlay Server\n",
	 (int)mytid.id.task, (int)mytid.id.lthread);
	overlay_window_listener_call(ovl_window_srv,
	                             &mytid,
	                             &env);
	printf("libovlwindow(listener_pthread): entering serverloop\n");
	window_listener_server_loop(NULL);
	return NULL;
}


/*** INTERFACE: INIT OVERLAY WINDOW LIBRARY ***/
int ovl_window_init(char *ovl_name) {
//	l4thread_t listener;
//	l4_threadid_t listener_tid;

	if (!ovl_name) ovl_name = "OvlWM";
	printf("libovlwindow(init): ask names for %s\n",ovl_name);
	while (names_waitfor_name(ovl_name, ovl_window_srv, 2000) == 0) {
		printf("libovlwindow(init): %s is not registered at names!\n",ovl_name);
	}

	printf("libovlwindow(init): create window event listener thread\n");
	pthread_create(&listener_pid, NULL, &listener_pthread, NULL);
	return 0;
}

