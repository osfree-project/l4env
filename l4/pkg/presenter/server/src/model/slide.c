/*
 * \brief   slide class module
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements a slide with its data.
 */

struct private_slide;
#define SLIDE struct private_slide
#define PRESENTER SLIDE

#include <errno.h>

#include <l4/env/errno.h>
#include <l4/dm_mem/dm_mem.h>

#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "model/slide.h"
#include "util/module_names.h"

#define SLIDE_STD_NAME "slide"

extern l4_threadid_t presenter_thread_id;

static struct presenter_general_services *presenter;

SLIDE {
    /* all general methods */
    struct presenter_methods *gen;

    /* all special methods of each slide */
    struct slide_methods *sld;

    /* all general data */
    struct presenter_data *pdat;

    /* now the private data */
    char *content;

    l4dm_dataspace_t *ds;
};

int init_slide(struct presenter_services *);

static struct presenter_methods gen_methods;

/********************************/
/*** SLIDE SPECIFIC METHODS   ***/
/********************************/

static int slide_set_content (SLIDE *sl, l4dm_dataspace_t *ds_old) {
    l4_int32_t err;
    l4_size_t ds_size;
    l4_addr_t content_addr;

    if (!sl || !ds_old) return -1;

    err = l4dm_copy(ds_old,0,SLIDE_STD_NAME,sl->ds);

    if (err) {
        LOG("Error (%d) on copy dataspace...", err);
        return err;
    }

    l4dm_mem_size(sl->ds,&ds_size);

    err=l4rm_attach(sl->ds,ds_size,0,L4DM_RW,(void *)&content_addr);

    if (err) {
        LOG("Error (%d) attaching dataspace...", err);
        l4dm_close(ds_old);
        return err;
    }

    sl->content = (char *) content_addr;

    err = l4dm_close(ds_old);

    if (err != 0) {
        LOG("Error (%d) closing dataspace...", err);
        return err;
    }

    return 0;
}

static char * slide_get_content (SLIDE *sl) {
    if (!sl) return NULL;
    return sl->content;
}

static void slide_del_content (SLIDE *sl) {
    l4_int32_t error;

    if (sl->content)
    {
        l4rm_detach(sl->content);
        error = l4dm_close(sl->ds);

        if (error != 0) {
            LOG("Error (%d) closing dataspace...", error);
            return;
        }
    }

    free(sl->ds);
}

static l4dm_dataspace_t * slide_get_ds(SLIDE *sl) {
    return sl->ds;
}

static int slide_get_content_size(SLIDE *sl) {
    l4_size_t ds_size;

    l4dm_mem_size(sl->ds,&ds_size);

    return (int) ds_size;
}

static struct slide_methods sld_methods = {
    slide_set_content,
    slide_get_content,
    slide_del_content,
    slide_get_ds,
    slide_get_content_size,
};

static SLIDE *create(void) {

    SLIDE *new = (SLIDE *)malloc(sizeof(SLIDE)+sizeof(struct presenter_data));

    new->pdat = (struct presenter_data *)((long)new + sizeof(SLIDE));

    presenter->default_presenter_methods(&gen_methods);

    new->gen = &gen_methods; /* pointer to general methods */
    new->sld = &sld_methods; /* pointer to slide specific methods */
    new->content  = NULL;
    new->ds = (l4dm_dataspace_t *) malloc(sizeof(l4dm_dataspace_t));

    return new;
}


static struct slide_services services = {
    create,
};

int init_slide(struct presenter_services *p) {

    presenter = p->get_module(PRESENTER_MODULE);

    p->register_module(SLIDE_MODULE,&services); 

    return 1;
}
