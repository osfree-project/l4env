/*
 * \brief	DOpE Window widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */


struct private_window;
#define WINDOW struct private_window
#define WIDGET WINDOW
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "script.h"
#include "memory.h"
#include "clipping.h"
#include "frame.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "widman.h"
#include "winman.h"
#include "window.h"
#include "winlayout.h"
#include "appman.h"
#include "userstate.h"
#include "messenger.h"

static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct frame_services 		*frame;
static struct clipping_services 	*clip;
static struct winman_services 		*winman;
static struct userstate_services 	*userstate;
static struct script_services		*script;
static struct winlayout_services	*winlayout;
static struct appman_services		*appman;
static struct messenger_services	*msg;

#define WIN_UPDATE_FRAME_UPDATE	0x01
#define WIN_UPDATE_SET_STAYTOP 	0x02
#define WIN_UPDATE_ELEMENTS		0x04

#define WIN_FLAGS_STAYTOP 0x01

struct private_window {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (window) */
	struct window_methods 	*win;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; /* access for window module and widget manager */
	
	/* here comes the private windos specific data */	
	s32	elements;	/* bitmask of windowelements (title,closer etc.) */
	s32	bordersize;
	s32	titlesize;
	WIDGET *elem;	/* pointer to first window element */
	FRAME *workarea;
	s32	min_w,min_h;
	u32	flags;
	u32	update;
};

int init_window(struct dope_services *d);



/************************/
/*** HELPER FUNCTIONS ***/
/************************/

