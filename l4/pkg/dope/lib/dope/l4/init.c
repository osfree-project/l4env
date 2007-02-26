#include "dopelib-config.h"
#include "dopeapp-server.h"
#include "dopelib.h"
#include <stdio.h>
#include <stdlib.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>
//#include "listener.h"
#include "sync.h"
//#include "init.h"

CORBA_Object         dope_server;

struct dopelib_mutex *dopelib_cmdf_mutex;
struct dopelib_mutex *dopelib_cmd_mutex;

void dopelib_usleep(int usec);
void dopelib_usleep(int usec) {
	l4_sleep(usec/1000);
}


long dope_init(void) {
	l4thread_init();
	INFO(printf("DOpElib(dope_init): ask for 'DOpE' at names...\n");)
	while (names_waitfor_name("DOpE", &dope_server, 1000) == 0) {
		ERROR(printf("DOpE is not registered at names!\n");)
	}
	INFO(printf("DOpElib(dope_init): found some DOpE.\n");)
	
	/* create mutex to make dope_cmdf and dope_cmd thread save */
	dopelib_cmdf_mutex = dopelib_create_mutex(0);
	dopelib_cmd_mutex  = dopelib_create_mutex(0);
	
	return 0;
}

