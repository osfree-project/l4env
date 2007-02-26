/*
 * \brief	DOpE Background widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */


struct private_background;
#define BACKGROUND struct private_background
#define WIDGET BACKGROUND
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "basegfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "background.h"
#include "widman.h"
#include "userstate.h"

static struct memory_services *mem;
static struct basegfx_services *gfx;
static struct widman_services *widman;
static struct userstate_services *userstate;

BACKGROUND {
	/* entry must point to a general widget interface */
	struct widget_methods 		*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (button) */
	struct background_methods 	*bg;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data			*wd; /* access for button module and widget manager */
	
	/* here comes the private button specific data */
	long	style;
	void	(*click) (void *);
};

int init_background(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void bg_draw(BACKGROUND *b,long x,long y) {
	if (b) {
		x+=b->wd->x;
		y+=b->wd->y;

		switch (b->style) {
		case BG_STYLE_WIN:
			gfx->draw_box(x,y,x+b->wd->w-1,y+b->wd->h-1,GFX_BOX_WINBG);
			break;
		case BG_STYLE_DESK:
			gfx->draw_box(x,y,x+b->wd->w-1,y+b->wd->h-1,GFX_BOX_DESKBG);
			break;		
		}
	}
}


static void (*orig_handle_event) (BACKGROUND *b,EVENT *e);

static void bg_handle_event(BACKGROUND *b,EVENT *e) {
	if (!b) return;
	if (e->type==EVENT_PRESS) {
		if (b->click) {
			b->click(b);
			return;
		}
	}
	if (orig_handle_event) orig_handle_event(b,e);
}



/***********************************/
/*** BACKGROUND SPECIFIC METHODS ***/
/***********************************/

static void bg_set_style(BACKGROUND *b,long new_style) { 
	b->style=new_style; 
}

static void bg_set_click(BACKGROUND *b,void (*click)(void *)) {
	b->click=click;
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
	BACKGROUND *new = (BACKGROUND *)mem->alloc(sizeof(BACKGROUND)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Background(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;	/* pointer to general widget methods */
	new->bg = &bg_methods;		/* pointer to button specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(BACKGROUND));
	widman->default_widget_data(new->wd);
		
	/* set background specific default attributes */
	new->style=BG_STYLE_WIN;
	new->click=NULL;
	
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

	mem=d->get_module("Memory 1.0");
	gfx=d->get_module("Basegfx 1.0");
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
