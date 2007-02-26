/*
 * \brief	DOpE Container widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Container widgets are layout widgets that allow
 * the placement of subwidgets by specifying pixel.
 */

struct private_container;
#define CONTAINER struct private_container
#define WIDGET CONTAINER
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "widget_data.h"
#include "widget.h"
#include "clipping.h"
#include "container.h"
#include "widman.h"


static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct clipping_services		*clip;


CONTAINER {
	/* entry must point to a general widget interface */
	struct widget_methods 		*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (container) */
	struct container_methods 	*cont;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data			*wd; /* access for container module and widget manager */
	
	/* here comes the private container specific data */
	WIDGET		*first_elem;
	WIDGET		*last_elem;
};

int init_container(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void cont_draw(CONTAINER *c,long x,long y) {
	WIDGET *cw;

	if (c) {
		x += c->wd->x;
		y += c->wd->y;

		clip->push(x,y,x+c->wd->w-1,y+c->wd->h-1);
		cw=c->last_elem;
		while (cw) {
			cw->gen->draw(cw,x,y);
			cw=cw->gen->get_prev(cw);
		}
		clip->pop();		
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
		cw=c->first_elem;
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

	new_element->gen->set_next(new_element,c->first_elem);
	if (c->first_elem) {
		c->first_elem->gen->set_prev(c->first_elem,new_element);
	} else {
		c->last_elem=new_element;
	}
	new_element->gen->set_prev(new_element,NULL);
	
	new_element->gen->set_parent(new_element,c);
	c->first_elem=new_element;
	
	new_element->gen->inc_ref(new_element);
	new_element->gen->force_redraw(new_element);
	
}

static void cont_remove(CONTAINER *c,WIDGET *element) {
	WIDGET *cw;

	if (!c) return;
	if (!element) return;
	if (element->gen->get_parent(element)!=c) return;
	
	/* first widget in our list? */
	if (element==c->first_elem) {
		c->first_elem=element->gen->get_next(element);
	} else {
	
		/* search in list */
		cw=c->first_elem;
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
	if (c) return c->first_elem;
	else return NULL;
}


static struct widget_methods 	gen_methods;
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
	CONTAINER *new = (CONTAINER *)mem->alloc(sizeof(CONTAINER)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Container(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;	/* pointer to general widget methods */
	new->cont = &cont_methods;	/* pointer to container specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(CONTAINER));
	widman->default_widget_data(new->wd);
	new->wd->max_w=640;
	new->wd->max_h=480;
		
	/* set container specific default attributes */
	new->first_elem=NULL;
	new->last_elem=NULL;

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

	mem		= d->get_module("Memory 1.0");
	widman	= d->get_module("WidgetManager 1.0");
	clip	= d->get_module("Clipping 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	gen_methods.draw	= cont_draw;
	gen_methods.find	= cont_find;
	
	d->register_module("Container 1.0",&services);	
	return 1;
}
