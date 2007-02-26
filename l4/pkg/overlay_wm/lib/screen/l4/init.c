/*
 * \brief   Overlay Screen library - connect to overlay wm server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "ovl_screen.h"

static l4_threadid_t ovl_tid;
CORBA_Object ovl_screen_srv = &ovl_tid;


/*** DICE MEMORY ALLOCATION FUNCTION ***/
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


/*** INTERFACE: INIT OVERLAY SCREEN LIBRARY ***/
int ovl_screen_init(char *ovl_name) {
	if (!ovl_name) ovl_name = "OvlWM";
	while (names_waitfor_name(ovl_name, ovl_screen_srv, 2000) == 0) {
		printf("%s is not registered at names!\n",ovl_name);
	}
	printf("found %s at names\n", ovl_name);
	return 0;
}

