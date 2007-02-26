/*
 * \brief	DOpE Scrollbar widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This widget type handles scrollbars. It uses
 * Buttons as child widgets.
 */


struct private_scrollbar;
#define SCROLLBAR struct private_scrollbar
#define WIDGET SCROLLBAR
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "button.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "widman.h"
#include "input.h"
#include "scrollbar.h"
#include "userstate.h"

static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct button_services 		*but;
static struct input_services		*input;
static struct userstate_services 	*userstate;

#define SCROLLBAR_UPDATE_TYPE	0x01
#define SCROLLBAR_UPDATE_SLIDER	0x02
#define SCROLLBAR_UPDATE_VALUES	0x02

struct private_scrollbar {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity */
	struct scrollbar_methods *scroll;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; /* access for window module and widget manager */
	
	/* here comes the private scrollbar specific data */	
	u32 	type;			/* bitmask of scrollbars properties */
	u16	arrow_size;		/* width or height of an arrow-element */
	WIDGET	*inc_arrow;		/* decrement-arrow (left or top) */
	WIDGET	*dec_arrow; 	/* increment-arrow (right or bottom) */
	WIDGET	*slider_bg;		/* slider background */
	WIDGET	*slider;		/* slider */
	u32	update_flags;	/* scrollbar specific update flags */
	u32	real_size;		/* size of the real contents */
	u32	view_size;		/* size of the visible part of the contents */
	s32	view_offset;	/* position of the visible part */
	u16	step_size;		/* step movement when arrow is pressed */
	void	(*scroll_update) (void *,u16);	
							/* scroll update callback function */
	void	*scroll_update_arg;
};


int init_scrollbar(struct dope_services *d);


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void scrollbar_draw(SCROLLBAR *w,long x,long y) {
	static long x1,y1,x2,y2;

	if (!w) return;
	
	x1=w->wd->x+x;
	y1=w->wd->y+y;
	x2=x1 + w->wd->w - 1;
	y2=y1 + w->wd->h - 1;
	
	/* draw elements of the scrollbar */
	w->slider_bg->gen->draw(w->slider_bg,w->wd->x+x,w->wd->y+y);
	w->slider->gen->draw(w->slider,w->wd->x+x,w->wd->y+y);
	w->inc_arrow->gen->draw(w->inc_arrow,w->wd->x+x,w->wd->y+y);
	w->dec_arrow->gen->draw(w->dec_arrow,w->wd->x+x,w->wd->y+y);
}


static u32 calc_slider_pos(u32 real_size,u32 view_offs,u32 slid_bg_size) {
	if (real_size==0) return 0;
	return (view_offs*slid_bg_size)/real_size;
}

static u32 calc_slider_offset(u32 real_size,u32 slid_pos,u32 slid_bg_size) {
	if (slid_bg_size==0) return 0;
	return (slid_pos*real_size)/slid_bg_size;
}

static u32 calc_slider_size(u32 real_size,u32 view_size,u32 slid_bg_size) {
	u32 result;
	if (real_size==0) return 0;
	result = (view_size*slid_bg_size)/real_size;	
	if (result<slid_bg_size) return result;
	return slid_bg_size;
}


