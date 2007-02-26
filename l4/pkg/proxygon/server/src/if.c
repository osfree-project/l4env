/*
 * \brief   Proxygon if server
 * \date    2004-09-30
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/l4con/l4con.h>
#include <l4/l4con/l4con-server.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/dope/dopelib.h>
#include <l4/util/macros.h>

/*** LOCAL INCLUDES ***/
#include "if.h"
#include "vc.h"

l4_threadid_t if_tid;

/*************************************
 *** IF SERVER COMPONENT FUNCTIONS ***
 *************************************/

/*** OPEN NEW VIRTUAL CONSOLE ***/
long
con_if_openqry_component(CORBA_Object _dice_corba_obj,
                         unsigned long sbuf1_size,
                         unsigned long sbuf2_size,
                         unsigned long sbuf3_size,
                         unsigned char priority,
                         l4_threadid_t *vcid,
                         short vfbmode,
                         CORBA_Server_Environment *_dice_corba_env) {

	/* start vc server thread */
	return start_vc_server(sbuf1_size, sbuf2_size, sbuf3_size,
	                       priority, vcid, vfbmode);
}


/*** CLOSE ALL VIRTUAL CONSOLES THAT BELONG TO THE SPECIFIED CLIENT ***/
long
con_if_close_all_component(CORBA_Object _dice_corba_obj,
                           const l4_threadid_t *client,
                           CORBA_Server_Environment *_dice_corba_env) {
	printf("close_all_component: client =  "l4util_idfmt"\n", l4util_idstr(*client));
	piss_off_client(*client);
	return 0;
}


/*** START IF SERVER THREAD AND REGISTER AT NAMES ***/
int start_if_server(void) {
	l4thread_t id;

	/* create if server thread */
	id = l4thread_create_named(con_if_server_loop, "proxygon-if",
	                           NULL, L4THREAD_CREATE_ASYNC);
	if (id <= 0) {
		printf("Error: could not create if server thread.\n");
		return -1;
	}

	/* remember thread id of interface thread */
	if_tid = l4thread_l4_id(id);

	/* register at names */
	if (!names_register_thread_weak(CON_NAMES_STR, if_tid)) {
		printf("Error: cannot register at names.\n");
		return -1;
	}
	return 0;
}
