#include "stdlib.h"
#include "pthread.h"

/*** LOCAL INCLUDES ***/
#include "thread.h"

void thread_create(void (*entry)(void *),void *arg) {
	pthread_t* tid = malloc(sizeof(pthread_t));
	pthread_create(tid, NULL, (void *(*)(void *))entry, arg);
}

