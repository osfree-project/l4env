/*
 * \brief   presentation manager class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a presentation manager which
 * manages and contains all presentations.
 */

struct private_presentermanager;
#define PRESMANAGER struct private_presentermanager
#define PRESENTER PRESMANAGER

#include <string.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_mem/dm_mem.h>

#include "model/presenter.h"
#include "util/presenter_conf.h"
#include "util/arraylist.h"
#include "util/keygenerator.h"
#include "util/module_names.h"
#include "util/memory.h"
#include "util/hashtab.h"
#include "util/dataprovider.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "model/presmanager.h"

#define PRESMAN_HASHTAB_SIZE 8

extern int _DEBUG;

static struct memory_services *mem;
static struct presenter_general_services *presenter;
static struct slide_services *sld_serv;
static struct presentation_services *pres_serv;
static struct hashtab_services *hashtab;
static struct arraylist_services *arraylist;
static struct keygenerator_services *keygenerator;
static struct dataprovider_services *dataprovider;

PRESMANAGER {
        /* all general methods */
        struct presenter_methods *gen;

        /* all special methods of each slide */
        struct presmanager_methods *pmm;

        /* all general data */
        struct presenter_data *pdat;

	/* private data */
	HASHTAB *presentation_tab;
	
	KEYGENERATOR *key_gen;
	
};

int init_presmanager(struct presenter_services *);

static struct presenter_methods gen_methods;


/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static char * calc_slide_path(char *path, int path_length, char *slide_name) {
	int i,j;
	int name_length = 0;
	char *complete_path=NULL;

	/* calculate length of slide name*/
	name_length = strlen(slide_name);

	/* alloc memory for complete path */
	complete_path = (char *) mem->alloc(path_length+name_length);

	/* copy path name */
	for(i=0;i<path_length;i++) {
		complete_path[i] = path[i];
	}

	/* copy slide_name */
	for(j=0;j<name_length;j++) {
		complete_path[i] = slide_name[j];
		i++;
	}

	return complete_path;
}

/*** calculate the name of the presentation from the configuration file name ***/
static char * calc_pres_name(char *fname) {
	int i,name_length;
	char *new, *name;
	name_length=0;

	/* search for last backslash */
	name = strrchr(fname,47);
	name++;

	/* calc the length of the config file name */
	while(name[name_length] != '.') name_length++;

	new = (char *) mem->alloc(name_length);

	for (i=0;i<name_length;i++) new[i] = name[i];
	new[name_length] = '\0';

	return new;
}


/***************************************/
/***  PRESMANAGER SPECIFIC METHODS   ***/
/**************************************/

static int presmanager_build_presentation (PRESMANAGER *pm, char *fname) {
	int index,length,size,present_key;
	SLIDE *sld;
	ARRAYLIST *al;
	PRESENTATION *p;
	l4dm_dataspace_t *ds;
	char *path_slide, *slide_name, *pres_path, *pres_name;

	length=0;

	if (!pm) return -1;

	/* create presentation */
	p = pres_serv->create();

	/* load config file and parse it */
	al = dataprovider->load_config(fname);

	if (al==NULL) {
		LOG("could not build up presentation");
		return -1;
	}

	/* calculate name of the presentation from name of configuration file */
	pres_name = calc_pres_name(fname);

	arraylist->set_iterator(al);
	size = arraylist->size(al);
	
	/* get directory of slides */
	pres_path = arraylist->get_next(al);

	/* set name of presentation with name of directory */
	p->gen->set_name((PRESENTER *)p,pres_name);

	/* calculate length of the slides directory */
	length = strlen(pres_path);

	for (index=1;index<size;index++) {
		/* get slide name */
		slide_name = arraylist->get_next(al);

		/* build path name for loading slide */
		path_slide = calc_slide_path(pres_path,length,slide_name);

		ds = (l4dm_dataspace_t *) mem->alloc(sizeof(l4dm_dataspace_t));

		/* load slide */
		dataprovider->load_content(path_slide,ds);

		/* create a slide */
		sld = sld_serv->create();

		/* set name and ds in slide */
	 	sld->gen->set_name((PRESENTER *)sld,slide_name);
		sld->sm->set_content((SLIDE *)sld,ds);

		/* add slide in presentation */
        p->presm->add_slide(p,sld);

	}

	/* add presentation to presmanager map */
	pm->pmm->add_presentation(pm,p);

	/* get key of presentation */
	present_key = p->gen->get_key((PRESENTER *)p);

	return present_key; 

}

static void presmanager_add_presentation (PRESMANAGER *pm, PRESENTATION *p) {
	if (!pm) return;

	 /* get a key by keygenerator and set it in presentation */
	 p->gen->set_key((PRESENTER *)p,keygenerator->get_key(pm->key_gen));

        /* add slide in hashtab of the given presentation */
	hashtab->add_elem(pm->presentation_tab,p->gen->get_key((PRESENTER *)p),p);

}

static void presmanager_del_presentation (PRESMANAGER *pm, PRESENTATION *p) {

	/* to implement */

}

static PRESENTATION * presmanager_get_presentation (PRESMANAGER *pm, int key) {
	if (!pm) return NULL;
        return hashtab->get_elem(pm->presentation_tab,key);

}

static struct presmanager_methods pm_methods = {
	presmanager_build_presentation,
        presmanager_add_presentation,
        presmanager_del_presentation,
        presmanager_get_presentation,
};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static PRESMANAGER *create(void) {
	PRESMANAGER *new = (PRESMANAGER *)mem->alloc(sizeof(PRESENTATION)+sizeof(struct presenter_data));
	new->gen	 = &gen_methods;
	new->pmm 	 = &pm_methods;
	
	new->presentation_tab = hashtab->create(PRESMAN_HASHTAB_SIZE);
	new->key_gen	      = keygenerator->create();

	return new;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct presmanager_services services = {
        create,
};

int init_presmanager(struct presenter_services *p) {
        mem = p->get_module(MEMORY_MODULE);
	presenter = p->get_module(PRESENTER_MODULE);
	hashtab = p->get_module(HASHTAB_MODULE);
        arraylist = p->get_module(ARRAYLIST_MODULE);
        keygenerator = p->get_module(KEYGENERATOR_MODULE);
	dataprovider = p->get_module(DATAPROVIDER_MODULE);
	sld_serv     = p->get_module(SLIDE_MODULE);
	pres_serv    = p->get_module(PRESENTATION_MODULE);

        p->register_module(PRESMANAGER_MODULE,&services);

        /* get default methods */
        presenter->default_presenter_methods(&gen_methods);

        return 1;
}


