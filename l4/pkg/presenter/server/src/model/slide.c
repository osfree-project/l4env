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

#include <l4/dm_phys/dm_phys.h>

#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "model/slide.h"
#include "util/module_names.h"
#include "util/memory.h"

#include <l4/env/errno.h>

#define _DEBUG 0

static struct memory_services *mem;
static struct presenter_general_services *presenter;

SLIDE {
	/* all general methods */
	struct presenter_methods *gen;

	/* all special methods of each slide */
	struct slide_methods *sld;

	/* all general data */
	struct presenter_data *pdat;

	/* now the private data */
	l4dm_dataspace_t *content;
	l4_addr_t	content_addr;
	l4_size_t	ds_size;
};

int init_slide(struct presenter_services *);

static struct presenter_methods gen_methods;

/********************************/
/*** SLIDE SPECIFIC METHODS   ***/
/********************************/

static void slide_set_content (SLIDE *sl, l4dm_dataspace_t *ds) {
	l4_int32_t error;
	l4_size_t ds_size;

	if (!sl) return;

	l4dm_mem_size(ds,&ds_size);

	error=l4rm_attach(ds,ds_size,0,L4DM_RW,(void *)&sl->content_addr);

        if (error != 0) {
                LOG("error attaching dataspace...");
                LOG("error: %d",error);
                if (error==-L4_EUSED) LOG("L4_EUSED");
                return;
        }  
	else {
                LOGd(_DEBUG,"attached ds at dezimal address %d",sl->content_addr);
        } 

	sl->content = ds;
	sl->ds_size = ds_size;
}

static l4dm_dataspace_t * slide_get_content (SLIDE *sl) {
	if (!sl) return NULL; 
	return sl->content;
}

static l4_addr_t slide_get_content_addr (SLIDE *sl) {
	if (!sl) return 0;
	return sl->content_addr;
}	

static struct slide_methods sld_methods = {
        slide_set_content,
        slide_get_content,
	slide_get_content_addr,
};

static SLIDE *create(void) {

	SLIDE *new = (SLIDE *)mem->alloc(sizeof(SLIDE)+sizeof(struct presenter_data));

	new->pdat = (struct presenter_data *)((long)new + sizeof(SLIDE));	

	presenter->default_presenter_methods(&gen_methods);

	new->gen = &gen_methods; /* pointer to general methods */
	new->sld = &sld_methods; /* pointer to slide specific methods */
	new->content  = NULL;
	new->content_addr = -1;

	return new;

}


static struct slide_services services = {
	create,
};

int init_slide(struct presenter_services *p) {
	mem = p->get_module(MEMORY_MODULE);
	presenter = p->get_module(PRESENTER_MODULE);

	p->register_module(SLIDE_MODULE,&services);	

	return 1;
}
