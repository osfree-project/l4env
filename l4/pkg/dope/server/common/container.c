/*
 * \brief   DOpE Container widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Container widgets are layout widgets that allow
 * the placement of subwidgets by specifying pixel.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct container;
#define WIDGET struct container

#include "dopestd.h"
#include "widget_data.h"
#include "widget.h"
#include "gfx.h"
#include "container.h"
#include "widman.h"

static struct widman_services *widman;
static struct gfx_services    *gfx;


struct container_data {
	WIDGET      *first_elem;
	WIDGET      *last_elem;
};

int init_container(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void cont_draw(CONTAINER *c,struct gfx_ds *ds,long x,long y) {
	WIDGET *cw;

	if (c) {
		x += c->wd->x;
		y += c->wd->y;

		gfx->push_clipping(ds,x,y,c->wd->w,c->wd->h);
		cw=c->cd->last_elem;
		while (cw) {
			cw->gen->draw(cw,ds,x,y);
			cw=cw->gen->get_prev(cw);
		}
		gfx->pop_clipping(ds);
	}

}


static WIDGET *cont_find(CONTAINER *c,long x,long y) {
	WIDGET *result;
	WIDGET *cw;

	if (!c) return NULL;

	/* check if position is inside the container */
	if ((x >= c->wd->x) && (y >= c->wd->y) &&
		(x < c->wd->x+c->wd->w) && (y < c->wd->y+c->wd->h)) {

		/* we are hit - lets check our children */
		cw=c->cd->first_elem;
		while (cw) {
			result=cw->gen->find(cw, x-c->wd->x, y-c->wd->y);
			if (result) {
				return result;
			}
			cw=cw->gen->get_next(cw);
		}
		return c;
	}
	return NULL;
}



/***********************************/
/*** CONTAINER SPECIFIC METHODS ***/
/***********************************/

static void cont_add(CONTAINER *c,WIDGET *new_element) {
	if (!c) return;

	new_element->gen->set_next(new_element,c->cd->first_elem);
	if (c->cd->first_elem) {
		c->cd->first_elem->gen->set_prev(c->cd->first_elem,new_element);
	} else {
		c->cd->last_elem=new_element;
	}
	new_element->gen->set_prev(new_element,NULL);

	new_element->gen->set_parent(new_element,c);
	c->cd->first_elem=new_element;

	new_element->gen->inc_ref(new_element);
	new_element->gen->force_redraw(new_element);

}

static void cont_remove(CONTAINER *c,WIDGET *element) {
	WIDGET *cw;

	if (!c) return;
	if (!element) return;
	if (element->gen->get_parent(element)!=c) return;

	/* first widget in our list? */
	if (element==c->cd->first_elem) {
		c->cd->first_elem=element->gen->get_next(element);
	} else {

		/* search in list */
		cw=c->cd->first_elem;
		while (cw) {
			if(cw->gen->get_next(cw)==element) {
				cw->gen->set_next(cw,element->gen->get_next(element));
				break;
			}
			cw=cw->gen->get_next(cw);
		}
	}

	element->gen->set_next(element,NULL);
	element->gen->set_parent(element,NULL);
	element->gen->dec_ref(element);
}


static WIDGET *cont_get_content(CONTAINER *c) {
	if (c) return c->cd->first_elem;
	else return NULL;
}


static struct widget_methods    gen_methods;
static struct container_methods cont_methods={
	cont_add,
	cont_remove,
	cont_get_content,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static CONTAINER *create(void) {

	/* allocate memory for new widget */
	CONTAINER *new = (CONTAINER *)malloc(sizeof(struct container)
	            + sizeof(struct widget_data)
	            + sizeof(struct container_data));
	if (!new) {
		INFO(printf("Container(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;   /* pointer to general widget methods */
	new->cont = &cont_methods;  /* pointer to container specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(struct container));
	new->cd = (struct container_data *)((long)new->wd + sizeof(struct widget_data));
	widman->default_widget_data(new->wd);

	/* set container specific default attributes */
	new->cd->first_elem=NULL;
	new->cd->last_elem=NULL;

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct container_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_container(struct dope_services *d) {

	widman  = d->get_module("WidgetManager 1.0");
	gfx     = d->get_module("Gfx 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	gen_methods.draw    = cont_draw;
	gen_methods.find    = cont_find;

	d->register_module("Container 1.0",&services);
	return 1;
}
