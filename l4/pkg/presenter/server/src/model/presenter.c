/*
 * \brief   presenter base class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements common functionality
 * of all presenter modules.
 */

struct private_presenter;
#define PRESENTER struct private_presenter

#include "model/presenter.h"
#include "util/presenter_conf.h"
#include "util/module_names.h"
#include "util/memory.h"

#define _DEBUG 0

static struct memory_services *mem;

PRESENTER {
	struct presenter_methods *gen;	 
	void   *presenter_specific_methods;
	struct presenter_data *pdata;
};

int init_presenter(struct presenter_services *p);

/**********************************************************/
/*** DEFAULT IMPLEMENTATIONS OF PRESENTER CLASS METHODS ***/
/**********************************************************/

static void pres_set_key(PRESENTER *p, int key) {
	p->pdata->key=key;
	LOGd(_DEBUG,"%d",p->pdata->key);
}

static int pres_get_key(PRESENTER *p) {
	return p->pdata->key;
}

static void pres_set_name(PRESENTER *p, char *name) {
	p->pdata->name=name;
}

static char *pres_get_name(PRESENTER *p) {
	return p->pdata->name;
}

/*** SET GENERAL PRESENTER METHODS TO DEFAULT METHODS ***/
static void default_presenter_methods(struct presenter_methods *m) {
	m->set_key	=	pres_set_key;	
	m->get_key	= 	pres_get_key;
	m->set_name	=	pres_set_name;
	m->get_name	=	pres_get_name;
}

static void default_presenter_data(struct presenter_data *d) {
	d->key=0;
	d->name=NULL;
}

static struct presenter_general_services services = {
	default_presenter_methods,
	default_presenter_data,
};

int init_presenter(struct presenter_services *p) {
	mem = p->get_module(MEMORY_MODULE);

	p->register_module(PRESENTER_MODULE,&services);

	return 1;
}
