/*
 * \brief   DOpE Window widget module
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


struct window;
#define WIDGET struct window

#include "dopestd.h"
#include "window.h"
#include "script.h"
#include "gfx.h"
#include "redraw.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "widman.h"
#include "winman.h"
#include "winlayout.h"
#include "appman.h"
#include "userstate.h"
#include "messenger.h"

static struct widman_services    *widman;
static struct gfx_services       *gfx;
static struct winman_services    *winman;
static struct redraw_services    *redraw;
static struct userstate_services *userstate;
static struct script_services    *script;
static struct winlayout_services *winlayout;
static struct appman_services    *appman;
static struct messenger_services *msg;

#define WIN_UPDATE_NEW_CONTENT  0x01
#define WIN_UPDATE_SET_STAYTOP  0x02
#define WIN_UPDATE_ELEMENTS     0x04

#define WIN_FLAGS_STAYTOP       0x01
#define WIN_FLAGS_BACKGROUND    0x02

struct window_data {
	s32 elements;                /* bitmask of windowelements (title,closer etc.) */
	s32 bordersize;
	s32 titlesize;
	WIDGET *elem;                /* pointer to first window element */
	WIDGET *content;
	s32 min_w,min_h;
	u32 flags;
	u32 update;
};

int init_window(struct dope_services *d);


/************************/
/*** HELPER FUNCTIONS ***/
/************************/

static void resize_workarea(WINDOW *w) {
	WIDGET *wa = (WIDGET *)w->wind->content;
	s32 left   = winlayout->get_left_border  (w->wind->elements);
	s32 right  = winlayout->get_right_border (w->wind->elements);
	s32 top    = winlayout->get_top_border   (w->wind->elements);
	s32 bottom = winlayout->get_bottom_border(w->wind->elements);

	if (!wa) return;

	wa->gen->set_x((WIDGET *)wa,left);
	wa->gen->set_y((WIDGET *)wa,top);
	wa->gen->set_w((WIDGET *)wa,w->wd->w - left - right);
	wa->gen->set_h((WIDGET *)wa,w->wd->h - top - bottom);

	w->wd->max_w = wa->gen->get_max_w(wa) + left + right;
	w->wd->max_h = wa->gen->get_max_h(wa) + top + bottom;
	w->wd->min_w = wa->gen->get_min_w(wa) + left + right;
	w->wd->min_h = wa->gen->get_min_h(wa) + top + bottom;

//  if (w->wd->w > w->wd->max_w) w->wd->w = w->wd->max_w;
//  if (w->wd->h > w->wd->max_h) w->wd->h = w->wd->max_h;
//  if (w->wd->w < w->wd->min_w) w->wd->w = w->wd->min_w;
//  if (w->wd->h < w->wd->min_h) w->wd->h = w->wd->min_h;
//  w->wd->update |= WID_UPDATE_SIZE;
}


static void adopt_win_elements(WINDOW *w,WIDGET *cw) {
	while (cw) {
		cw->gen->set_parent(cw,w);
		cw->gen->update(cw,0);
		cw = cw->gen->get_next(cw);
	}
}


static void destroy_win_elements(WIDGET *cw) {
	WIDGET *nw;
	while (cw) {
		nw=cw->gen->get_next(cw);
		cw->gen->dec_ref(cw);
		cw=nw;
	}
}


/***********************************/
/*** USERSTATE HANDLER FUNCTIONS ***/
/***********************************/

/*** VARIABLES FOR USERSTATE HANDLING ***/
static long    owx1,owy1,owx2,owy2;   /* original window area */
static long    nwx1,nwy1,nwx2,nwy2;   /* new window area */
static WINDOW *curr_window;           /* currently modified window */


static void win_move_motion_callback(WIDGET *cw, int dx, int dy) {
	if (!curr_window) return;
	nwx1 = owx1 + dx;
	nwy1 = owy1 + dy;
	curr_window->gen->set_x((WIDGET *)curr_window, nwx1);
	curr_window->gen->set_y((WIDGET *)curr_window, nwy1);
	curr_window->gen->update((WIDGET *)curr_window,WID_UPDATE_REDRAW);
	if (dx || dy) {
		u8 *m  = curr_window->gen->get_bind_msg((WIDGET *)curr_window,"move");
		s32 id = curr_window->gen->get_app_id((WIDGET *)curr_window);
		if (m) msg->send_action_event(id,"move",m);
	}
}


