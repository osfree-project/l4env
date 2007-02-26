/*
 * \brief   DOpE client library - internally used synchronisation primitives
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <string.h>

/*** L4 INCLUDES ***/
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>

/*** LOCAL INCLUDES ***/
#include "sync.h"

struct dopelib_sem   { l4semaphore_t sem;  };
struct dopelib_mutex { l4lock_t      lock; };


struct dopelib_sem *dopelib_sem_create(int init_state) {
	struct dopelib_sem *new_sem = malloc(sizeof(struct dopelib_sem));
	if (!new_sem) return NULL;
	memset(new_sem, 0, sizeof(struct dopelib_sem));

	new_sem->sem = init_state ? L4SEMAPHORE_LOCKED : L4SEMAPHORE_UNLOCKED;
	return new_sem;
}


void dopelib_sem_destroy(struct dopelib_sem *sem) {
	if (sem) free(sem);
}


void dopelib_sem_wait(struct dopelib_sem *sem) {
	if (sem) l4semaphore_down(&sem->sem);
}


void dopelib_sem_post(struct dopelib_sem *sem) {
	if (!sem) return;
	l4semaphore_up(&sem->sem);
}


struct dopelib_mutex *dopelib_mutex_create(int init_state) {
	struct dopelib_mutex *new_mutex = malloc(sizeof(struct dopelib_mutex));
	if (!new_mutex) return NULL;
	memset(new_mutex, 0, sizeof(struct dopelib_mutex));

	new_mutex->lock = L4LOCK_UNLOCKED;
	if (init_state) dopelib_mutex_lock(new_mutex);

	return new_mutex;
}


void dopelib_mutex_destroy(struct dopelib_mutex *m) {
	if (m) free(m);
}


void dopelib_mutex_lock(struct dopelib_mutex *m) {
	if (m) l4lock_lock(&m->lock);
}


void dopelib_mutex_unlock(struct dopelib_mutex *m) {
	if (m) l4lock_unlock(&m->lock);
}

