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
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#define CORBA_Object l4_threadid_t *
#include "thread.h"


void thread_create(void (*entry)(void *),void *arg) {
	l4thread_create(entry,arg,L4THREAD_CREATE_ASYNC);
}

void thread2ident(l4_threadid_t *tid,char *dst) {
	sprintf(dst,"t_id=0x%08X,%08X", (int)tid->lh.low, (int)tid->lh.high);
}