static void win_move_leave_callback(WIDGET *cw, int dx, int dy) {
	if (!curr_window) return;

	if (dx || dy) {
		u8 *m  = curr_window->gen->get_bind_msg((WIDGET *)curr_window,"moved");
		s32 id = curr_window->gen->get_app_id((WIDGET *)curr_window);
		if (m) msg->send_action_event(id,"moved",m);
	}

	if ((dx>-2) && (dx<2) && (dy>-2) && (dy<2)) {
		curr_window->win->top(curr_window);
	}

	if (cw) {
		cw->gen->set_state(cw,0);
		cw->gen->update(cw,WID_UPDATE_REDRAW);
	}
}


static void win_resize_motion_callback(WIDGET *cw, int dx, int dy) {
	long size_flags = 0;
	long min_w, max_w, min_h, max_h;

	if (!cw || !curr_window) return;

	size_flags = (long)cw->gen->get_context(cw);
	min_w = curr_window->gen->get_min_w((WIDGET *)curr_window);
	max_w = curr_window->gen->get_max_w((WIDGET *)curr_window);
	min_h = curr_window->gen->get_min_h((WIDGET *)curr_window);
	max_h = curr_window->gen->get_max_h((WIDGET *)curr_window);

	/* flag for left resizing */
	if (size_flags & 1) {
		nwx1 = owx1 + dx;
		if (nwx1 + min_w - 1 > owx2) nwx2 = nwx1 + min_w - 1;
		else if (nwx1 + max_w - 1 < owx2) nwx2 = nwx1 + max_w - 1;
		else nwx2 = owx2;
	}

	/* flag for right resizing */
	if (size_flags & 4) {
		nwx2 = owx2 + dx;
		if (owx1 + min_w - 1 > nwx2) nwx1 = nwx2 - min_w + 1;
		else if (owx1 + max_w - 1 < nwx2) nwx1 = nwx2 - max_w + 1;
		else nwx1 = owx1;
	}

	/* flag for top resizing */
	if (size_flags & 2) {
		nwy1 = owy1 + dy;
		if (nwy1 + min_h - 1 > owy2) nwy2 = nwy1 + min_h - 1;
		else if (nwy1 + max_h - 1 < owy2) nwy2 = nwy1 + max_h - 1;
		else nwy2 = owy2;
	}

	/* flag for bottom resizing */
	if (size_flags & 8) {
		nwy2 = owy2 + dy;
		if (owy1 + min_h - 1 > nwy2) nwy1 = nwy2 - min_h + 1;
		else if (owy1 + max_h - 1 <nwy2) nwy1 = nwy2 - max_h + 1;
		else nwy1 = owy1;
	}

	curr_window->gen->set_x((WIDGET *)curr_window, nwx1);
	curr_window->gen->set_y((WIDGET *)curr_window, nwy1);
	curr_window->gen->set_w((WIDGET *)curr_window, nwx2 - nwx1 + 1);
	curr_window->gen->set_h((WIDGET *)curr_window, nwy2 - nwy1 + 1);
	curr_window->gen->update((WIDGET *)curr_window,WID_UPDATE_REDRAW);
}


