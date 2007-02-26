#include "dopelib-config.h"
#include "dopelib.h"
#include "sync.h"
#include <dope-client.h>
#include <stdio.h>
#include <pthread.h>

CORBA_Object dope_server;

struct dopelib_mutex *dopelib_cmdf_mutex;
struct dopelib_mutex *dopelib_cmd_mutex;

void dopelib_usleep(int usec);
void dopelib_usleep(int usec) {
	usleep(usec);
}


long dope_init(void) {
	
	/* create mutex to make dope_cmdf and dope_cmd thread save */
	dopelib_cmdf_mutex = dopelib_create_mutex(0);
	dopelib_cmd_mutex  = dopelib_create_mutex(0);
	
	/* init dope server object */
	dope_server.sin_family = AF_INET;
	dope_server.sin_port = htons(9997);
	inet_aton("127.0.0.1", &dope_server.sin_addr);
	
	INFO(printf("DOpElib(dope_init): dope_init finished.\n");)
	return 0;
}


