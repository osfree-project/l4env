#include "dopelib-config.h"
#include "dopelib.h"
#include "listener.h"
#include "sync.h"
#include <dopeapp-server.h>
#include <stdio.h>
#include <pthread.h>

extern int get_free_port(void);

static char *listener_ident;
static struct dopelib_mutex *listener_init_mutex;

static void *listener_thread(void *arg) {
	CORBA_char listener_ident_buf[128];
	CORBA_Environment listener_env = dice_default_environment;
	int listener_port = get_free_port();
	
	listener_ident = &listener_ident_buf[0];
	sprintf(listener_ident, "%d", listener_port);
	
	listener_env.srv_port = listener_port;
	listener_env.user_data = arg;
	
	dopelib_unlock_mutex(listener_init_mutex);
	dopeapp_listener_server_loop(&listener_env);
	return NULL;
}


char *dopelib_start_listener(long id) {
	pthread_t listener_tid;
	
	/* start action listener */
	listener_init_mutex = dopelib_create_mutex(1);
	INFO(printf("DOpElib(dopelib_start_listener): creating server thread\n"));
	pthread_create(&listener_tid,NULL,listener_thread,(void *)id);
	INFO(printf("DOpElib(dopelib_start_listener): created.\n"));
	dopelib_lock_mutex(listener_init_mutex);
	INFO(printf("DOpElib(dopelib_start_listener): listener_ident = %s\n",listener_ident);)
	dopelib_destroy_mutex(listener_init_mutex);
	listener_init_mutex = NULL;
	
	INFO(printf("DOpElib(dopelib_start_listener): finished.\n");)
	return listener_ident;
}

