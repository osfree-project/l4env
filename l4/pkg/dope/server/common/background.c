/*
 * \brief   DOpE Background widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct background;
#define WIDGET struct background

#include "dopestd.h"
#include "gfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "background.h"
#include "widman.h"
#include "userstate.h"

static struct gfx_services *gfx;
static struct widman_services *widman;
static struct userstate_services *userstate;

struct background_data {
	long    style;
	void    (*click) (void *);
};

int init_background(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void bg_draw(BACKGROUND *b,struct gfx_ds *ds,long x,long y) {
	if (b) {
		x+=b->wd->x;
		y+=b->wd->y;

		switch (b->bd->style) {
		case BG_STYLE_WIN:
			gfx->draw_box(ds,x,y,x+b->wd->w-1,y+b->wd->h-1,0xa0b0c0ff);
			break;
		case BG_STYLE_DESK:
			gfx->draw_box(ds,x,y,x+b->wd->w-1,y+b->wd->h-1,0x506070ff);
			break;
		}
	}
}


static void (*orig_handle_event) (BACKGROUND *b,EVENT *e);

static void bg_handle_event(BACKGROUND *b,EVENT *e) {
	if (!b) return;
	if (e->type==EVENT_PRESS) {
		if (b->bd->click) {
			b->bd->click(b);
			return;
		}
	}
	if (orig_handle_event) orig_handle_event(b,e);
}



/***********************************/
/*** BACKGROUND SPECIFIC METHODS ***/
/***********************************/

static void bg_set_style(BACKGROUND *b,long new_style) {
	b->bd->style=new_style;
}

static void bg_set_click(BACKGROUND *b,void (*click)(void *)) {
	b->bd->click=click;
}

static struct widget_methods gen_methods;
static struct background_methods bg_methods={
	bg_set_style,
	bg_set_click
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static BACKGROUND *create(void) {

	/* allocate memory for new widget */
	BACKGROUND *new = (BACKGROUND *)malloc(sizeof(struct background)
	            + sizeof(struct widget_data)
	            + sizeof(struct background_data));
	if (!new) {
		INFO(printf("Background(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;    /* pointer to general widget methods */
	new->bg  = &bg_methods;     /* pointer to button specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(struct background));
	new->bd = (struct background_data *)((long)new->wd + sizeof(struct widget_data));
	widman->default_widget_data(new->wd);

	/* set background specific default attributes */
	new->bd->style=BG_STYLE_WIN;
	new->bd->click=NULL;

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct background_services services = {
	create
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_background(struct dope_services *d) {

	gfx=d->get_module("Gfx 1.0");
	widman=d->get_module("WidgetManager 1.0");
	userstate=d->get_module("UserState 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	gen_methods.draw=bg_draw;
	orig_handle_event=gen_methods.handle_event;
	gen_methods.handle_event=bg_handle_event;

	d->register_module("Background 1.0",&services);
	return 1;
}
