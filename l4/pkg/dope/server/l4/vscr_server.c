/*
 * \brief   DOpE VScreen server module
 * \date    2002-01-04
 * \author  Norman Feske <no@atari.org>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <stdio.h>  /* !!! should be kicked out !!! */

#include "dopestd.h"
#include "thread.h"
#include "vscreen.h"
#include "vscr_server.h"

#include <l4/util/util.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include "vscr-server.h"

#define MAX_IDENTS 40

static struct thread_services *thread;

static u8 ident_tab[MAX_IDENTS];    /* vscreen server identifier table */
static s16 thread_started=0;

int init_vscr_server(struct dope_services *d);


/*****************************/
/*** VSCREEN WIDGET SERVER ***/
/*****************************/


void dope_vscr_waitsync_component(CORBA_Object _dice_corba_obj,
                                        CORBA_Environment *_dice_corba_env) {
	VSCREEN *vs = (VSCREEN *) _dice_corba_env->user_data;
	vs->vscr->waitsync(vs);
}

void dope_vscr_refresh_component(CORBA_Object _dice_corba_obj,
                                 int x,
                                 int y,
                                 int w,
                                 int h,
                                 CORBA_Environment *_dice_corba_env) {

	VSCREEN *vs = (VSCREEN *) _dice_corba_env->user_data;
	vs->vscr->refresh(vs,x,y,w,h);
}

static void vscreen_server_thread(void *arg) {
	int i;
	char ident_buf[10];
	VSCREEN *vs = (VSCREEN *)arg;
	CORBA_Environment dice_env = dice_default_environment;

	dice_env.user_data = vs;
	INFO(printf("VScreen(server_thread): entered server thread\n"));

	l4thread_set_prio(l4thread_myself(),l4thread_get_prio(l4thread_myself())-5);

//  INFO(printf("VScreen(server_thread): tid = %lu.%lu\n",
//              (long)(vs->server_tid.id.task),
//              (long)(vs->server_tid.id.lthread)));

	INFO(printf("VScreen(server_thread): find server identifier\n"));

	/* find free identifier for this vscreen server */
	for (i=0;(i<MAX_IDENTS) && (ident_tab[i]);i++);
	if (i<MAX_IDENTS-1) {
		sprintf(ident_buf,"Dvs%d",i);
		ident_tab[i]=1;
	} else {
		/* if there are not enough identifiers, exit the server thread */
		thread_started=1;
		ERROR(printf("VScreen(server_thread): no free identifiers\n"));
		return;
	}

	INFO(printf("VScreen(server_thread): ident_buf=%s\n",ident_buf));
	if (!names_register(ident_buf)) return;

	vs->vscr->reg_server(vs,ident_buf);
	thread_started = 1;
	INFO(printf("VScreen(server_thread): thread successfully started.\n"));
	dice_env.timeout = L4_IPC_TIMEOUT(250,14,0,0,0,0); /* send timeout 1ms */
	dope_vscr_server_loop(&dice_env);
}



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static THREAD *start(VSCREEN *vscr_widget) {
	THREAD *new;

	thread_started = 0;
	new = thread->create_thread(&vscreen_server_thread, (void *)vscr_widget);
	while (!thread_started) l4_sleep(1);
	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct vscr_server_services services = {
	start
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/


int init_vscr_server(struct dope_services *d) {
	thread = d->get_module("Thread 1.0");
	d->register_module("VScreenServer 1.0",&services);
	return 1;
}