/*** SET SIZES AND POSITIONS OF SCROLLBAR CONTROL ELEMENTS ***/
static void refresh_elements(SCROLLBAR *w,u16 redraw_flag) {
	static u32 sbg_size;
	
	if (!w) return;

	if (w->view_offset + w->view_size > w->real_size) {
		w->view_offset = w->real_size - w->view_size;
		if (w->view_offset < 0) w->view_offset = 0;
	}

	if (w->type == SCROLLBAR_VER) {
	
		sbg_size = w->wd->h - 2*w->arrow_size;
	
		w->inc_arrow->gen->set_x(w->inc_arrow,0);
		w->inc_arrow->gen->set_y(w->inc_arrow,w->wd->h - w->arrow_size - 0);
		w->inc_arrow->gen->set_w(w->inc_arrow,w->wd->w - 0*2);
		w->inc_arrow->gen->set_h(w->inc_arrow,w->arrow_size);
		w->inc_arrow->gen->update(w->inc_arrow,redraw_flag);
		
		w->dec_arrow->gen->set_x(w->dec_arrow,0);
		w->dec_arrow->gen->set_y(w->dec_arrow,0);
		w->dec_arrow->gen->set_w(w->dec_arrow,w->wd->w - 0*2);
		w->dec_arrow->gen->set_h(w->dec_arrow,w->arrow_size);
		w->dec_arrow->gen->update(w->dec_arrow,redraw_flag);
		
		w->slider->gen->set_x(w->slider,2);
		w->slider->gen->set_y(w->slider,w->arrow_size + 2 +
			calc_slider_pos(w->real_size,w->view_offset,sbg_size-3));
			
		w->slider->gen->set_w(w->slider,w->wd->w - 2*2);
		w->slider->gen->set_h(w->slider,
			calc_slider_size(w->real_size,w->view_size,sbg_size-3));
			
		w->slider->gen->update(w->slider,redraw_flag);

		w->slider_bg->gen->set_x(w->slider_bg,0);
		w->slider_bg->gen->set_y(w->slider_bg,w->arrow_size);
		w->slider_bg->gen->set_w(w->slider_bg,w->wd->w);
		w->slider_bg->gen->set_h(w->slider_bg,sbg_size);
		w->slider_bg->gen->update(w->slider_bg,redraw_flag);
		
		return;
	}
	
	if (w->type == SCROLLBAR_HOR) {

		sbg_size = w->wd->w - 2*w->arrow_size;

		w->inc_arrow->gen->set_x(w->inc_arrow,w->wd->w - w->arrow_size - 0);
		w->inc_arrow->gen->set_y(w->inc_arrow,0);
		w->inc_arrow->gen->set_w(w->inc_arrow,w->arrow_size);
		w->inc_arrow->gen->set_h(w->inc_arrow,w->wd->h - 0*2);
		w->inc_arrow->gen->update(w->inc_arrow,redraw_flag);
		
		w->dec_arrow->gen->set_x(w->dec_arrow,0);
		w->dec_arrow->gen->set_y(w->dec_arrow,0);
		w->dec_arrow->gen->set_w(w->dec_arrow,w->arrow_size);
		w->dec_arrow->gen->set_h(w->dec_arrow,w->wd->h - 0*2);
		w->dec_arrow->gen->update(w->dec_arrow,redraw_flag);
		
		w->slider->gen->set_x(w->slider,w->arrow_size + 2 +
			calc_slider_pos(w->real_size,w->view_offset,sbg_size-3));
			
		w->slider->gen->set_y(w->slider,2);
		w->slider->gen->set_w(w->slider,
			calc_slider_size(w->real_size,w->view_size,sbg_size-3));
			
		w->slider->gen->set_h(w->slider,w->wd->h - 2*2);
		w->slider->gen->update(w->slider,redraw_flag);

		w->slider_bg->gen->set_x(w->slider_bg,w->arrow_size);
		w->slider_bg->gen->set_y(w->slider_bg,0);
		w->slider_bg->gen->set_w(w->slider_bg,sbg_size);
		w->slider_bg->gen->set_h(w->slider_bg,w->wd->h);
		w->slider_bg->gen->update(w->slider_bg,redraw_flag);
	
		return;
	}
}
	
	
static void (*orig_scroll_update) (SCROLLBAR *w,u16 redraw_flag);

static void scrollbar_update(SCROLLBAR *w,u16 redraw_flag) {
	refresh_elements(w,redraw_flag);
	orig_scroll_update(w,redraw_flag);
	if (w->scroll_update) w->scroll_update(w->scroll_update_arg,redraw_flag); 
}


