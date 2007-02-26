/*
 * \brief   DOpE server module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is the main communication interface
 * between DOpE clients and DOpE.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** DOPE SPECIFIC ***/
#include "dopestd.h"
#include "thread.h"
#include "server.h"
#include "appman.h"
#include "script.h"
#include "input.h"

#include <stdlib.h>   /* only for strtol */
#include <dope-server.h>

int init_server(struct dope_services *d);

static struct thread_services *thread;
static struct appman_services *appman;
static struct script_services *script;
static struct input_services  *input;


long dope_manager_init_app_component(CORBA_Object _dice_corba_obj,
                                           const char* appname,
                                           const char* listener,
                                           CORBA_Environment *_dice_corba_env) {
	CORBA_Object l = malloc(sizeof(struct sockaddr));
	s32 appid = appman->reg_app(appname);
	if (!l) {
		ERROR(printf("Server(dope_manager_init_app): out of memory!\n"););
		return 0;
	}
	l->sin_family = AF_INET;
	l->sin_port = strtol(listener,NULL,10);
	inet_aton("127.0.0.1", &l->sin_addr);
	appman->reg_listener(appid,l);
	INFO(printf("Server(init_app): application init requested. appname=%s listener=%s\n",appname,listener);)
	return appid;
}


void dope_manager_deinit_app_component(CORBA_Object _dice_corba_obj,
                                             long appid,
                                             CORBA_Environment *_dice_corba_env) {
	INFO(printf("Server(deinit_app): application (id=%lu) deinit requested\n",appid);)
	appman->unreg_app(appid);
}


long dope_manager_exec_cmd_component(CORBA_Object _dice_corba_obj,
                                           long appid,
                                           const char* cmd,
                                           char* *result,
                                           CORBA_Environment *_dice_corba_env) {
//  INFO(printf("Server(exec_cmd): cmd %s execution requested by app_id=%lu\n",cmd,appid);)
	*result = script->exec_command(appid,cmd);
//  INFO(printf("Server(exec_cmd): return value is %s\n",*result);)
	return 0;
}


long dope_manager_get_keystate_component(CORBA_Object _dice_corba_obj,
                                               long keycode,
                                               CORBA_Environment *_dice_corba_env) {
	return input->get_keystate(keycode);
}


char dope_manager_get_ascii_component(CORBA_Object _dice_corba_obj,
                                            long keycode,
                                            CORBA_Environment *_dice_corba_env) {
	return input->get_ascii(keycode);
}


/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static void server_thread(void *arg) {
	static CORBA_Environment dopesrv_env;
	INFO(printf("Server(server_thead): entering server mainloop\n");)
	dopesrv_env.srv_port = htons(9997);
	dope_manager_server_loop(&dopesrv_env);
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** START SERVING ***/
static void start(void) {
	INFO(printf("Server(start): creating server thread\n");)
	thread->create_thread(&server_thread,NULL);
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct server_services services = {
	start,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_server(struct dope_services *d) {

	thread = d->get_module("Thread 1.0");
	appman = d->get_module("ApplicationManager 1.0");
	script = d->get_module("Script 1.0");
	input  = d->get_module("Input 1.0");

	d->register_module("Server 1.0",&services);
	return 1;
}
