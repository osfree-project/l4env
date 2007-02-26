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
#include "misc.h"


/*** UTILITY: CONVERT CALLBACK FUNCTION TO BIND ARGUMENT STRING ***/
char *dopelib_callback_to_bindarg(void (*callback)(dope_event *,void *),
                                  void *arg,
                                  char *dst_buf, int dst_len) {

	snprintf(dst_buf, dst_len, "#! %lx %lx", (long)callback, (long)arg);
	return dst_buf;
}


/*** INTERFACE: BIND AN EVENT TO A DOpE WIDGET ***/
void dope_bind(long id, const char *var, const char *event_type,
               void (*callback)(dope_event *,void *), void *arg) {
	char cmdbuf[256];
	char bindbuf[64];

	dopelib_mutex_lock(dopelib_cmdf_mutex);

	dopelib_callback_to_bindarg(callback, arg, bindbuf, sizeof(bindbuf));
	snprintf(cmdbuf, sizeof(cmdbuf), "%s.bind(\"%s\",\"%s\")",
	         var, event_type, bindbuf);

	dope_cmd(id,cmdbuf);
	dopelib_mutex_unlock(dopelib_cmdf_mutex);
}


/*** INTERFACE: BIND AN EVENT TO A DOpE WIDGET SPECIFIED AS FORMAT STRING ***/
void dope_bindf(long id, const char *varfmt, const char *event_type,
                void (*callback)(dope_event *,void *), void *arg,...) {
	char varstr[512];
	va_list list;

	/* decode varargs */
	va_start(list, arg);
	vsnprintf(varstr, sizeof(varstr), varfmt, list);
	va_end(list);

	dope_bind(id, varstr, event_type, callback, arg);
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