static WIDGET *scrollbar_find(SCROLLBAR *w,long x,long y) {
	WIDGET *result;
	
	if (!w) return NULL;
	
	/* check if position is inside the scrollbar */
	if ((x >= w->wd->x) && (y >= w->wd->y) &&
		(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {
		
		/* we are hit - lets check our children */
		result=w->slider->gen->find(w->slider, x-w->wd->x, y-w->wd->y);
		if (result) return result;

		result=w->slider_bg->gen->find(w->slider_bg, x-w->wd->x, y-w->wd->y);
		if (result) return result;
		
		result=w->inc_arrow->gen->find(w->inc_arrow, x-w->wd->x, y-w->wd->y);
		if (result) return result;
		
		result=w->dec_arrow->gen->find(w->dec_arrow, x-w->wd->x, y-w->wd->y);
		if (result) return result;

	}
	return NULL;
}


/**********************************/
/*** SCROLLBAR SPECIFIC METHODS ***/
/**********************************/


/*** GET/SET TYPE OF SCROLLBAR (VERTICAL OR HORIZONTAL) ***/
static void scrollbar_set_type (SCROLLBAR *s,u32 new_type) {
	if (!s) return;
	
	if (new_type != s->type) {
		s->type=new_type;
//		refresh_elements(s,0);
	}
	s->update_flags = s->update_flags | SCROLLBAR_UPDATE_TYPE;
}
static u32 scrollbar_get_type (SCROLLBAR *s) {
	if (!s) return 0;
	return s->type;
}


/*** GET/SET Y POSITION OF SLIDER ***/
static void scrollbar_set_slider_x (SCROLLBAR *s,s32 new_sx) {
	if (!s) return;
	
	/* ignore x slider position change of vertical scrollbar */
	if (s->type == SCROLLBAR_HOR) {
	
		new_sx -= s->arrow_size - 2;		
		if (new_sx < 0) new_sx = 0;

		s->view_offset = calc_slider_offset(s->real_size,new_sx,
											s->slider_bg->gen->get_w(s->slider_bg)-3);
		if (s->view_offset > s->real_size - s->view_size) 
			s->view_offset = s->real_size - s->view_size;

		s->update_flags = s->update_flags | SCROLLBAR_UPDATE_SLIDER;
	}
}
static u32 scrollbar_get_slider_x (SCROLLBAR *s) {
	if (!s) return 0;
	return s->slider->gen->get_x(s->slider);
}


/*** GET/SET Y POSITION OF SLIDER ***/
static void scrollbar_set_slider_y (SCROLLBAR *s,s32 new_sy) {
	if (!s) return;
	
	/* ignore y slider position change of horizontal scrollbar */
	if (s->type == SCROLLBAR_VER) {

		new_sy -= s->arrow_size - 2;
		if (new_sy < 0) new_sy = 0;

		s->view_offset = calc_slider_offset(s->real_size,new_sy,
											s->slider_bg->gen->get_h(s->slider_bg)-3);
		if (s->view_offset > s->real_size - s->view_size) 
			s->view_offset = s->real_size - s->view_size;

		s->update_flags = s->update_flags | SCROLLBAR_UPDATE_SLIDER;
	}
}
static u32 scrollbar_get_slider_y (SCROLLBAR *s) {
	if (!s) return 0;
	return s->slider->gen->get_y(s->slider);
}


/*** GET/SET REAL DIMENSION OF SCROLL AREA ***/
static void scrollbar_set_real_size (SCROLLBAR *s,u32 new_real_size) {
	if (!s) return;
	s->real_size=new_real_size;
	s->update_flags = s->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_real_size (SCROLLBAR *s) {
	if (!s) return 0;
	return s->real_size;
}


/*** GET/SET VISIBLE SIZE OF SCROLL AREA ***/
static void scrollbar_set_view_size (SCROLLBAR *s,u32 new_view_size) {
	if (!s) return;
	s->view_size=new_view_size;
	s->update_flags = s->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_view_size (SCROLLBAR *s) {
	if (!s) return 0;
	return s->view_size;
}


/*** GET/SET VIEW OFFSET OF SCROLL AREA ***/
static void scrollbar_set_view_offset (SCROLLBAR *s,s32 new_view_offset) {
	if (!s) return;
	s->view_offset=new_view_offset;
	if (s->view_offset < 0) s->view_offset=0;
	if (s->view_offset > s->real_size - s->view_size) 
		s->view_offset = s->real_size - s->view_size;

	s->update_flags = s->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_view_offset (SCROLLBAR *s) {
	if (!s) return 0;
	return s->view_offset;
}

static s32 scrollbar_get_arrow_size(SCROLLBAR *s) {
	if (!s) return 0;
	return s->arrow_size;
}

static void scrollbar_reg_scroll_update(SCROLLBAR *s,void (*callback)(void *,u16),void *arg) {
	if (!s) return;
	s->scroll_update=callback;
	s->scroll_update_arg=arg;
}


static struct widget_methods gen_methods;
static struct scrollbar_methods scroll_methods={
	scrollbar_set_type,
	scrollbar_get_type,
	scrollbar_set_slider_x,
	scrollbar_get_slider_x,
	scrollbar_set_slider_y,
	scrollbar_get_slider_y,
	scrollbar_set_real_size,
	scrollbar_get_real_size,
	scrollbar_set_view_size,
	scrollbar_get_view_size,
	scrollbar_set_view_offset,
	scrollbar_get_view_offset,
	scrollbar_get_arrow_size,
	scrollbar_reg_scroll_update,
};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static WIDGET *new_button(char *txt,void *clic,long context) {
	BUTTON *nb = but->create();
	nb->but->set_click(nb,clic);
	nb->gen->set_context((WIDGET *)nb,(void *)context);
	nb->but->set_text(nb,txt);
	return (WIDGET *)nb;
}


static void slider_callback(BUTTON *b) {
	if (!b) return;
	userstate->set(USERSTATE_SCROLLDRAG,b->gen->get_parent((WIDGET *)b));
}

static void slider_bg_callback(BUTTON *b) {
	SCROLLBAR  *s;
	u16 		dir=1;	/* default right/bottom direction */
	
	if (!b) return;
	s = (SCROLLBAR *)b->gen->get_parent((WIDGET *)b);
	if (!s) return;

	/* check if mouse is on the left or right side of the slider */
	if (s->type == SCROLLBAR_HOR) {
		if (input->get_mx() < s->slider->gen->get_abs_x(s->slider)) dir=0;
	}
	if (s->type == SCROLLBAR_VER) {
		if (input->get_my() < s->slider->gen->get_abs_y(s->slider)) dir=0;
	}	

	/* change view offset dependent on the scroll direction */
	if (dir) {
		s->view_offset += s->view_size;
		if (s->view_offset > s->real_size - s->view_size) 
			s->view_offset = s->real_size - s->view_size;
	} else {	
		s->view_offset -= s->view_size;
		if (s->view_offset < 0) s->view_offset = 0;
	}
	
	s->gen->update(s,WID_UPDATE_REDRAW);
}

static void arrow_callback(BUTTON *b) {
	if (!b) return;
	userstate->set(USERSTATE_SCROLLSTEP,(WIDGET *)b);
}


static SCROLLBAR *create(void) {

	/* allocate memory for new widget */
	SCROLLBAR *new = (SCROLLBAR *)mem->alloc(sizeof(SCROLLBAR)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Scrollbar(create): out of memory\n"));
		return NULL;
	}
	new->gen 	= &gen_methods;		/* pointer to general widget methods */
	new->scroll = &scroll_methods;	/* pointer to scrollbar specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(SCROLLBAR));
	widman->default_widget_data(new->wd);
	new->wd->w=128;
	new->wd->h=128;

	/* default scrollbar attributes */
	new->type=SCROLLBAR_HOR;
	new->arrow_size=13;
	new->real_size=500;
	new->view_size=128;
	new->view_offset=0;
	new->step_size=16;
	new->scroll_update=NULL;

	/* create scrollbar element widgets */
	new->inc_arrow = new_button(NULL,arrow_callback,0);
	new->dec_arrow = new_button(NULL,arrow_callback,1);
	new->slider    = new_button(NULL,slider_callback,0);
	new->slider_bg = new_button(NULL,slider_bg_callback,0);
	
	new->slider_bg->gen->set_state(new->slider_bg,1);

	new->inc_arrow->gen->set_parent(new->inc_arrow,new);
	new->dec_arrow->gen->set_parent(new->dec_arrow,new);
	new->slider->gen->set_parent(new->slider,new);
	new->slider_bg->gen->set_parent(new->slider_bg,new);

	refresh_elements(new,WID_UPDATE_HIDDEN);
	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct scrollbar_services services = {
	create
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_scrollbar(struct dope_services *d) {

	mem			=	d->get_module("Memory 1.0");
	but			=	d->get_module("Button 1.0");
	widman		=	d->get_module("WidgetManager 1.0");
	userstate	=	d->get_module("UserState 1.0");
	input		=	d->get_module("Input 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_scroll_update	=	gen_methods.update;
	gen_methods.draw	=	&scrollbar_draw;
	gen_methods.update	=	&scrollbar_update;
	gen_methods.find	=	&scrollbar_find;
	
	d->register_module("Scrollbar 1.0",&services);
	return 1;
}
