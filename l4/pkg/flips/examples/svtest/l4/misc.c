/*** GENERAL INCLUDES ***/
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

/*** L4 SPECIFIC INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>

/*** LOCAL INCLUDES ***/
#include "pthread.h"


#ifndef USE_UCLIBC
void bzero(void *s, int n);
void bzero(void *s, int n) {
	memset(s, 0, n);
}
#endif


int pthread_create(pthread_t * thread, void * attr, void * (*start_routine)(void *), void * arg);
int pthread_create(pthread_t * thread, void * attr, void * (*start_routine)(void *), void * arg) {
	*thread = l4thread_create((void (*)(void *))start_routine, arg, L4THREAD_CREATE_ASYNC);
	if (*thread == -L4_ENOTHREAD) return -1;
	return 0;
}


void pthread_exit(void *retval);
void pthread_exit(void *retval) {
	l4thread_exit();
}


int pthread_join(pthread_t th, void **thread_return);
int pthread_join(pthread_t th, void **thread_return) {
	printf("svtest(misc.c): pthread_join called - not yet implemented\n");
	return -1;
}

void sleep(int sec);
void sleep(int sec) {
	l4_usleep(1000*1000*sec);
}


void gettimeofday(void);
void gettimeofday(void) {
	printf("svtest(misc.c): gettimeofday called - not yet implemented\n");
}


void gethostbyname(void);
void gethostbyname(void) {
	printf("svtest(misc.c): gethostbyname called - not yet implemented\n");
}