static void win_resize_leave_callback(WIDGET *cw, int dx, int dy) {
	if (!curr_window) return;

	if (dx || dy) {
		u8  *m = curr_window->gen->get_bind_msg((WIDGET *)curr_window,"resized");
		s32 id = curr_window->gen->get_app_id((WIDGET *)curr_window);
		if (m) msg->send_action_event(id,"resized",m);
		if ((owx1 != nwx1) || (owy1 != nwy1)) {
			m = curr_window->gen->get_bind_msg((WIDGET *)curr_window,"moved");
			if (m) msg->send_action_event(id,"moved",m);
		}
	}

	if ((dx>-2) && (dx<2) && (dy>-2) && (dy<2)) {
		curr_window->win->top(curr_window);
	}

	if (cw) {
		cw->gen->set_state(cw,0);
		cw->gen->update(cw,WID_UPDATE_REDRAW);
	}
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void (*orig_set_app_id) (WIDGET *w,s32 app_id);
static void win_set_app_id(WINDOW *w,s32 app_id) {
	char *app_name;
	if (!w) return;
	app_name = appman->get_app_name(app_id);
	if (app_name) winlayout->set_win_title(w->wind->elem,app_name);
	if (orig_set_app_id) orig_set_app_id((WIDGET *)w,app_id);
}

static void win_draw(WINDOW *w,struct gfx_ds *ds,long x,long y) {
	static long x1,y1,x2,y2;
	WIDGET *cw;

	if (w->wind->flags & WIN_FLAGS_BACKGROUND) {
		gfx->draw_box(ds,w->wd->x+x,w->wd->y+y,w->wd->w,w->wd->h,0x708090ff);
	}

	/* draw window content */
	cw = w->wind->content;
	if (cw) {
		x1 = cw->gen->get_x(cw) + w->wd->x + x;
		y1 = cw->gen->get_y(cw) + w->wd->y + y;
		x2 = x1 + cw->gen->get_w(cw) - 1;
		y2  =y1 + cw->gen->get_h(cw) - 1;
		gfx->push_clipping(ds,x1,y1,x2-x1+1,y2-y1+1);
		cw->gen->draw(cw,ds,w->wd->x+x,w->wd->y+y);
		gfx->pop_clipping(ds);
	}

	/* draw window elements */
	cw = w->wind->elem;
	while (cw) {
		x1 = cw->gen->get_x(cw) + w->wd->x+x;
		y1 = cw->gen->get_y(cw) + w->wd->y+y;
		x2 = x1 + cw->gen->get_w(cw) - 1;
		y2 = y1 + cw->gen->get_h(cw) - 1;
		gfx->push_clipping(ds,x1,y1,x2-x1+1,y2-y1+1);
		cw->gen->draw(cw,ds,w->wd->x+x,w->wd->y+y);
		gfx->pop_clipping(ds);
		cw = cw->gen->get_next(cw);
	}
}



static void (*orig_win_update) (WINDOW *w,u16 redraw_flag);

static void win_update(WINDOW *w,u16 redraw_flag) {
	u16 redraw=redraw_flag;

	if (w->wind->update & WIN_UPDATE_ELEMENTS) {
		destroy_win_elements(w->wind->elem);
		w->wind->elem = winlayout->create_win_elements(w->wind->elements,w->wd->w,w->wd->h);
		adopt_win_elements(w,w->wind->elem);
	}

	if ((w->wd->update & WID_UPDATE_SIZE) || (w->wind->update & (WIN_UPDATE_ELEMENTS | WIN_UPDATE_NEW_CONTENT))) {
		//      redraw=0;
		winlayout->resize_win_elements(w->wind->elem,w->wind->elements,w->wd->w,w->wd->h);
		resize_workarea(w);
	}
	if ((w->wd->update & WID_UPDATE_SIZE) || (w->wind->update & WIN_UPDATE_NEW_CONTENT)) {
		if (w->wind->content) w->wind->content->gen->update(w->wind->content,0);
		if (redraw) w->gen->force_redraw(w);
	}
	orig_win_update(w,redraw_flag);
	if (w->wind->update & WIN_UPDATE_SET_STAYTOP) winman->reorder();

	w->wind->update=0;
}

static u16 win_do_layout(WINDOW *w, WIDGET *child, u16 redraw_flag) {
	WIDGET *wa = (WIDGET *)w->wind->content;
	s32 left   = winlayout->get_left_border  (w->wind->elements);
	s32 right  = winlayout->get_right_border (w->wind->elements);
	s32 top    = winlayout->get_top_border   (w->wind->elements);
	s32 bottom = winlayout->get_bottom_border(w->wind->elements);

	if (!wa) return redraw_flag;

//  wa->gen->set_x((WIDGET *)wa,left);
//  wa->gen->set_y((WIDGET *)wa,top);
//  wa->gen->set_w((WIDGET *)wa,w->wd->w - left - right);
//  wa->gen->set_h((WIDGET *)wa,w->wd->h - top - bottom);

	w->wd->max_w = wa->gen->get_max_w(wa) + left + right;
	w->wd->max_h = wa->gen->get_max_h(wa) + top + bottom;
	w->wd->min_w = wa->gen->get_min_w(wa) + left + right;
	w->wd->min_h = wa->gen->get_min_h(wa) + top + bottom;

	if (w->wd->w > w->wd->max_w) w->gen->set_w(w,w->wd->max_w);
	if (w->wd->h > w->wd->max_h) w->gen->set_h(w,w->wd->max_h);
	if (w->wd->w < w->wd->min_w) w->gen->set_w(w,w->wd->min_w);
	if (w->wd->h < w->wd->min_h) w->gen->set_h(w,w->wd->min_h);

	winlayout->resize_win_elements(w->wind->elem,w->wind->elements,w->wd->w,w->wd->h);
	resize_workarea(w);
	w->gen->force_redraw(w);
	orig_win_update(w,redraw_flag);
	return 0;
}

static WIDGET *win_find(WINDOW *w,long x,long y) {
	WIDGET *result;
	WIDGET *c;

	if (!w) return NULL;

	/* check if position is inside the window */
	if ((x >= w->wd->x) && (y >= w->wd->y) &&
		(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {

		/* we are hit - lets check our children */
		c = (WIDGET *)w->wind->content;
		if (c) {
			result = c->gen->find(c, x-w->wd->x, y-w->wd->y);
			if (result) return result;
		}

		/* window elements */
		c = w->wind->elem;
		while (c) {
			result = c->gen->find(c, x-w->wd->x, y-w->wd->y);
			if (result) return result;
			c = c->gen->get_next(c);
		}
		return w;
	}
	return NULL;
}


static void win_handle_event(WIDGET *w,EVENT *e) {

	if (e->type == EVENT_PRESS) {
		curr_window = w;
		nwx1 = owx1 = curr_window->gen->get_x((WIDGET *)w);
		nwy1 = owy1 = curr_window->gen->get_y((WIDGET *)w);
		userstate->drag(w, win_move_motion_callback, NULL, win_move_leave_callback);
	}
}


static char *win_get_type(WINDOW *w) {
	return "Window";
}

/*******************************/
/*** WINDOW SPECIFIC METHODS ***/
/*******************************/


static void win_set_content(WINDOW *w, WIDGET *new_content) {
	s32 left   = winlayout->get_left_border  (w->wind->elements);
	s32 right  = winlayout->get_right_border (w->wind->elements);
	s32 top    = winlayout->get_top_border   (w->wind->elements);
	s32 bottom = winlayout->get_bottom_border(w->wind->elements);

	if (!w || !w->wind || !new_content) return;
	if (w->wind->content) {
		w->wind->content->gen->set_parent(w->wind->content,NULL);
		w->wind->content->gen->dec_ref(w->wind->content);
	}
	w->wind->content = new_content;
	new_content->gen->set_parent(new_content,w);
	new_content->gen->inc_ref(new_content);

	w->wd->max_w = new_content->gen->get_max_w(new_content) + left + right;
	w->wd->max_h = new_content->gen->get_max_h(new_content) + top + bottom;
	w->wd->min_w = new_content->gen->get_min_w(new_content) + left + right;
	w->wd->min_h = new_content->gen->get_min_h(new_content) + top + bottom;

	if (w->wd->w > w->wd->max_w) w->gen->set_w(w,w->wd->max_w);
	if (w->wd->h > w->wd->max_h) w->gen->set_h(w,w->wd->max_h);
	if (w->wd->w < w->wd->min_w) w->gen->set_w(w,w->wd->min_w);
	if (w->wd->h < w->wd->min_h) w->gen->set_h(w,w->wd->min_h);

	w->wind->update |= WIN_UPDATE_NEW_CONTENT;
}


static WIDGET *win_get_content(WINDOW *w) {
	if (!w || !w->wind) return NULL;
	return w->wind->content;
}


static void win_open(WINDOW *w) {
	if (!w || !w->wind) return;
	winman->add(w);
}


static void win_close(WINDOW *w) {
	if (!w || !w->wind) return;
	winman->remove(w);
}


static void win_top(WINDOW *w) {
	u8 *m;
	winman->top(w);
	m = w->gen->get_bind_msg(w,"top");
	if (m) msg->send_action_event(w->wd->app_id,"top",m);
}


static void win_activate(WINDOW *w) {
	if (!w || !w->wind) return;
	winman->activate(w);
}


static void win_set_staytop(WINDOW *w,s16 staytop_flag) {
	if (!w || !w->wind) return;
	if (staytop_flag) {
		w->wind->flags = w->wind->flags | WIN_FLAGS_STAYTOP;
	} else {
		w->wind->flags = w->wind->flags & (WIN_FLAGS_STAYTOP^0xffffffff);
	}
	w->wind->update = w->wind->update | WIN_UPDATE_SET_STAYTOP;
}


static s16 win_get_staytop(WINDOW *w) {
	if (!w || !w->wind) return 0;
	if (w->wind->flags & WIN_FLAGS_STAYTOP) return 1;
	return 0;
}


static void win_set_elem_mask(WINDOW *w,s32 new_elem_mask) {
	if (!w || !w->wind) return;
	w->wind->elements = new_elem_mask;
	w->wind->update = w->wind->update | WIN_UPDATE_ELEMENTS;
}


static s32 win_get_elem_mask(WINDOW *w) {
	if (!w || !w->wind) return 0;
	return w->wind->elements;
}


static void win_set_state(WINDOW *w,s32 state) {
	if (!w) return;
	winlayout->set_win_state(w->wind->elem,state);
}


static void win_set_title(WINDOW *w,char *new_title) {
	if (!w || !w->wind || !new_title) return;
	winlayout->set_win_title(w->wind->elem, new_title);
}


static char *win_get_title(WINDOW *w) {
	if (!w || !w->wind) return NULL;
	return winlayout->get_win_title(w->wind->elem);
}


static void win_set_background(WINDOW *w,s16 bg_flag) {
	if (!w || !w->wind) return;
	if (bg_flag) {
		w->wind->flags = w->wind->flags | WIN_FLAGS_BACKGROUND;
	} else {
		w->wind->flags = w->wind->flags & (WIN_FLAGS_BACKGROUND^0xffffffff);
	}
	w->wind->update |= WIN_UPDATE_NEW_CONTENT;
}


static s16 win_get_background(WINDOW *w) {
	if (!w || !w->wind) return 0;
	if (w->wind->flags & WIN_FLAGS_BACKGROUND) return 1;
	return 0;
}


static void win_handle_resize(WINDOW *w, WIDGET *cw) {
	cw->gen->set_state(cw,1);
	cw->gen->update(cw,WID_UPDATE_REDRAW);

	/* determine the associated window and its current position */
	curr_window=(WINDOW *)cw->gen->get_window(cw);
	nwx1 = owx1 = curr_window->gen->get_x((WIDGET *)curr_window);
	nwy1 = owy1 = curr_window->gen->get_y((WIDGET *)curr_window);
	nwx2 = owx2 = owx1 + curr_window->gen->get_w((WIDGET *)curr_window) - 1;
	nwy2 = owy2 = owy1 + curr_window->gen->get_h((WIDGET *)curr_window) - 1;

	userstate->drag(cw, win_resize_motion_callback, NULL, win_resize_leave_callback);
}


static void win_handle_move(WINDOW *w, WIDGET *cw) {
	cw->gen->set_state(cw,1);
	cw->gen->update(cw,WID_UPDATE_REDRAW);

	/* determine the associated window and its current position */
	curr_window=(WINDOW *)cw->gen->get_window(cw);
	if (!curr_window) return;

	nwx1 = owx1 = curr_window->gen->get_x((WIDGET *)curr_window);
	nwy1 = owy1 = curr_window->gen->get_y((WIDGET *)curr_window);

	userstate->drag(cw, win_move_motion_callback, NULL, win_move_leave_callback);
}


static struct widget_methods gen_methods;
static struct window_methods win_methods={
	win_set_content,
	win_get_content,
	win_set_staytop,
	win_get_staytop,
	win_set_elem_mask,
	win_get_elem_mask,
	win_set_title,
	win_get_title,
	win_set_background,
	win_get_background,
	win_set_state,
	win_activate,
	win_open,
	win_close,
	win_top,
	win_handle_move,
	win_handle_resize,
};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static WINDOW *create(void) {

	/* allocate memory for new widget */
	WINDOW *new = (WINDOW *)malloc(sizeof(WINDOW)
	                             + sizeof(struct widget_data)
	                             + sizeof(struct window_data));
	if (!new) {
		INFO(printf("Window(create): out of memory\n"));
		return NULL;
	}
	new->gen  = &gen_methods;            /* pointer to general widget methods */
	new->win  = &win_methods;            /* pointer to window specific methods */
	new->wd   = (struct widget_data *)((long)new + sizeof(WINDOW));
	new->wind = (struct window_data *)((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);
	new->wd->x = new->wd->ox = 50;
	new->wd->y = new->wd->oy = 50;
	new->wd->w = new->wd->ow = 128;
	new->wd->h = new->wd->oh = 128;
	new->wd->min_w = 48*2;
	new->wd->min_h = 48*2;
	new->wd->max_w = 3000;
	new->wd->max_h = 3000;

	/* set window specific attributes */
	new->wind->flags = WIN_FLAGS_BACKGROUND;
	new->wind->elements = WIN_CLOSER+WIN_FULLER+WIN_TITLE+WIN_BORDERS;
	new->wind->content = NULL;
	new->wind->update = 0;

	/* create window elements */
	new->wind->elem = winlayout->create_win_elements(new->wind->elements,
	                                                 new->wd->w,new->wd->h);

	/* tell all window elements about its proud parent */
	adopt_win_elements(new,new->wind->elem);

	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct window_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype = script->reg_widget_type("Window",(void *(*)(void))create);

	script->reg_widget_method(widtype,"void open(void)",&win_open);
	script->reg_widget_method(widtype,"void close(void)",&win_close);
	script->reg_widget_method(widtype,"void top(void)",&win_top);

	script->reg_widget_attrib(widtype,"Widget content",win_get_content,win_set_content,win_update);
	script->reg_widget_attrib(widtype,"boolean staytop",win_get_staytop,win_set_staytop,win_update);
	script->reg_widget_attrib(widtype,"boolean background",win_get_background,win_set_background,win_update);
	script->reg_widget_attrib(widtype,"string title",win_get_title,win_set_title,win_update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_window(struct dope_services *d) {

	gfx       = d->get_module("Gfx 1.0");
	widman    = d->get_module("WidgetManager 1.0");
	winman    = d->get_module("WindowManager 1.0");
	redraw    = d->get_module("RedrawManager 1.0");
	userstate = d->get_module("UserState 1.0");
	script    = d->get_module("Script 1.0");
	winlayout = d->get_module("WinLayout 1.0");
	appman    = d->get_module("ApplicationManager 1.0");
	msg       = d->get_module("Messenger 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	orig_win_update         = gen_methods.update;
	orig_set_app_id         = gen_methods.set_app_id;

	gen_methods.get_type    = &win_get_type;
	gen_methods.draw        = &win_draw;
	gen_methods.update      = &win_update;
	gen_methods.set_app_id  = &win_set_app_id;
	gen_methods.find        = &win_find;
	gen_methods.handle_event= &win_handle_event;
	gen_methods.do_layout   = &win_do_layout;

	/* register script commands */
	build_script_lang();

	d->register_module("Window 1.0",&services);
	return 1;
}
