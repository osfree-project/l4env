/*
 * \brief   DOpE client library - L4 specific initialisation
 * \date    2002-11-13
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

#include "dopestd.h"
#include "dopeapp-server.h"
#include "dopelib.h"
#include <stdio.h>
#include <stdlib.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include "sync.h"

l4_threadid_t        l4_dope_server;
CORBA_Object         dope_server;

struct dopelib_mutex *dopelib_cmdf_mutex;
struct dopelib_mutex *dopelib_cmd_mutex;

void dopelib_usleep(int usec);
void dopelib_usleep(int usec) {
	l4_sleep(usec/1000);
}


long dope_init(void) {
//	l4thread_init();
	INFO(printf("DOpElib(dope_init): ask for 'DOpE' at names...\n");)
	dope_server = &l4_dope_server;
	while (names_waitfor_name("DOpE", dope_server, 1000) == 0) {
		ERROR(printf("DOpE is not registered at names!\n");)
	}
	INFO(printf("DOpElib(dope_init): found some DOpE.\n");)
	
	/* create mutex to make dope_cmdf and dope_cmd thread save */
	dopelib_cmdf_mutex = dopelib_create_mutex(0);
	dopelib_cmd_mutex  = dopelib_create_mutex(0);
	
	return 0;
}

