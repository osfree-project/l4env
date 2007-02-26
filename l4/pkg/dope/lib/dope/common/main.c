/*
 * \brief   DOpE client library
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

/*** GENERIC INCLUDES ***/
#include <stdio.h>
#include <stdarg.h>

/*** DOPE INCLUDES ***/
#include <dopelib.h>
#include "dopestd.h"

/*** LOCAL INCLUDES ***/
#include "listener.h"
#include "sync.h"
#include "events.h"
#include "init.h"


/*** INTERFACE: BIND AN EVENT TO A DOpE WIDGET ***/
void dope_bind(long id, const char *var, const char *event_type,
               void (*callback)(dope_event *,void *), void *arg) {
	static char cmdbuf[257];

	dopelib_mutex_lock(dopelib_cmdf_mutex);
	snprintf(cmdbuf, 256, "%s.bind(\"%s\",\"#! %lx %lx\")",
	         var, event_type, (u32)callback, (u32)arg);
	dope_cmd(id,cmdbuf);
	dopelib_mutex_unlock(dopelib_cmdf_mutex);
}


/*** INTERFACE: BIND AN EVENT TO A DOpE WIDGET SPECIFIED AS FORMAT STRING ***/
void dope_bindf(long id, const char *varfmt, const char *event_type,
                void (*callback)(dope_event *,void *), void *arg,...) {
	static char cmdbuf[257];
	static char varstr[1024];
	va_list list;

	dopelib_mutex_lock(dopelib_cmdf_mutex);
	va_start(list, arg);
	vsnprintf(varstr, 1024, varfmt, list);
	va_end(list);

	snprintf(cmdbuf, 256, "%s.bind(\"%s\",\"#! %lx %lx\")",
	 varstr,event_type, (u32)callback, (u32)arg);
	dope_cmd(id, cmdbuf);
	dopelib_mutex_unlock(dopelib_cmdf_mutex);
}


/*** INTERFACE: PROCESS SINGLE DOpE EVENT ***/
void dope_process_event(long id) {
	dope_event *e;
	char *bindarg, *s;
	u32 num1 = 0, num2 = 0;

	dopelib_wait_event(id, &e, &bindarg);

	/* test if cookie is valid */
	if ((bindarg[0] != '#') || (bindarg[1] != '!')) return;

	/* determine callback adress and callback argument */
	s = bindarg + 3;
	while (*s == ' ') s++;
	while ((*s != 0) && (*s != ' ')) {
		num1 = (num1 << 4) + (*s & 15);
		if (*(s++) > '9') num1 += 9;
	}
	if (*(s++) == 0) return;
	while ((*s != 0) && (*s != ' ')) {
		num2 = (num2 << 4) + (*s&15);
		if (*(s++) > '9') num2 += 9;
	}

	/* call callback routine */
	if (num1) ((void (*)(dope_event *, void *))num1)(e, (void *)num2);

}


/*** INTERFACE: HANDLE EVENTLOOP OF A DOpE CLIENT ***/
void dope_eventloop(long id) {
	while (1) dope_process_event(id);
}


/*** INTERFACE: DISCONNECT FROM DOpE ***
 *
 * FIXME: wait for the finishing of the current command and block net commands
 */
void dope_deinit(void) {}


void *CORBA_alloc(unsigned long size);
void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}
