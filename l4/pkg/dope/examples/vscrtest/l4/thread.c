/*
 * \brief   L4 specific thread handling
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

/*** L4 INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>

/*** LOCAL INCLUDES ***/
#include "thread.h"

void thread_create(void (*entry)(void *),void *arg) {
	l4thread_create(entry,arg,L4THREAD_CREATE_ASYNC);
}


void usleep(long us) {
	l4_usleep(us);
}
