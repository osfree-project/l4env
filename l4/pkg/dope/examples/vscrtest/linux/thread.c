/*
 * \brief   Linux specific thread handling
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
#include <pthread.h>

/*** LOCAL INCLUDES ***/
#include "thread.h"

void thread_create(void (*entry)(void *),void *arg) {
	pthread_t* tid = malloc(sizeof(pthread_t));
	pthread_create(tid, NULL, (void *(*)(void *))entry, arg);
}

