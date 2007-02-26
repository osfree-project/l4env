/*
 * \brief   Overlay Screen library - connect to overlay wm server
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

/*** LOCAL INCLUDES ***/
#include "overlay-client.h"
#include "ovl_screen.h"

static struct sockaddr_in ovl_screen_sockaddr;
CORBA_Object ovl_screen_srv = &ovl_screen_sockaddr;

/*** DICE MEMORY ALLOCATION FUNCTION ***/
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


/*** INTERFACE: INIT OVERLAY SCREEN LIBRARY ***/
int ovl_screen_init(char *ovl_name) {
	ovl_screen_sockaddr.sin_family = AF_INET;
	ovl_screen_sockaddr.sin_port = htons(13246);
	inet_aton("127.0.0.1", &ovl_screen_sockaddr.sin_addr);
	return 0;
}

