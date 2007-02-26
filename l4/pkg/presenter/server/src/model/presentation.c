/*
 * \brief   presentation module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements the presentation module which 
 */

struct private_presentation;
#define PRESENTATION struct private_presentation
#define PRESENTER PRESENTATION

#include <l4/dm_mem/dm_mem.h>

#include "model/presenter.h"
#include "util/presenter_conf.h"
#include "util/arraylist.h"
#include "util/module_names.h"
#include "model/slide.h"
#include "model/presentation.h"
#include "util/keygenerator.h"

#define MAX_POSITIONS 128

#define _DEBUG 0

static struct presenter_general_services *presenter;
static struct arraylist_services *arraylist;
static struct keygenerator_services *keygenerator;

PRESENTATION {
    /* all general methods */
    struct presenter_methods *gen;

    /* all special methods of each slide */
    struct presentation_methods *pres;

    /* all general data */
    struct presenter_data *pdat;

    KEYGENERATOR *key_gen;

    ARRAYLIST *slides;

    char *path;

};

int init_presentation(struct presenter_services *);

static struct presenter_methods gen_methods;

/***************************************/
/*** PRESENTATION SPECIFIC METHODS   ***/
/***************************************/

static ARRAYLIST * presentation_get_slides_of_pres(PRESENTATION *p) {
    if (!p) return NULL;

    return p->slides;
}

static void presentation_set_slide_order(PRESENTATION *p,int *positions) {
    if (!p) return;

    /* to implement */
}

static int * presentation_get_slide_order(PRESENTATION *p) {
    if (!p) return NULL;

    /* to implement */

    return NULL;
}

static SLIDE * presentation_get_slide(PRESENTATION *p, int key) {
    SLIDE *sl;
    int i, sldkey;
    if (!p) return NULL; 

    arraylist->set_iterator(p->slides);

    for (i=0;i<arraylist->size(p->slides);i++) {
        sl = (SLIDE *) arraylist->get_next(p->slides);

        sldkey = sl->gen->get_key((PRESENTER *)sl);

        if (sldkey == key) return sl;
    }

    return NULL;
}

static void presentation_add_slide (PRESENTATION *p, SLIDE *sl) {
    int new_key;
    if (!p) return;

    /* generate a new key for slide */
    new_key = keygenerator->get_key(p->key_gen);

    /* get a key by keygenerator and set it in slide */
    sl->gen->set_key((PRESENTER *)sl,new_key);

    /* add slide in arraylist of the given presentation */
    arraylist->add_elem(p->slides,sl);
}

static void presentation_del_slide (PRESENTATION *p, int slide_key) {
    SLIDE *sld;
    int i;
    if (!p) return;
    sld = NULL;

    arraylist->set_iterator(p->slides);

    for (i=0;i<arraylist->size(p->slides);i++) {

        sld = (SLIDE *) arraylist->get_next(p->slides);

        if (sld->gen->get_key((PRESENTER *)sld) == slide_key) {
            arraylist->remove_elem(p->slides,
                       arraylist->get_current_index(p->slides));
            break;
        }
    }

    sld->sm->del_content(sld);

    free(sld);
}

static void presentation_del_all_slides (PRESENTATION *p) {
    int i;
    SLIDE *sld;
    if (!p) return;

    arraylist->set_iterator(p->slides);

    for (i=0;i<arraylist->size(p->slides);i++) {

        sld = (SLIDE *) arraylist->get_next(p->slides);

        sld->sm->del_content(sld);

        free(sld);
    }

    arraylist->destroy(p->slides);
}

static void presentation_set_path(PRESENTATION *p, char *path) {
    if (!p || !path) return;

    p->path = strdup(path);
}

static char * presentation_get_path(PRESENTATION *p) {
    if (!p) return NULL;
    return p->path;
}


static struct presentation_methods pres_methods = {
    presentation_get_slides_of_pres,
    presentation_set_slide_order,
    presentation_get_slide_order,
    presentation_get_slide,
    presentation_add_slide,
    presentation_del_slide,
    presentation_del_all_slides,
    presentation_set_path,
    presentation_get_path,
};

static PRESENTATION *create(void) {
    PRESENTATION *new = (PRESENTATION *) malloc(sizeof(PRESENTATION)
                        +sizeof(struct presenter_data));

    new->pdat = (struct presenter_data *)((long)new + sizeof(PRESENTATION));
    new->gen  = &gen_methods; /* pointer to general methods */
    new->pres = &pres_methods; /* pointer to slide specific methods */

    new->slides = arraylist->create();
    new->key_gen = keygenerator->create(MAX_POSITIONS); 
    new->path = NULL;

    return new;
}

static struct presentation_services services = {
    create,
};

int init_presentation(struct presenter_services *p) {
    presenter = p->get_module(PRESENTER_MODULE);
    arraylist = p->get_module(ARRAYLIST_MODULE);
    keygenerator = p->get_module(KEYGENERATOR_MODULE);

    p->register_module(PRESENTATION_MODULE,&services);

    /* get default methods */
    presenter->default_presenter_methods(&gen_methods);

    return 1;
}
