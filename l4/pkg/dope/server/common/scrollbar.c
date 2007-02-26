/*
 * \brief   DOpE Scrollbar widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This widget type handles scrollbars. It uses
 * Buttons as child widgets.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct scrollbar;
#define WIDGET struct scrollbar

#include "dopestd.h"
#include "button.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "widman.h"
#include "input.h"
#include "scrollbar.h"
#include "userstate.h"

static struct widman_services    *widman;
static struct button_services    *but;
static struct input_services     *input;
static struct userstate_services *userstate;

#define SCROLLBAR_UPDATE_TYPE   0x01
#define SCROLLBAR_UPDATE_SLIDER 0x02
#define SCROLLBAR_UPDATE_VALUES 0x02

struct scrollbar_data {
	u32     type;           /* bitmask of scrollbars properties */
	u16     arrow_size;     /* width or height of an arrow-element */
	WIDGET *inc_arrow;      /* decrement-arrow (left or top) */
	WIDGET *dec_arrow;      /* increment-arrow (right or bottom) */
	WIDGET *slider_bg;      /* slider background */
	WIDGET *slider;         /* slider */
	u32     update_flags;   /* scrollbar specific update flags */
	u32     real_size;      /* size of the real contents */
	u32     view_size;      /* size of the visible part of the contents */
	s32     view_offset;    /* position of the visible part */
	u16     step_size;      /* step movement when arrow is pressed */
	void  (*scroll_update) (void *,u16);
	                        /* scroll update callback function */
	void   *scroll_update_arg;
};


int init_scrollbar(struct dope_services *d);


/***********************************/
/*** USERSTATE HANDLER FUNCTIONS ***/
/***********************************/

/*** VARIABLES FOR USERSTATE HANDLING ***/
static int        osx, osy;              /* slider position when begin of dragging */
static SCROLLBAR *curr_scrollbar;        /* currently modified scrollbar widget */
static float      curr_scroll_speed;     /* current scroll speed */
static float      max_scroll_speed = 20; /* maximal scroll speed */
static float      scroll_accel = 0.4;    /* scroll acceleration */

static void slider_motion_callback(WIDGET *w, int dx, int dy) {
	curr_scrollbar->scroll->set_slider_x(curr_scrollbar, osx + dx);
	curr_scrollbar->scroll->set_slider_y(curr_scrollbar, osy + dy);
	curr_scrollbar->gen->update((WIDGET *)curr_scrollbar, WID_UPDATE_REDRAW);
}

static void scrollstep_tick_callback(WIDGET *w, int dx, int dy) {
	s32 offset, d, mb;

	if (!curr_scrollbar) return;

	mb = input->get_mb();
	if (!mb) return;

	offset = curr_scrollbar->scroll->get_view_offset(curr_scrollbar);
	d      = (s32)curr_scroll_speed;

	/* check if left or right arrow is selected */
	if (w->gen->get_context(w)) d = -d;

	/* check if left or right mouse button is pressed */
	if (mb == 2) d = -d;

	curr_scrollbar->scroll->set_view_offset(curr_scrollbar,offset + d);
	curr_scrollbar->gen->update((WIDGET *)curr_scrollbar,WID_UPDATE_REDRAW);

	curr_scroll_speed += scroll_accel;
	if (curr_scroll_speed>max_scroll_speed) curr_scroll_speed=max_scroll_speed;
}

