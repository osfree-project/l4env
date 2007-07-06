/*
 * \brief   Linux specific thread and sleep functions
 * \date    2004-01-11
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
#include <unistd.h>
#include <pthread.h>

/*** LOCAL INCLUDES ***/
#include "thread.h"


/*** START L4 THREAD ***/
void minitop_thread_create(void (*entry)(void *),void *arg) {
	pthread_t* tid = malloc(sizeof(pthread_t));
	pthread_create(tid, NULL, (void *(*)(void *))entry, arg);
}


/*** WAIT SPECIFIED NUMBER OF MICROSECONDS ***/
void minitop_usleep(unsigned long usec) {
	usleep(usec);
}
