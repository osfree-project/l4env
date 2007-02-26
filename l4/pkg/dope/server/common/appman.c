/*
 * \brief	DOpE application manager module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is used to manage DOpE-clients.
 * It handles all needed application specific
 * information such as its name, variables etc.
 */


#include "dope-config.h"
#include "memory.h"
#include "hashtab.h"
#include "thread.h"
#include "appman.h"

#define MAX_APPS 64             /* maximum amount of DOpE clients */
#define APP_NAMELEN 64          /* application identifier string length */
#define WID_HASHTAB_SIZE 32     /* applications widget hash table config */
#define WID_HASH_CHARS 5
#define VAR_HASHTAB_SIZE 32     /* applications variable hash table config */
#define VAR_HASH_CHARS 5

static struct memory_services  *mem;
static struct hashtab_services *hashtab;

struct app_struct {
	char name[APP_NAMELEN];     /* identifier string of the application */
	THREAD  *app_thread;        /* application thread */
	HASHTAB *wids;              /* hashtable containing widget symbols */
	HASHTAB *vars;              /* hashtable containing variable symbols */
	THREAD 	*listener;          /* associated result/event listener thread */
};

static struct app_struct *apps[MAX_APPS];

int init_appman(struct dope_services *d);



/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

/*** RETURN THE FIRST FREE APPLICATION ID ***/
static s32 get_free_app_id(void) {
	u32 i;
	
	for (i=0;i<MAX_APPS;i++) {
		if (!apps[i]) return i;
	}
	DOPEDEBUG(printf("AppMan(get_free_app_id): no free dope application id!\n");)
	return -1;
}


/*** CREATE NEW APPLICATION ***/
static struct app_struct *new_app(void) {
	struct app_struct *new;
	
	/* get memory for application structure */
	new = (struct app_struct *)mem->alloc(sizeof(struct app_struct));
	if (!new) {
		DOPEDEBUG(printf("AppMan(new_app): out of memory!\n");)	
		return NULL;
	}
	
	/* create hash table to store the widgets of the application */
	new->wids = hashtab->create(WID_HASHTAB_SIZE,WID_HASH_CHARS);
	if (!new->wids) {
		DOPEDEBUG(printf("AppMan(new_app): failed during creating of widget hashtab\n");)
		mem->free(new);
		return NULL;
	}
	
	/* create hash table to store the variables of the application */
	new->vars = hashtab->create(VAR_HASHTAB_SIZE,VAR_HASH_CHARS);
	if (!new->vars) {
		DOPEDEBUG(printf("AppMan(new_app): failed during creating of variable hashtab\n");)
		hashtab->destroy(new->wids);
		mem->free(new);
		return NULL;
	}
	
	new->listener = NULL;
	new->app_thread = NULL;
	return new;
}


/*** SET THE IDENTIFIER STRING OF AN APPLICATION ***/
static void set_app_name(struct app_struct *app,char *appname) {
	u32 i;
	
	if (!app || !appname) return;

	/* copy application name into application data structure */
	for (i=0;i<APP_NAMELEN;i++) {
		app->name[i]=appname[i];
		if (!appname[i]) break;
	}
	app->name[APP_NAMELEN-1]=0;
}


/*** TEST IF AN APP_ID IS A VALID ONE ***/
static u16 app_id_valid(u32 app_id) {

	if (app_id>=MAX_APPS) {
		DOPEDEBUG(printf("AppMan(app_id_value): invalid app_id (out of range)\n");)
		return 0;
	}
	if (!apps[app_id]) {
		DOPEDEBUG(printf("AppMan(unregister): invalid app_id (no application with this id)\n");)
		return 0;	
	}
	return 1;
}


/*** RETURN NAME OF APPLICATION WITH THE SPECIFIED ID ***/
static char *get_app_name(s32 app_id) {
	struct app_struct *app;
	if (!app_id_valid(app_id)) return "";
	app = apps[app_id];
	return app->name;
}



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** REGISTER NEW APPLICATION ***/
/* app_name: identifier string for the application */
/* returns dope_client id */
static s32 register_app(char *app_name) {
	s32 newapp_id;

	newapp_id = get_free_app_id();
	if (newapp_id<0) {
		DOPEDEBUG(printf("AppMan(register): application registering failed (no free app id)\n");)
		return -1;
	}
	
	/* create data structures for the DOpE internal representation of the app */
	apps[newapp_id]=new_app();
	if (!apps[newapp_id]) {
		DOPEDEBUG(printf("AppMan(register): application registering failed.\n");)
		return -1;	
	}
	
	set_app_name(apps[newapp_id],app_name);
	return newapp_id;
}


/*** UNREGISTER AN APPLICATION AND FREE ALL ASSOCIATED INTERNAL RESSOURCES ***/
static s32 unregister_app(u32 app_id) {

	if (!app_id_valid(app_id)) return -1;
	
	/* destroy the with the application associated hash tables */
	hashtab->destroy(apps[app_id]->wids);
	hashtab->destroy(apps[app_id]->vars);
	
	/* free the memory for the internal application representation */
	mem->free(apps[app_id]);
	
	/* mark the corresponding app_id to be free */
	apps[app_id]=NULL;
	return 0;
}


/*** REQUEST WIDGET HASH TABLE OF AN APPLICATION ***/
static HASHTAB *get_widgets(u32 app_id) {

	if (!app_id_valid(app_id)) {
		DOPEDEBUG(printf("ApplicationManager(get_widgets): invalid app_id\n"));
		return NULL;
	}
	return apps[app_id]->wids;
}


/*** REQUEST VARIABLE HASH TABLE OF AN APPLICATION ***/
static HASHTAB *get_variables(u32 app_id) {

	if (!app_id_valid(app_id)) return NULL;
	return apps[app_id]->vars;
}


/*** REGISTER EVENT LISTENER THREAD OF AN APPLICATION ***/
static void reg_listener(s32 app_id, THREAD *listener) {
	if (!app_id_valid(app_id)) return;
	apps[app_id]->listener = listener;
}


/*** REQUEST EVENT LISTENER THREAD OF AN APPLICATION ***/
static THREAD *get_listener(s32 app_id) {
	if (!app_id_valid(app_id)) return NULL;
	return apps[app_id]->listener;
}


/*** REGISTER APPLICATION THREAD ***/
static void reg_app_thread(s32 app_id, THREAD *app_thread) {
	if (!app_id_valid(app_id)) return;
	apps[app_id]->app_thread = app_thread;
}


/*** REQUEST APPLICATION THREAD ***/
static THREAD *get_app_thread(s32 app_id) {
	if (!app_id_valid(app_id)) return NULL;
	return apps[app_id]->app_thread;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct appman_services services = {
	register_app,
	unregister_app,
	get_widgets,
	get_variables,
	reg_listener,
	get_listener,
	get_app_name,
	reg_app_thread,
	get_app_thread,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_appman(struct dope_services *d) {
	u32 i;
	
	for (i=0;i<MAX_APPS;i++) apps[i]=NULL;
	
	mem 	= d->get_module("Memory 1.0");
	hashtab = d->get_module("HashTable 1.0");
	
	d->register_module("ApplicationManager 1.0",&services);
	return 1;
}
