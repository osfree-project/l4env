/*
 * \brief   DOpE thread handling module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Component that provides functions to  handle
 * threads and related things (e.g. semaphores)
 * to the other components of DOpE.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include "dopestd.h"
#include "thread.h"

struct thread {
	l4_threadid_t tid;
};

struct mutex {
	l4semaphore_t sem;
	s8 locked_flag;
};


int init_thread(struct dope_services *d);


static unsigned long hex2u32(const char *s) {
	int i;
	unsigned long result=0;
	for (i=0;i<8;i++, s++) {
		if (!(*s)) return result;
		result = result*16 + (*s & 0xf);
		if (*s > '9') result += 9;
	}
	return result;
}


/*********************/
/* SERVICE FUNCTIONS */
/*********************/

/*** ALLOCATE THREAD STRUCTURE ***/
static THREAD *alloc_thread(void) {
	THREAD *new = (THREAD *)zalloc(sizeof(THREAD));
	return new;
}


/*** FREE THREAD STRUCTURE ***/
static void free_thread(THREAD *t) {
	if (!t) return;
	free(t);
}


/*** COPY CONTEXT OF A THREAD ***/
static void copy_thread(THREAD *src, THREAD *dst) {
	if (!src || !dst) return;
	dst->tid = src->tid;
}


/*** CREATE NEW THREAD AND RETURN THREAD IDENTIFIER ***
 *
 * \param dst_tid   out parameter to where the new thread id should be stored
 * \param entry     start function of new thread
 * \param arg       private argument to be passed to thread start function
 * \return          0 on success, otherwise a negative error code
 *
 * The dst_tid parameter can be NULL.
 */
static int start_thread(THREAD *dst_tid, void (*entry)(void *), void *arg) {
	l4thread_t new_thread = l4thread_create(entry, arg, L4THREAD_CREATE_ASYNC);

	INFO(printf("Thread(create): new thread is %x.%x\n", l4_myself().id.task, (int)new_thread));

	/* if something went wrong, return error code */
	if ((int)new_thread < 0) return -1;

	/* return thread id to out parameter */
	if (dst_tid) dst_tid->tid = l4thread_l4_id(new_thread);

	return 0;
}


/*** KILL THREAD ***/
static void kill_thread(THREAD *tid) {
	l4thread_shutdown(l4thread_id(tid->tid));
}


/*** CREATE NEW MUTEX AND SET IT UNLOCKED ***/
static MUTEX *create_mutex(int init) {
	MUTEX *result;

	result = (MUTEX *)malloc(sizeof(MUTEX));
	if (!result) {
		INFO(printf("Thread(create_mutex): out of memory!\n");)
		return NULL;
	}

	if (init) {
		result->sem = L4SEMAPHORE_LOCKED;
		result->locked_flag = 1;

	} else {
		result->sem = L4SEMAPHORE_UNLOCKED;
		result->locked_flag = 0;
	}
	return (MUTEX *)result;
}


/*** DESTROY MUTEX ***/
static void destroy_mutex(MUTEX *m) {
	if (!m) return;
	free(m);
}


/*** LOCK MUTEX ***/
static void mutex_down(MUTEX *m) {
	if (!m) return;
	l4semaphore_down(&m->sem);
	m->locked_flag = 1;
}


/*** UNLOCK MUTEX ***/
static void mutex_up(MUTEX *m) {
	if (!m) return;
	l4semaphore_up(&m->sem);
	m->locked_flag = 0;
}


/*** TEST IF MUTEX IS LOCKED ***/
static s8 mutex_is_down(MUTEX *m) {
	if (!m) return 0;
	return m->locked_flag;
}


/*** CONVERT IDENTIFIER TO THREAD ID ***/
static int ident2thread(const u8 *ident, THREAD *dst) {
	l4_threadid_t *tid = &dst->tid;
	int i;
	if (!ident || !tid) return -1;

	/* check if identifier string length is valid */
	for (i=0;i<24;i++) {
		if (!ident[i]) return -1;
	}
	tid->lh.low  = hex2u32(ident+7);
	tid->lh.high = hex2u32(ident+16);
	return 0;
}


/*** DETERMINE IF TWO THREADS ARE IDENTICAL ***
 *
 * \return   1 if task is equal, 2 it task and thread id is equal
 */
static int thread_equal(THREAD *t1, THREAD *t2) {
	if (!t1 || !t2) return 0;

	if (t1->tid.id.task == t2->tid.id.task) {
		if (t1->tid.id.lthread == t2->tid.id.lthread) return 2;
		return 1;
	}
	return 0;
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct thread_services services = {
	alloc_thread,
	free_thread,
	copy_thread,
	start_thread,
	kill_thread,
	create_mutex,
	destroy_mutex,
	mutex_down,
	mutex_up,
	mutex_is_down,
	ident2thread,
	thread_equal,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_thread(struct dope_services *d) {
	d->register_module("Thread 1.0", &services);
	return 1;
}
