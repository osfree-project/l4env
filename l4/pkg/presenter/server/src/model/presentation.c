/*
 * \brief   presentation module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements the presentation module which 
 * manages and contains all slides of it. 
 */

struct private_presentation;
#define PRESENTATION struct private_presentation
#define PRESENTER PRESENTATION

#include <l4/dm_phys/dm_phys.h>

#include "model/presenter.h"
#include "util/presenter_conf.h"
#include "util/arraylist.h"
#include "util/module_names.h"
#include "util/memory.h"
#include "util/hashtab.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "util/keygenerator.h"

#define PRES_HASHTAB_SIZE 128     /* presentation slides hash table config */
#define MAX_POSITIONS 128

extern int _DEBUG;

static struct presenter_general_services *presenter;
static struct memory_services *mem;
static struct hashtab_services *hashtab;
static struct arraylist_services *arraylist;
static struct keygenerator_services *keygenerator;

PRESENTATION {
        /* all general methods */
        struct presenter_methods *gen;

        /* all special methods of each slide */
        struct presentation_methods *pres;

        /* all general data */
        struct presenter_data *pdat;

	/* now all private data*/
	HASHTAB *slides_tab;	

	KEYGENERATOR *key_gen;

	int positions[MAX_POSITIONS];

	int current_amount;

};

int init_presentation(struct presenter_services *);

static struct presenter_methods gen_methods;

/***************************************/
/*** PRESENTATION SPECIFIC METHODS   ***/
/***************************************/

static ARRAYLIST * presentation_get_slides_of_pres(PRESENTATION *p) {
	int i;
	SLIDE *sld;
	ARRAYLIST *al = arraylist->create();

	for (i=0;i<MAX_POSITIONS;i++) {
		if (p->positions[i]==0) break;
		sld = (SLIDE *) hashtab->get_elem(p->slides_tab,p->positions[i]);
		arraylist->add_elem(al,sld);	
	}

	return al;
}

static void presentation_set_slide_order(PRESENTATION *p,int *positions) {
	int i;
	if (!p) return;
	for (i=0;i<MAX_POSITIONS;i++) {
                if (positions[i]==0) break;
		p->positions[i]=positions[i];
	}
}

static int * presentation_get_slide_order(PRESENTATION *p) {
	if (!p) return NULL;
	return p->positions;
}

static SLIDE * presentation_get_slide(PRESENTATION *p, int key) {
	if (!p) return NULL; 
	return hashtab->get_elem(p->slides_tab,key);	
}

static void presentation_add_slide (PRESENTATION *p, SLIDE *sl) {
	if (!p) return;

	/* get a key by keygenerator and set it in slide */
	sl->gen->set_key((PRESENTER *)sl,keygenerator->get_key(p->key_gen));

	/* add slide in hashtab of the given presentation */
	hashtab->add_elem(p->slides_tab,sl->gen->get_key((PRESENTER *)sl),sl);
	
	/* add key of slide to position list */
	p->positions[p->current_amount] = sl->gen->get_key((PRESENTER *)sl);
	p->current_amount++;
	
}

static void presentation_del_slide (PRESENTATION *p, SLIDE *sl) {
	if (!p || !sl) return;
        hashtab->remove_elem(p->slides_tab,sl->gen->get_key((PRESENTER *)sl));

	/* to implement reorganiziation of positions */
}

static struct presentation_methods pres_methods = {
        presentation_get_slides_of_pres,
        presentation_set_slide_order,
	presentation_get_slide_order,
	presentation_get_slide,
	presentation_add_slide,
	presentation_del_slide,
};

static PRESENTATION *create(void) {
	PRESENTATION *new = 
	(PRESENTATION *)mem->alloc(sizeof(PRESENTATION)+sizeof(struct presenter_data));

        new->pdat = (struct presenter_data *)((long)new + sizeof(PRESENTATION));
	new->gen  = &gen_methods; /* pointer to general methods */
        new->pres = &pres_methods; /* pointer to slide specific methods */

	new->slides_tab = hashtab->create(PRES_HASHTAB_SIZE);
	new->key_gen    = keygenerator->create(); 
	
	new->current_amount = 0;	

	return new;
}

static struct presentation_services services = {
        create,
};

int init_presentation(struct presenter_services *p) {
        mem = p->get_module(MEMORY_MODULE);
        presenter = p->get_module(PRESENTER_MODULE);
	hashtab = p->get_module(HASHTAB_MODULE);
	arraylist = p->get_module(ARRAYLIST_MODULE);
	keygenerator = p->get_module(KEYGENERATOR_MODULE);

        p->register_module(PRESENTATION_MODULE,&services);

	/* get default methods */
        presenter->default_presenter_methods(&gen_methods);

        return 1;
}


