/*
 * \brief   DOpE client library - event listener
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
#include <l4/thread/thread.h>
#include "listener.h"


char *listener_ident; /* !!! protection by mutex needed! */


static void listener_thread(void *arg) {
	char listener_ident_buf[128];
	l4_threadid_t listener_tid;
	CORBA_Environment dice_env = dice_default_environment;
	
	dice_env.user_data = arg;
	
	listener_tid = l4thread_l4_id( l4thread_myself() );
	snprintf(listener_ident_buf,127,"%lu %lu",(unsigned long)listener_tid.lh.high,
	                                          (unsigned long)listener_tid.lh.low);
	listener_ident = &listener_ident_buf[0];
	INFO(printf("%s\n",listener_ident_buf));
	l4thread_started(NULL);
	INFO(printf("DOpElib(listener_thread): entering server loop\n");)
	dopeapp_listener_server_loop(&dice_env);
}


char *dopelib_start_listener(long id) {
	
	INFO(printf("DOpElib(dope_init): start listener.\n");)
	l4thread_create(listener_thread,(void *)id,L4THREAD_CREATE_SYNC);
	
	INFO(printf("DOpElib(dope_init): listener_ident = %s\n",listener_ident);)
	INFO(printf("DOpElib(dope_init): dope_init finished.\n");)
	return listener_ident;
}