static void scrollstep_leave_callback(WIDGET *cw, int dx, int dy) {
	cw->gen->set_state(cw,0);
	cw->gen->update(cw,WID_UPDATE_REDRAW);
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void scrollbar_draw(SCROLLBAR *w,struct gfx_ds *ds,long x,long y) {
	static long x1,y1,x2,y2;

	if (!w) return;

	x1=w->wd->x+x;
	y1=w->wd->y+y;
	x2=x1 + w->wd->w - 1;
	y2=y1 + w->wd->h - 1;

	/* draw elements of the scrollbar */
	w->sd->slider_bg->gen->draw(w->sd->slider_bg,ds,w->wd->x+x,w->wd->y+y);
	w->sd->slider->gen->draw(w->sd->slider,ds,w->wd->x+x,w->wd->y+y);
	w->sd->inc_arrow->gen->draw(w->sd->inc_arrow,ds,w->wd->x+x,w->wd->y+y);
	w->sd->dec_arrow->gen->draw(w->sd->dec_arrow,ds,w->wd->x+x,w->wd->y+y);
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

	if (w->sd->view_offset + w->sd->view_size > w->sd->real_size) {
		w->sd->view_offset = w->sd->real_size - w->sd->view_size;
		if (w->sd->view_offset < 0) w->sd->view_offset = 0;
	}

	if (w->sd->type == SCROLLBAR_VER) {

		sbg_size = w->wd->h - 2*w->sd->arrow_size;

		w->sd->inc_arrow->gen->set_x(w->sd->inc_arrow,0);
		w->sd->inc_arrow->gen->set_y(w->sd->inc_arrow,w->wd->h - w->sd->arrow_size - 0);
		w->sd->inc_arrow->gen->set_w(w->sd->inc_arrow,w->wd->w - 0*2);
		w->sd->inc_arrow->gen->set_h(w->sd->inc_arrow,w->sd->arrow_size);
		w->sd->inc_arrow->gen->update(w->sd->inc_arrow,redraw_flag);

		w->sd->dec_arrow->gen->set_x(w->sd->dec_arrow,0);
		w->sd->dec_arrow->gen->set_y(w->sd->dec_arrow,0);
		w->sd->dec_arrow->gen->set_w(w->sd->dec_arrow,w->wd->w - 0*2);
		w->sd->dec_arrow->gen->set_h(w->sd->dec_arrow,w->sd->arrow_size);
		w->sd->dec_arrow->gen->update(w->sd->dec_arrow,redraw_flag);

		w->sd->slider->gen->set_x(w->sd->slider,2);
		w->sd->slider->gen->set_y(w->sd->slider,w->sd->arrow_size + 2 +
			calc_slider_pos(w->sd->real_size,w->sd->view_offset,sbg_size-3));

		w->sd->slider->gen->set_w(w->sd->slider,w->wd->w - 2*2);
		w->sd->slider->gen->set_h(w->sd->slider,
			calc_slider_size(w->sd->real_size,w->sd->view_size,sbg_size-3));

		w->sd->slider->gen->update(w->sd->slider,redraw_flag);

		w->sd->slider_bg->gen->set_x(w->sd->slider_bg,0);
		w->sd->slider_bg->gen->set_y(w->sd->slider_bg,w->sd->arrow_size);
		w->sd->slider_bg->gen->set_w(w->sd->slider_bg,w->wd->w);
		w->sd->slider_bg->gen->set_h(w->sd->slider_bg,sbg_size);
		w->sd->slider_bg->gen->update(w->sd->slider_bg,redraw_flag);

		return;
	}

	if (w->sd->type == SCROLLBAR_HOR) {

		sbg_size = w->wd->w - 2*w->sd->arrow_size;

		w->sd->inc_arrow->gen->set_x(w->sd->inc_arrow,w->wd->w - w->sd->arrow_size - 0);
		w->sd->inc_arrow->gen->set_y(w->sd->inc_arrow,0);
		w->sd->inc_arrow->gen->set_w(w->sd->inc_arrow,w->sd->arrow_size);
		w->sd->inc_arrow->gen->set_h(w->sd->inc_arrow,w->wd->h - 0*2);
		w->sd->inc_arrow->gen->update(w->sd->inc_arrow,redraw_flag);

		w->sd->dec_arrow->gen->set_x(w->sd->dec_arrow,0);
		w->sd->dec_arrow->gen->set_y(w->sd->dec_arrow,0);
		w->sd->dec_arrow->gen->set_w(w->sd->dec_arrow,w->sd->arrow_size);
		w->sd->dec_arrow->gen->set_h(w->sd->dec_arrow,w->wd->h - 0*2);
		w->sd->dec_arrow->gen->update(w->sd->dec_arrow,redraw_flag);

		w->sd->slider->gen->set_x(w->sd->slider,w->sd->arrow_size + 2 +
			calc_slider_pos(w->sd->real_size,w->sd->view_offset,sbg_size-3));

		w->sd->slider->gen->set_y(w->sd->slider,2);
		w->sd->slider->gen->set_w(w->sd->slider,
			calc_slider_size(w->sd->real_size,w->sd->view_size,sbg_size-3));

		w->sd->slider->gen->set_h(w->sd->slider,w->wd->h - 2*2);
		w->sd->slider->gen->update(w->sd->slider,redraw_flag);

		w->sd->slider_bg->gen->set_x(w->sd->slider_bg,w->sd->arrow_size);
		w->sd->slider_bg->gen->set_y(w->sd->slider_bg,0);
		w->sd->slider_bg->gen->set_w(w->sd->slider_bg,sbg_size);
		w->sd->slider_bg->gen->set_h(w->sd->slider_bg,w->wd->h);
		w->sd->slider_bg->gen->update(w->sd->slider_bg,redraw_flag);

		return;
	}
}


static void (*orig_scroll_update) (SCROLLBAR *w,u16 redraw_flag);

static void scrollbar_update(SCROLLBAR *w,u16 redraw_flag) {
	refresh_elements(w,redraw_flag);
	orig_scroll_update(w,redraw_flag);
	if (w->sd->scroll_update) w->sd->scroll_update(w->sd->scroll_update_arg,redraw_flag);
}


static WIDGET *scrollbar_find(SCROLLBAR *w,long x,long y) {
	WIDGET *result;

	if (!w) return NULL;

	/* check if position is inside the scrollbar */
	if ((x >= w->wd->x) && (y >= w->wd->y) &&
		(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {

		/* we are hit - lets check our children */
		result=w->sd->slider->gen->find(w->sd->slider, x-w->wd->x, y-w->wd->y);
		if (result) return result;

		result=w->sd->slider_bg->gen->find(w->sd->slider_bg, x-w->wd->x, y-w->wd->y);
		if (result) return result;

		result=w->sd->inc_arrow->gen->find(w->sd->inc_arrow, x-w->wd->x, y-w->wd->y);
		if (result) return result;

		result=w->sd->dec_arrow->gen->find(w->sd->dec_arrow, x-w->wd->x, y-w->wd->y);
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

	if (new_type != s->sd->type) {
		s->sd->type=new_type;
//      refresh_elements(s,0);
	}
	s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_TYPE;
}
static u32 scrollbar_get_type (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->type;
}


/*** GET/SET Y POSITION OF SLIDER ***/
static void scrollbar_set_slider_x (SCROLLBAR *s,s32 new_sx) {
	if (!s) return;

	/* ignore x slider position change of vertical scrollbar */
	if (s->sd->type == SCROLLBAR_HOR) {

		new_sx -= s->sd->arrow_size - 2;
		if (new_sx < 0) new_sx = 0;

		s->sd->view_offset = calc_slider_offset(s->sd->real_size,new_sx,
		                                    s->sd->slider_bg->gen->get_w(s->sd->slider_bg)-3);
		if (s->sd->view_offset > s->sd->real_size - s->sd->view_size)
			s->sd->view_offset = s->sd->real_size - s->sd->view_size;

		s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_SLIDER;
	}
}
static u32 scrollbar_get_slider_x (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->slider->gen->get_x(s->sd->slider);
}


/*** GET/SET Y POSITION OF SLIDER ***/
static void scrollbar_set_slider_y (SCROLLBAR *s,s32 new_sy) {
	if (!s) return;

	/* ignore y slider position change of horizontal scrollbar */
	if (s->sd->type == SCROLLBAR_VER) {

		new_sy -= s->sd->arrow_size - 2;
		if (new_sy < 0) new_sy = 0;

		s->sd->view_offset = calc_slider_offset(s->sd->real_size,new_sy,
		                                    s->sd->slider_bg->gen->get_h(s->sd->slider_bg)-3);
		if (s->sd->view_offset > s->sd->real_size - s->sd->view_size)
			s->sd->view_offset = s->sd->real_size - s->sd->view_size;

		s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_SLIDER;
	}
}
static u32 scrollbar_get_slider_y (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->slider->gen->get_y(s->sd->slider);
}


/*** GET/SET REAL DIMENSION OF SCROLL AREA ***/
static void scrollbar_set_real_size (SCROLLBAR *s,u32 new_real_size) {
	if (!s) return;
	s->sd->real_size=new_real_size;
	s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_real_size (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->real_size;
}


/*** GET/SET VISIBLE SIZE OF SCROLL AREA ***/
static void scrollbar_set_view_size (SCROLLBAR *s,u32 new_view_size) {
	if (!s) return;
	s->sd->view_size=new_view_size;
	s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_view_size (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->view_size;
}


/*** GET/SET VIEW OFFSET OF SCROLL AREA ***/
static void scrollbar_set_view_offset (SCROLLBAR *s,s32 new_view_offset) {
	if (!s) return;
	s->sd->view_offset=new_view_offset;
	if (s->sd->view_offset < 0) s->sd->view_offset=0;
	if (s->sd->view_offset > s->sd->real_size - s->sd->view_size)
		s->sd->view_offset = s->sd->real_size - s->sd->view_size;

	s->sd->update_flags = s->sd->update_flags | SCROLLBAR_UPDATE_VALUES;
}
static u32 scrollbar_get_view_offset (SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->view_offset;
}

static s32 scrollbar_get_arrow_size(SCROLLBAR *s) {
	if (!s) return 0;
	return s->sd->arrow_size;
}

static void scrollbar_reg_scroll_update(SCROLLBAR *s,void (*callback)(void *,u16),void *arg) {
	if (!s) return;
	s->sd->scroll_update=callback;
	s->sd->scroll_update_arg=arg;
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
	curr_scrollbar = b->gen->get_parent((WIDGET *)b);
	if (!curr_scrollbar) return;
	osx = curr_scrollbar->scroll->get_slider_x(curr_scrollbar);
	osy = curr_scrollbar->scroll->get_slider_y(curr_scrollbar);
	userstate->drag((WIDGET *)b, slider_motion_callback, NULL, NULL);
}

static void slider_bg_callback(BUTTON *b) {
	SCROLLBAR  *s;
	u16 dir=1;        /* default right/bottom direction */

	if (!b) return;
	s = (SCROLLBAR *)b->gen->get_parent((WIDGET *)b);
	if (!s) return;

	/* check if mouse is on the left or right side of the slider */
	if (s->sd->type == SCROLLBAR_HOR) {
		if (input->get_mx() < s->sd->slider->gen->get_abs_x(s->sd->slider)) dir=0;
	}
	if (s->sd->type == SCROLLBAR_VER) {
		if (input->get_my() < s->sd->slider->gen->get_abs_y(s->sd->slider)) dir=0;
	}

	/* change view offset dependent on the scroll direction */
	if (dir) {
		s->sd->view_offset += s->sd->view_size;
		if (s->sd->view_offset > s->sd->real_size - s->sd->view_size)
			s->sd->view_offset = s->sd->real_size - s->sd->view_size;
	} else {
		s->sd->view_offset -= s->sd->view_size;
		if (s->sd->view_offset < 0) s->sd->view_offset = 0;
	}

	s->gen->update(s,WID_UPDATE_REDRAW);
}

static void arrow_callback(BUTTON *b) {
	if (!b) return;
	curr_scrollbar = b->gen->get_parent((WIDGET *)b);
	if (!curr_scrollbar) return;
	b->gen->set_state((WIDGET *)b,1);
	b->gen->update((WIDGET *)b,WID_UPDATE_REDRAW);
	curr_scroll_speed=0.6;
	userstate->drag((WIDGET *)b, NULL, scrollstep_tick_callback, scrollstep_leave_callback);
}


static SCROLLBAR *create(void) {

	/* allocate memory for new widget */
	SCROLLBAR *new = (SCROLLBAR *)malloc(sizeof(struct scrollbar)
	            + sizeof(struct widget_data)
	            + sizeof(struct scrollbar_data));
	if (!new) {
		INFO(printf("Scrollbar(create): out of memory\n"));
		return NULL;
	}
	new->gen    = &gen_methods;     /* pointer to general widget methods */
	new->scroll = &scroll_methods;  /* pointer to scrollbar specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(struct scrollbar));
	new->sd = (struct scrollbar_data *)((long)new->wd + sizeof(struct widget_data));
	widman->default_widget_data(new->wd);
	new->wd->w=128;
	new->wd->h=128;

	/* default scrollbar attributes */
	new->sd->type=SCROLLBAR_HOR;
	new->sd->arrow_size=13;
	new->sd->real_size=500;
	new->sd->view_size=128;
	new->sd->view_offset=0;
	new->sd->step_size=16;
	new->sd->scroll_update=NULL;

	/* create scrollbar element widgets */
	new->sd->inc_arrow = new_button(NULL,arrow_callback,0);
	new->sd->dec_arrow = new_button(NULL,arrow_callback,1);
	new->sd->slider    = new_button(NULL,slider_callback,0);
	new->sd->slider_bg = new_button(NULL,slider_bg_callback,0);

	new->sd->slider_bg->gen->set_state(new->sd->slider_bg,1);

	new->sd->inc_arrow->gen->set_parent(new->sd->inc_arrow,new);
	new->sd->dec_arrow->gen->set_parent(new->sd->dec_arrow,new);
	new->sd->slider->gen->set_parent(new->sd->slider,new);
	new->sd->slider_bg->gen->set_parent(new->sd->slider_bg,new);

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

	but       = d->get_module("Button 1.0");
	widman    = d->get_module("WidgetManager 1.0");
	userstate = d->get_module("UserState 1.0");
	input     = d->get_module("Input 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_scroll_update = gen_methods.update;
	gen_methods.draw   = &scrollbar_draw;
	gen_methods.update = &scrollbar_update;
	gen_methods.find   = &scrollbar_find;

	d->register_module("Scrollbar 1.0",&services);
	return 1;
}
