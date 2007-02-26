/*
 * \brief   DOpE server module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is the main communication interface
 * between DOpE clients and DOpE.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** L4-SPECIFIC ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/events/events.h>
#include "dope-server.h"

/*** DOPE SPECIFIC ***/
#include "dopestd.h"
#include "thread.h"
#include "server.h"
#include "appman.h"
#include "script.h"
#include "scope.h"
#include "screen.h"
#include "messenger.h"
#include "userstate.h"
#include <l4/dope/dopedef.h>

static struct userstate_services *userstate;
static struct thread_services    *thread;
static struct appman_services    *appman;
static struct script_services    *script;
static struct scope_services     *scope;
static struct screen_services    *screen;
static struct messenger_services *msg;

struct thread {
	l4_threadid_t tid;
};

extern int config_events;

int init_server(struct dope_services *d);


/**********************************
 *** FUNCTIONS FOR INTERNAL USE ***
 **********************************/

/*** DOpE SERVER THREAD ***/
static void server_thread(void *arg) {
	CORBA_Server_Environment env = dice_default_server_environment;
	l4thread_set_prio(l4thread_myself(), l4thread_get_prio(l4thread_myself())-1);

	if (!names_register("DOpE")) {
		ERROR(printf("can't register at names\n"));
		return;
	}
	INFO(printf("Server(server_thread): entering server loop\n"));
	dope_manager_server_loop(&env);
}


/******************************************************
 *** FUNCTIONS THAT ARE CALLED BY THE SERVER THREAD ***
 ******************************************************/

long dope_manager_init_app_component(CORBA_Object _dice_corba_obj,
                                           const char* appname,
                                           const char* listener,
                                           CORBA_Server_Environment *_dice_corba_env) {
	s32 app_id = appman->reg_app((char *)appname);
	SCOPE *rootscope = scope->create();
	THREAD *listener_thread = thread->alloc_thread();
	appman->set_rootscope(app_id, rootscope);

	INFO(printf("Server(init_app): application init request. appname=%s, listener=%s (new app_id=%x)\n", appname, listener, (int)app_id));

	thread->ident2thread(listener, listener_thread);
	appman->reg_list_thread(app_id, listener_thread);
	appman->reg_listener(app_id, (void *)&listener_thread->tid);
	return app_id;
}


void dope_manager_deinit_app_component(CORBA_Object _dice_corba_obj,
                                       long app_id,
                                       CORBA_Server_Environment *_dice_corba_env) {
	struct thread client_thread = { *_dice_corba_obj };

	/* check if the app_id belongs to the client */
	if (!thread->thread_equal(&client_thread, appman->get_listener(app_id))) {
		printf("Server(deinit_app): Error: permission denied\n");
		return;
	}

	INFO(printf("Server(deinit_app): application (id=%lu) deinit requested\n", (u32)app_id);)
	screen->forget_children(app_id);
	appman->unreg_app(app_id);
}


long dope_manager_exec_cmd_component(CORBA_Object _dice_corba_obj,
                                     long app_id,
                                     const char* cmd,
                                     CORBA_Server_Environment *_dice_corba_env) {
	struct thread client_thread = { *_dice_corba_obj };
	int ret;

	/* check if the app_id belongs to the client */
	if (!thread->thread_equal(&client_thread, appman->get_listener(app_id))) {
		printf("Server(exec_cmd): Error: permission denied\n");
		return DOPE_ERR_PERM;
	}

	appman->reg_app_thread(app_id, (THREAD *)&client_thread);
	INFO(printf("Server(exec_cmd): cmd %s execution requested by app_id=%lu\n", cmd, (u32)app_id);)
	//printf("Server(exec_cmd): cmd %s execution requested by app_id=%lu\n", cmd, (u32)app_id);
	appman->lock(app_id);
	ret = script->exec_command(app_id, (char *)cmd, NULL, 0);
	appman->unlock(app_id);
	if (ret < 0) printf("DOpE(exec_cmd): Error - command \"%s\" returned %d\n", cmd, ret);
	return ret;
}


long dope_manager_exec_req_component(CORBA_Object _dice_corba_obj,
                                     long app_id,
                                     const char* cmd,
                                     char result[256],
                                     int *res_len,
                                     CORBA_Server_Environment *_dice_corba_env) {
	struct thread client_thread = { *_dice_corba_obj };
	int ret;

	/* check if the app_id belongs to the client */
	if (!thread->thread_equal(&client_thread, appman->get_listener(app_id))) {
		printf("Server(exec_req): Error: permission denied\n");
		return DOPE_ERR_PERM;
	}

	appman->reg_app_thread(app_id, (THREAD *)&client_thread);

	INFO(printf("Server(exec_req): cmd %s execution requested by app_id=%lu\n", cmd, (u32)app_id);)
	appman->lock(app_id);
	result[0] = 0;
	ret = script->exec_command(app_id, (char *)cmd, &result[0], *res_len);
	appman->unlock(app_id);
	INFO(printf("Server(exec_req): send result msg: %s\n", result));

	if (ret < 0) printf("DOpE(exec_req): Error - command \"%s\" returned %d\n", cmd, ret);
	result[255] = 0;
	return ret;
}


long dope_manager_get_keystate_component(CORBA_Object _dice_corba_obj,
                                               long keycode,
                                               CORBA_Server_Environment *_dice_corba_env) {
	return userstate->get_keystate(keycode);
}


char dope_manager_get_ascii_component(CORBA_Object _dice_corba_obj,
                                            long keycode,
                                            CORBA_Server_Environment *_dice_corba_env) {
	return userstate->get_ascii(keycode);
}


/****************************
 *** DROPS EVENTS HANDLER ***
 ****************************/

/*** DROPS EVENTS HANDLER THREAD ***/
static void events_thread(void *arg) {
	l4events_ch_t event_ch = L4EVENTS_EXIT_CHANNEL;
	l4events_nr_t event_nr = L4EVENTS_NO_NR;
	l4events_event_t event;
	l4_threadid_t tid;
	int res, app_id;

	/* init event lib and register for event */
	l4events_init();
	l4events_register(event_ch, 14);

	INFO(printf("Server(events_thread): events receiver thread is up.\n"));
	
	/* event loop */
	while (1) {

		/* wait for event */
		res = l4events_give_ack_and_receive(&event_ch, &event, &event_nr,
		                                    L4_IPC_NEVER, L4EVENTS_RECV_ACK);
		if (res != L4EVENTS_OK) continue;

		/* determine affected application id */
		tid = *(l4_threadid_t*)event.str;
		app_id = appman->app_id_of_thread((THREAD *)&tid);
		if (app_id < 0) continue;

		/* deinit corresponding DOpE application */
		dope_manager_deinit_app_component(&tid, app_id, NULL);
	}
}


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** START SERVING ***/
static void start(void) {

	INFO(printf("Server(start): creating server thread\n");)
	thread->start_thread(NULL, &server_thread, NULL);

	INFO(printf("Server(start): creating events thread\n");)
	if (config_events) thread->start_thread(NULL, &events_thread, NULL);
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct server_services services = {
	start,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_server(struct dope_services *d) {
	thread    = d->get_module("Thread 1.0");
	appman    = d->get_module("ApplicationManager 1.0");
	script    = d->get_module("Script 1.0");
	msg       = d->get_module("Messenger 1.0");
	userstate = d->get_module("UserState 1.0");
	scope     = d->get_module("Scope 1.0");
	screen    = d->get_module("Screen 1.0");

	d->register_module("Server 1.0", &services);
	return 1;
}
