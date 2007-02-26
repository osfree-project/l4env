/*
 * \brief	DOpE server module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is the main communication interface
 * between DOpE clients and DOpE.
 */


/*** L4-SPECIFIC ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include "dope-server.h"

/*** DOPE SPECIFIC ***/
#include "dope-config.h"
#include "memory.h"
#include "thread.h"
#include "server.h"
#include "appman.h"
#include "script.h"
#include "messenger.h"
#include "input.h"


static struct input_services     *input;
static struct memory_services    *mem;
static struct thread_services    *thread;
static struct appman_services    *appman;
static struct script_services    *script;
static struct messenger_services *msg;

int init_server(struct dope_services *d);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


static void server_thread(void *arg) {

	if (!names_register("DOpE")) {
		ERROR(printf("can't register at names\n"));
		return;
	}
	INFO(printf("Server(server_thread): entering server loop\n"));
	dope_manager_server_loop(NULL);
}


/*** CONVERTS THREAD IDENTIFIER STRING INTO L4 THREAD ID TYPE ***/
static l4_threadid_t *threadident_to_tid(const char *ident) {
	u32 low=0, high=0;
	l4_threadid_t *new_tid;
	while ((*ident>='0') && (*ident<='9')) high = high*10 + (*(ident++) & 0xf);
	if (*ident == 0) return NULL;
	ident++;
	while ((*ident>='0') && (*ident<='9')) low  =  low*10 + (*(ident++) & 0xf);
	new_tid = (l4_threadid_t *)mem->alloc(sizeof(l4_threadid_t));
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

CORBA_long dope_manager_init_app_component(CORBA_Object *_dice_corba_obj,
                                           CORBA_char_ptr appname,
                                           CORBA_char_ptr listener,
                                           CORBA_Environment *_dice_corba_env) {
	s32 app_id = appman->reg_app((char *)appname);
	l4_threadid_t *listener_tid = threadident_to_tid(listener);
	
	INFO(printf("Server(init_app): application init request. appname=%s, listener=%s\n",appname,listener));

	appman->reg_listener(app_id,listener_tid);
	return app_id;
}


CORBA_void dope_manager_deinit_app_component(CORBA_Object *_dice_corba_obj,
                                             CORBA_long appid,
                                             CORBA_Environment *_dice_corba_env) {
	l4_threadid_t *listener_tid = appman->get_listener(appid);

	INFO(printf("Server(deinit_app): application (id=%lu) deinit requested\n",(u32)appid);)

	mem->free(listener_tid);
	appman->reg_listener(appid,NULL);
	appman->unreg_app(appid);
}


CORBA_long dope_manager_exec_cmd_component(CORBA_Object *_dice_corba_obj,
                                           CORBA_long appid,
                                           CORBA_char_ptr cmd,
                                           CORBA_char_ptr *result,
                                           CORBA_Environment *_dice_corba_env) {
	char *res;
	appman->reg_app_thread(appid,_dice_corba_obj);
	
	INFO(printf("Server(exec_cmd): cmd %s execution requested by app_id=%lu\n",cmd,(u32)appid);)
	res = script->exec_command(appid,(char *)cmd);
	INFO(printf("Server(exec_cmd): send result msg: %s\n",res));

	*result=res;
	return 0;
}


CORBA_long dope_manager_get_keystate_component(CORBA_Object *_dice_corba_obj,
                                               CORBA_long keycode,
                                               CORBA_Environment *_dice_corba_env) {
	return input->get_keystate(keycode);
}


CORBA_char dope_manager_get_ascii_component(CORBA_Object *_dice_corba_obj,
                                            CORBA_long keycode,
                                            CORBA_Environment *_dice_corba_env) {
	return input->get_ascii(keycode);
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** START SERVING ***/
static void start(void) {
	DOPEDEBUG(printf("Server(start): creating server thread\n");)
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

	mem    = d->get_module("Memory 1.0");
	thread = d->get_module("Thread 1.0");
	appman = d->get_module("ApplicationManager 1.0");
	script = d->get_module("Script 1.0");
	msg    = d->get_module("Messenger 1.0");
	input  = d->get_module("Input 1.0");
	
	d->register_module("Server 1.0",&services);
	return 1;
}
