#include <dope-client.h>
#include "dopelib-config.h"
#include "dopelib.h"
#include "app_struct.h"
#include "sync.h"
#include "listener.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern CORBA_Object dope_server;
extern struct dopelib_mutex *dopelib_cmdf_mutex;
extern struct dopelib_mutex *dopelib_cmd_mutex;

struct dopelib_app_struct *dopelib_apps[MAX_DOPE_CLIENTS];

static struct dopelib_app_struct first_app;

long dope_init_app(char *app_name) {
	int id;
	struct dopelib_app_struct *app;
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
		app = (struct dopelib_app_struct *)malloc(sizeof(struct dopelib_app_struct));
		if (!app) {
			ERROR(printf("DOpElib(init_app): error: out of memory\n"));
			return -1;
		}
		memset(app, 0, sizeof(struct dopelib_app_struct));
	}
	dopelib_apps[id] = app;
	
	listener_ident = dopelib_start_listener(id);
	INFO(printf("DOpElib(dope_init_app): app_name = %s, listener = %s\n",app_name,listener_ident);)
	
	app->app_id = dope_manager_init_app_call(&dope_server,app_name,listener_ident,&app->env);
	return id;
}


char *dope_cmd(long id,char *command) {
	static char resbuf[256];
	static char *res=resbuf;
	struct dopelib_app_struct *app = dopelib_apps[id];

	dopelib_lock_mutex(dopelib_cmd_mutex);
//	INFO(printf("DOpElib(dope_cmd): executing command '%s' ...\n",command);)
	dope_manager_exec_cmd_call(&dope_server,app->app_id,command,&res,&app->env);
	dopelib_unlock_mutex(dopelib_cmd_mutex);

//	INFO(printf("DOpElib(dope_cmd): command executed. ret=%s\n",res);)
	return res;
}


char *dope_cmdf(long app_id, char *format, ...) {
	char *ret;
	va_list list;
	static char cmdstr[1024];
	
	dopelib_lock_mutex(dopelib_cmdf_mutex);
	va_start(list, format);
	vsnprintf(cmdstr, 1024, format, list);
	va_end(args);
	ret = dope_cmd(app_id, cmdstr);
	dopelib_unlock_mutex(dopelib_cmdf_mutex);
	
	return ret;
}


long dope_get_keystate(long id, long keycode) {
	return dope_manager_get_keystate_call(&dope_server,keycode,&dopelib_apps[id]->env);
}


char dope_get_ascii(long id, long keycode) {
	return dope_manager_get_ascii_call(&dope_server,keycode,&dopelib_apps[id]->env);
}

