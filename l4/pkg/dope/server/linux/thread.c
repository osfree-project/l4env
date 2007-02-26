/*
 * \brief	DOpE thread handling module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Component that provides functions to  handle 
 * threads and related things (e.g. semaphores) 
 * to the other components of DOpE.
 */ 

#define THREAD SDL_Thread
#define MUTEX  SDL_mutex

#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "dope-config.h"
#include "thread.h"

int init_thread(struct dope_services *d);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static THREAD *create_thread(void (*entry)(void *),void *arg) {
	return SDL_CreateThread((int (*)(void *))entry,arg);
}

static MUTEX *create_mutex(int init_locked) {
	return SDL_CreateMutex();
}

static void destroy_mutex(MUTEX *m) {
	SDL_DestroyMutex(m);
}

static void lock_mutex(MUTEX *m) {
	SDL_mutexP(m);
}

static void unlock_mutex(MUTEX *m) {
	SDL_mutexV(m);
}

static s8 mutex_is_down(MUTEX *m) {
	return 0;
}

static THREAD *ident2thread(u8 *ident) {
	return NULL;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct thread_services services = {
	create_thread,
	create_mutex,
	destroy_mutex,
	lock_mutex,
	unlock_mutex,
	mutex_is_down,	
	ident2thread,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_thread(struct dope_services *d) {
	
	d->register_module("Thread 1.0",&services);
	return 1;
}
