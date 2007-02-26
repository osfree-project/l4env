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


/*** L4-SPECIFIC ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include "dope-server.h"

/*** DOPE SPECIFIC ***/
#include "dopestd.h"
#include "thread.h"
#include "server.h"
#include "appman.h"
#include "script.h"
#include "messenger.h"
#include "input.h"

#define MIN(a,b) (a<b?a:b)

static struct input_services     *input;
static struct thread_services    *thread;
static struct appman_services    *appman;
static struct script_services    *script;
static struct messenger_services *msg;

int init_server(struct dope_services *d);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


static void server_thread(void *arg) {
	CORBA_Environment env = dice_default_environment;
	env.timeout = L4_IPC_TIMEOUT(250,14,0,0,0,0); /* send timeout 1ms */

	if (!names_register("DOpE")) {
		ERROR(printf("can't register at names\n"));
		return;
	}
	INFO(printf("Server(server_thread): entering server loop\n"));
	dope_manager_server_loop(&env);
}


/*** CONVERTS THREAD IDENTIFIER STRING INTO L4 THREAD ID TYPE ***/
static l4_threadid_t *threadident_to_tid(const char *ident) {
	u32 low=0, high=0;
	l4_threadid_t *new_tid;
	while ((*ident>='0') && (*ident<='9')) high = high*10 + (*(ident++) & 0xf);
	if (*ident == 0) return NULL;
	ident++;
	while ((*ident>='0') && (*ident<='9')) low  =  low*10 + (*(ident++) & 0xf);
	new_tid = (l4_threadid_t *)malloc(sizeof(l4_threadid_t));
	if (!new_tid) {
		ERROR(printf("Server(threadident_to_tid): out of memory!\n"));
		return NULL;
	}
	new_tid->lh.high = high;
	new_tid->lh.low  = low;
	return new_tid;
}



/******************************************************/
/*** FUNCTIONS THAT ARE CALLED BY THE SERVER THREAD ***/
/******************************************************/

long dope_manager_init_app_component(CORBA_Object _dice_corba_obj,
                                           const char* appname,
                                           const char* listener,
                                           CORBA_Environment *_dice_corba_env) {
	s32 app_id = appman->reg_app((char *)appname);
	l4_threadid_t *listener_tid = threadident_to_tid(listener);

	INFO(printf("Server(init_app): application init request. appname=%s, listener=%s\n",appname,listener));

	appman->reg_listener(app_id,listener_tid);
	return app_id;
}


void dope_manager_deinit_app_component(CORBA_Object _dice_corba_obj,
                                             long appid,
                                             CORBA_Environment *_dice_corba_env) {
	l4_threadid_t *listener_tid = appman->get_listener(appid);

	INFO(printf("Server(deinit_app): application (id=%lu) deinit requested\n",(u32)appid);)

	free(listener_tid);
	appman->reg_listener(appid,NULL);
	appman->unreg_app(appid);
}


long dope_manager_exec_cmd_component(CORBA_Object _dice_corba_obj,
                                     long appid,
                                     const char* cmd,
                                     CORBA_Environment *_dice_corba_env) {
	appman->reg_app_thread(appid,_dice_corba_obj);

	INFO(printf("Server(exec_cmd): cmd %s execution requested by app_id=%lu\n",cmd,(u32)appid);)
	script->exec_command(appid,(char *)cmd);
	INFO(printf("Server(exec_cmd): send result msg: %s\n",res));
	return 0;
}


long dope_manager_exec_req_component(CORBA_Object _dice_corba_obj,
                                     long appid,
                                     const char* cmd,
                                     char result[256],
                                     int *res_len,
                                     CORBA_Environment *_dice_corba_env) {
	char *res;
	int num_bytes;
	appman->reg_app_thread(appid,_dice_corba_obj);

	INFO(printf("Server(exec_req): cmd %s execution requested by app_id=%lu\n",cmd,(u32)appid);)
	res = script->exec_command(appid,(char *)cmd);
	INFO(printf("Server(exec_req): send result msg: %s\n",res));

	num_bytes = MIN( MIN(255, strlen(res)+1), *res_len);
//	printf("Server(exec_req): copy result (%d bytes)\n", num_bytes);
	
	memcpy(result, res, num_bytes);
	result[255] = 0;
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
	msg    = d->get_module("Messenger 1.0");
	input  = d->get_module("Input 1.0");

	d->register_module("Server 1.0",&services);
	return 1;
}
