/*
 * \brief   DOpE client library - DICE specific functions
 * \date    2003-02-28
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

#include "dopestd.h"
#include "dopelib.h"
#include <dope-client.h>
#include "app_struct.h"
#include "sync.h"
#include "listener.h"

#include <stdio.h>
#include <stdarg.h>

extern CORBA_Object dope_server;
extern struct dopelib_mutex *dopelib_cmdf_mutex;
extern struct dopelib_mutex *dopelib_cmd_mutex;

struct dopelib_app *dopelib_apps[MAX_DOPE_CLIENTS];

static struct dopelib_app first_app;

long dope_init_app(char *app_name) {
	int id;
	struct dopelib_app *app;
	char *listener_ident;
	
	for (id=0; (id<MAX_DOPE_CLIENTS) && (dopelib_apps[id]); id++);
	if (id >= MAX_DOPE_CLIENTS) {
		ERROR(printf("DOpElib(init_app): error: no free id found\n"));
		return -1;
	}

	/* use static struct for first application to avoid memory allocation in most cases */
	if (id == 0) {
		app = &first_app;
	} else {
		app = (struct dopelib_app *)malloc(sizeof(struct dopelib_app));
		if (!app) {
			ERROR(printf("DOpElib(init_app): error: out of memory\n"));
			return -1;
		}
		memset(app, 0, sizeof(struct dopelib_app));
	}
	dopelib_apps[id] = app;
	
	listener_ident = dopelib_start_listener(id);
	INFO(printf("DOpElib(dope_init_app): app_name = %s, listener = %s\n",app_name,listener_ident);)
	
	app->app_id = dope_manager_init_app_call(dope_server,app_name,listener_ident,&app->env);
	return id;
}


/*** INTERFACE: EXEC DOPE COMMAND AND REQUEST RESULT ***
 *
 * \param id       virtual DOpE application id
 * \param res      destination buffer for storing the result
 * \param res_max  maximum length of result
 * \param command  DOpE command to execute
 */
int dope_req(long id, char *res, int res_max, char *cmd) {
	struct dopelib_app *app = dopelib_apps[id];
	if (!app || !cmd || !dope_server) return -1;
	return dope_manager_exec_req_call(dope_server, app->app_id,
	                                  cmd, res, &res_max, &app->env);
}


/*** INTERFACE: EXEC DOpE FORMAT STRING COMMAND AND REQUEST RESULT ***/
int dope_reqf(long app_id, char *res, int res_max, char *format, ...) {
	int ret;
	va_list list;
	static char cmdstr[1024];
	
	dopelib_lock_mutex(dopelib_cmdf_mutex);
	va_start(list, format);
	vsnprintf(cmdstr, 1024, format, list);
	va_end(list);
	ret = dope_req(app_id, res, res_max, cmdstr);
	dopelib_unlock_mutex(dopelib_cmdf_mutex);
	
	return ret;
}


/*** INTERFACE: SHORTCUT EXEC A DOPE COMMAND WITHOUT RECEIVING THE RESULT ***
 *
 * \param id       virtual DOpE application id
 * \param command  DOpE command to execute
 * \return         0 on success
 *
 * This function actually receives the result from the DOpE server
 * but does not provide it to the caller.
 */
int dope_cmd(long id, char *cmd) {
	struct dopelib_app *app = dopelib_apps[id];
	return dope_manager_exec_cmd_call(dope_server, app->app_id, cmd, &app->env);
}


/*** INTERFACE: EXEC DOpE FORMAT STRING COMMAND ***/
int dope_cmdf(long app_id, char *format, ...) {
	int ret;
	va_list list;
	static char cmdstr[1024];
	
	dopelib_lock_mutex(dopelib_cmdf_mutex);
	va_start(list, format);
	vsnprintf(cmdstr, 1024, format, list);
	va_end(list);
	ret = dope_cmd(app_id, cmdstr);
	dopelib_unlock_mutex(dopelib_cmdf_mutex);
	
	return ret;
}


long dope_get_keystate(long id, long keycode) {
	return dope_manager_get_keystate_call(dope_server,keycode,&dopelib_apps[id]->env);
}


char dope_get_ascii(long id, long keycode) {
	return dope_manager_get_ascii_call(dope_server,keycode,&dopelib_apps[id]->env);
}

