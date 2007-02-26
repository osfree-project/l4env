/*
 * \brief   OverlayWM - thread abstraction
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <pthread.h>

/*** LOCAL INCLUDES ***/
#define CORBA_Object void *   /* this is not nice but who cares... */
#include "thread.h"

void thread_create(void (*entry)(void *),void *arg) {
	pthread_t* tid = malloc(sizeof(pthread_t));
	pthread_create(tid, NULL, (void *(*)(void *))entry, arg);
}

void thread2ident(void *thread,char *dst) {
}