static void resize_workarea(WINDOW *w) {
	WIDGET *wa = (WIDGET *)w->workarea;
	s32 left   = winlayout->get_left_border  (w->elements);
	s32 right  = winlayout->get_right_border (w->elements);
	s32 top    = winlayout->get_top_border   (w->elements);
	s32 bottom = winlayout->get_bottom_border(w->elements);

	wa->gen->set_x((WIDGET *)wa,left);
	wa->gen->set_y((WIDGET *)wa,top);
	wa->gen->set_w((WIDGET *)wa,w->wd->w - left - right);
	wa->gen->set_h((WIDGET *)wa,w->wd->h - top - bottom);
	
	w->wd->max_w = wa->gen->get_max_w(wa) + left + right;
	w->wd->max_h = wa->gen->get_max_h(wa) + top + bottom;
	w->wd->min_w = wa->gen->get_min_w(wa) + left + right;
	w->wd->min_h = wa->gen->get_min_h(wa) + top + bottom;
	
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


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void (*orig_set_app_id) (WIDGET *w,s32 app_id);
static void win_set_app_id(WINDOW *w,s32 app_id) {
	char *app_name;
	if (!w) return;
	app_name = appman->get_app_name(app_id);
	if (app_name) winlayout->set_win_title(w->elem,app_name);
	if (orig_set_app_id) orig_set_app_id((WIDGET *)w,app_id);
}

static void win_draw(WINDOW *w,long x,long y) {
	static long x1,y1,x2,y2;
	WIDGET *cw = w->elem;
	
	/* draw window elements */
	while (cw) {		
		x1 = cw->gen->get_x(cw) + w->wd->x+x;
		y1 = cw->gen->get_y(cw) + w->wd->y+y;
		x2 = x1 + cw->gen->get_w(cw) - 1;
		y2 = y1 + cw->gen->get_h(cw) - 1;
		clip->push(x1,y1,x2,y2);
		cw->gen->draw(cw,w->wd->x+x,w->wd->y+y);
		clip->pop();
		cw = cw->gen->get_next(cw);
	}

	/* draw window content */
	cw = (WIDGET *)w->workarea;
	if (cw) {
		x1 = cw->gen->get_x(cw) + w->wd->x+x;
		y1 = cw->gen->get_y(cw) + w->wd->y+y;
		x2 = x1 + cw->gen->get_w(cw) - 1;
		y2  =y1 + cw->gen->get_h(cw) - 1;
		clip->push(x1,y1,x2,y2);
		cw->gen->draw(cw,w->wd->x+x,w->wd->y+y);
		clip->pop();
	}
}



static void (*orig_win_update) (WINDOW *w,u16 redraw_flag);

static void win_update(WINDOW *w,u16 redraw_flag) {
	u16 redraw=redraw_flag;

	if (w->update & WIN_UPDATE_ELEMENTS) {
		destroy_win_elements(w->elem);
		w->elem = winlayout->create_win_elements(w->elements,w->wd->w,w->wd->h);
		adopt_win_elements(w,w->elem);
	}

	if ((w->wd->update & WID_UPDATE_SIZE) | (w->update & WIN_UPDATE_ELEMENTS)) {
		redraw=0;		
		winlayout->resize_win_elements(w->elem,w->elements,w->wd->w,w->wd->h);
		resize_workarea(w);
		
	}
	if ((w->wd->update & WID_UPDATE_SIZE) || (w->update & WIN_UPDATE_FRAME_UPDATE)) {
		if (w->workarea) w->workarea->gen->update((WIDGET *)w->workarea,redraw);
	}
	orig_win_update(w,redraw_flag);
	if (w->update & WIN_UPDATE_SET_STAYTOP) winman->reorder();
	
	w->wd->update=0;
	w->update=0;
}


static WIDGET *win_find(WINDOW *w,long x,long y) {
	WIDGET *result;
	WIDGET *c;
	
	if (!w) return NULL;
	
	/* check if position is inside the window */
	if ((x >= w->wd->x) && (y >= w->wd->y) &&
		(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {
		
		/* we are hit - lets check our children */
		c=(WIDGET *)w->workarea;
		result=c->gen->find(c, x-w->wd->x, y-w->wd->y);
		if (result) return result;		

		/* window elements */
		c=w->elem;
		while (c) {
			result=c->gen->find(c, x-w->wd->x, y-w->wd->y);
			if (result) return result;		
			c=c->gen->get_next(c);
		}
		return w;
	}
	return NULL;
}


static void win_handle_event(WIDGET *w,EVENT *e) {
	
	if (e->type == EVENT_PRESS) {
		userstate->set(USERSTATE_WINMOVE,(WIDGET *)w);
	}
}


static char *win_get_type(WINDOW *w) {
	return "Window";
}

/*******************************/
/*** WINDOW SPECIFIC METHODS ***/
/*******************************/

static void *win_get_workarea(WINDOW *w) {
	return w->workarea;
}


static void win_open(WINDOW *win) {
	winman->add(win);
}


static void win_close(WINDOW *win) {
	winman->remove(win);
}


static void win_top(WINDOW *win) {
	u8 *m;
	winman->top(win);
	m=win->gen->get_bind_msg(win,"top");
	if (m) msg->send_action_event(win->wd->app_id,"top",m);
}


static void win_activate(WINDOW *win) {
	winman->activate(win);
}


static void win_set_staytop(WINDOW *win,s16 staytop_flag) {
	if (!win) return;
	if (staytop_flag) win->flags = win->flags | WIN_FLAGS_STAYTOP;
	else win->flags = win->flags & (WIN_FLAGS_STAYTOP^0xffffffff);
	win->update = win->update | WIN_UPDATE_SET_STAYTOP;
}


static s16 win_get_staytop(WINDOW *win) {
	if (!win) return 0;
	if (win->flags & WIN_FLAGS_STAYTOP) return 1;
	return 0;
}


static void win_set_elem_mask(WINDOW *win,s32 new_elem_mask) {
	if (!win) return;
	win->elements = new_elem_mask;
	win->update = win->update | WIN_UPDATE_ELEMENTS;
}


static s32 win_get_elem_mask(WINDOW *win) {
	if (!win) return 0;
	return win->elements;
}


static void win_set_state(WINDOW *win,s32 state) {
	if (!win) return;
	winlayout->set_win_state(win->elem,state);
}


static void win_set_title(WINDOW *win,char *new_title) {
	if (!win || !new_title) return;
	winlayout->set_win_title(win->elem,new_title);
}


static char *win_get_title(WINDOW *win) {
	if (!win) return NULL;
	return winlayout->get_win_title(win->elem);
}


/*** METHODS FOR DIRECT ACCESS OF THE CONTENT-FRAMES ATTRIBUTES ***/


static WIDGET *win_get_content(WINDOW *win) {
	FRAME *f;
	if (!win) return NULL;
	f=(FRAME *)win->workarea;
	return f->frame->get_content(f);
}


static void win_set_content(WINDOW *w,WIDGET *new_content) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_content(f,new_content);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_scrollx(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_scrollx(f);
}


static void win_set_scrollx(WINDOW *w,u32 flag) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_scrollx(f,flag);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_scrolly(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_scrolly(f);
}


static void win_set_scrolly(WINDOW *w,u32 flag) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_scrolly(f,flag);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_fitx(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_fitx(f);
}


static void win_set_fitx(WINDOW *w,u32 flag) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_fitx(f,flag);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_fity(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_fity(f);
}


static void win_set_fity(WINDOW *w,u32 flag) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_fity(f,flag);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_xview(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_xview(f);
}


static void win_set_xview(WINDOW *w,u32 xview) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_xview(f,xview);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_yview(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_yview(f);
}


static void win_set_yview(WINDOW *w,u32 yview) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_yview(f,yview);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static u32 win_get_background(WINDOW *win) {
	FRAME *f;
	if (!win) return 0;
	f=(FRAME *)win->workarea;
	return f->frame->get_background(f);
}


static void win_set_background(WINDOW *w,u32 flag) {
	FRAME *f;
	if (!w) return;
	f=(FRAME *)w->workarea;
	if (!f) return;
	f->frame->set_background(f,flag);
	w->update = w->update | WIN_UPDATE_FRAME_UPDATE;
}


static struct widget_methods gen_methods;
static struct window_methods win_methods={
	win_get_workarea,
	win_set_staytop,
	win_get_staytop,
	win_set_elem_mask,
	win_get_elem_mask,
	win_set_title,
	win_get_title,
	win_set_state,
	win_activate,
	win_open,
	win_close,
	win_top
};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static WINDOW *create(void) {

	/* allocate memory for new widget */
	WINDOW *new = (WINDOW *)mem->alloc(sizeof(WINDOW)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Window(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;	/* pointer to general widget methods */
	new->win = &win_methods;	/* pointer to window specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(WINDOW));
	widman->default_widget_data(new->wd);
	new->wd->x=50;
	new->wd->y=50;
	new->wd->w=128;
	new->wd->h=128;
		
	/* set window specific attributes */
	new->flags = 0;
	new->wd->min_w = 48*2;
	new->wd->min_h = 48*2;
	new->wd->max_w = 3000;
	new->wd->max_h = 3000;
	
	new->elements = WIN_CLOSER+WIN_FULLER+WIN_TITLE+WIN_BORDERS;

	/* create window elements */
	new->elem = winlayout->create_win_elements(new->elements,new->wd->w,new->wd->h);

	/* create frame for work area */
	new->workarea = frame->create();
	if (new->workarea) {
		new->workarea->frame->set_background(new->workarea,1);
		new->workarea->gen->set_parent((WIDGET *)new->workarea,new);
		resize_workarea(new);
		new->workarea->gen->update((WIDGET *)new->workarea,0);
	}
	
	/* tell all window elements about its proud parent */
	adopt_win_elements(new,new->elem);

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
	script->reg_widget_attrib(widtype,"boolean scrollx",win_get_scrollx,win_set_scrollx,win_update);
	script->reg_widget_attrib(widtype,"boolean scrolly",win_get_scrolly,win_set_scrolly,win_update);
	script->reg_widget_attrib(widtype,"boolean fitx",win_get_fitx,win_set_fitx,win_update);
	script->reg_widget_attrib(widtype,"boolean fity",win_get_fity,win_set_fity,win_update);
	script->reg_widget_attrib(widtype,"long xview",win_get_xview,win_set_xview,win_update);
	script->reg_widget_attrib(widtype,"long yview",win_get_yview,win_set_yview,win_update);
	script->reg_widget_attrib(widtype,"string title",win_get_title,win_set_title,win_update);
	
	widman->build_script_lang(widtype,&gen_methods);
}


int init_window(struct dope_services *d) {

	mem			=	d->get_module("Memory 1.0");
	clip		=	d->get_module("Clipping 1.0");
	widman		=	d->get_module("WidgetManager 1.0");
	frame		=	d->get_module("Frame 1.0");
	winman		=	d->get_module("WindowManager 1.0");
	userstate	=	d->get_module("UserState 1.0");
	script		=	d->get_module("Script 1.0");
	winlayout	=	d->get_module("WinLayout 1.0");
	appman		=	d->get_module("ApplicationManager 1.0");
	msg			=	d->get_module("Messenger 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	orig_win_update			= gen_methods.update;
	orig_set_app_id 		= gen_methods.set_app_id;
	
	gen_methods.get_type	= &win_get_type;
	gen_methods.draw		= &win_draw;
	gen_methods.update		= &win_update;
	gen_methods.set_app_id	= &win_set_app_id;
	gen_methods.find		= &win_find;
	gen_methods.handle_event= &win_handle_event;

	/* register script commands */
	build_script_lang();
		
	d->register_module("Window 1.0",&services);
	return 1;
}
