/*** L4 INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h> 

/*** LOCAL INCLUDES ***/
#include "thread.h"

void thread_create(void (*entry)(void *),void *arg) {
	l4thread_create(entry,arg,L4THREAD_CREATE_ASYNC);
}




