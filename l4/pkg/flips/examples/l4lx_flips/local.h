/**
 * \file   flips/examples/l4lx_flips/local.h
 * \brief  Internal interfaces
 *
 * \date   02/03/2006
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __FLIPS_EXAMPLES_L4LX_FLIPS_LOCAL_H_
#define __FLIPS_EXAMPLES_L4LX_FLIPS_LOCAL_H_

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/types.h>

/***************************/
/*** SELECT NOTIFICATONS ***/
/***************************/

#include <sys/select.h>

struct notify_fd_set {
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	l4_threadid_t notifier;
};

void notify_init_fd_set(struct notify_fd_set *fds);
void notify_request(struct notify_fd_set *fds, int fd, int mode, const l4_threadid_t notifier);
void notify_clear(struct notify_fd_set *fds, int fd, int mode, const l4_threadid_t notifier);
void notify_select_thread(struct notify_fd_set *fds);

/***********************/
/*** THREAD HANDLING ***/
/***********************/

#include <l4/util/sll.h>
#include <semaphore.h>

/* session thread and client information */
struct session_thread_info {
	sem_t started;                   /* startup synchronization */
	pthread_t session_thread;        /* pthread ID of session thread */
	pthread_t select_thread;         /* pthread ID of select worker */
	struct notify_fd_set select_fds; /* file descriptor set for select */
	l4_threadid_t session_l4thread;  /* L4 thread ID of session thread */
	l4_threadid_t select_l4thread;   /* L4 thread ID of select thread */

	/* exit event handling */
	l4_taskid_t partner;             /* client task */
	slist_t *elem;                   /* session list element */
};

/**********************
 *** EVENTS SUPPORT ***
 **********************/

#include <pthread.h>

void init_events(slist_t **sl, pthread_mutex_t *l);

/*****************/
/*** DEBUGGING ***/
/*****************/

#include <stdio.h>

#define D(args...)                        \
	do {                                  \
		fprintf(stderr, "%s: ", LOG_tag); \
		fprintf(stderr, args);            \
		fprintf(stderr, "\n");            \
	} while (0)

#endif
