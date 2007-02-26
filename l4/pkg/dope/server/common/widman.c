/*
 * \brief	DOpE widget base class module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module implements common functionality
 * of all widgets.
 */


struct private_widget;
#define WIDGET struct private_widget
#define WIDGETARG WIDGET

#include "event.h"
#include "dope-config.h"
#include "memory.h"
#include "script.h"
#include "widget_data.h"
#include "widget.h"
#include "widman.h"
#include "messenger.h"
#include "redraw.h"

#define MAX(a,b) a>b?a:b
#define MIN(a,b) a<b?a:b

static struct memory_services    *mem;
static struct redraw_services    *redraw;
static struct script_services    *script;
static struct messenger_services *msg;

/* we only need to access the interface that all widget types have in common */
WIDGET {
	struct widget_methods 	*gen;	/* pointer to general methods */
	void					*widget_specific_methods;
	struct widget_data		*wd;	/* pointer to general attributes */
};

int init_widman(struct dope_services *d);



/******************************/
/* FUNCTIONS FOR INTERNAL USE */
/******************************/

static s32 strlength(u8 *s) {
	s32 result=0;
	while (*(s++)) result++;
	return result;
}

static u8 *strdup(u8 *s) {
	u8 *d;
	u8 *result;
	s32 strl;
	if (!s) return NULL;
	strl = strlength(s);
	if (strl>=0) {
		result = mem->alloc(strl+2);
		if (!result) return NULL;
		d=result;
		while (*s) *(d++)=*(s++);
		*d=0;
		return result;
	}
	return NULL;
}

static u16 streq(char *s1,char *s2) {
	int i;
	for (i=0;i<256;i++) {
		if (*(s1) != *(s2++)) return 0;
		if (*(s1++) == 0) return 1;
	}
	return 1;
}



/*********************************************************/
/* DEFAULT IMPLEMENTATIONS FOR WIDGET BASE CLASS METHODS */
/*********************************************************/

/*** DESTROY WIDGET (NORMALLY WID_DEC_REF SHOULD BE CALLED INSTEAD OF THIS ***/
static void wid_destroy(WIDGET *w) {
	/* dont destroy widget, which are still in use */
	if (w->wd->ref_cnt) {
		DOPEDEBUG(printf("WidgetManager(destroy): The reference counter of the object %lu is not zero -\n",(long)w));
		DOPEDEBUG(printf("                        I hope you know what you are doing!..."));
	}
	mem->free(w);
}


/*** GET/SET APPLICATION ID OF THE OWNER OF THE WIDGET ***/
static long wid_get_app_id(WIDGET *w) {
	return w->wd->app_id;
}
static void wid_set_app_id(WIDGET *w,long new_app_id){
	w->wd->app_id=new_app_id;
}


/*** GET/SET WIDGET POSITION RELATIVE TO ITS PARENT ***/
static long wid_get_x(WIDGET *w) {
	return w->wd->x;
}
static void wid_set_x(WIDGET *w,long new){
	w->wd->x=new;
	w->wd->update = w->wd->update | WID_UPDATE_POS;
}
static long wid_get_y(WIDGET *w) {
	return w->wd->y;
}
static void wid_set_y(WIDGET *w,long new){
	w->wd->y=new;
	w->wd->update = w->wd->update | WID_UPDATE_POS;
}


/*** GET/SET WIDGET SIZE ***/
static long wid_get_w(WIDGET *w) {
	return w->wd->w;
}
static void wid_set_w(WIDGET *w,long new){
	if (new<4) new=4;
	w->wd->w=new;
	w->wd->update = w->wd->update | WID_UPDATE_SIZE;
}
static long wid_get_h(WIDGET *w) {
	return w->wd->h;
}
static void wid_set_h(WIDGET *w,long new){
	if (new<4) new=4;
	w->wd->h=new;
	w->wd->update = w->wd->update | WID_UPDATE_SIZE;
}


/*** GET MINIMAL/MAXIMAL SIZE OF A WIDGET ***/
static long wid_get_min_w(WIDGET *w) {
	return w->wd->min_w;
}
static long wid_get_min_h(WIDGET *w) {
	return w->wd->min_h;
}
static long wid_get_max_w(WIDGET *w) {
	return w->wd->max_w;
}
static long wid_get_max_h(WIDGET *w) {
	return w->wd->max_h;
}


/*** DETERMINE ABSOLUTE POSITION OF THE WIDGET ON THE SCREEN ***/
static long	wid_get_abs_x(WIDGET *w) {
	long x=0;
	while ((w!=NULL) && (w!=(WIDGET *)'#')) {
		x+=w->wd->x;
		w=w->wd->parent;
	}
	return x;
}
static long	wid_get_abs_y(WIDGET *w) {
	long y=0;
	while ((w!=NULL) && (w!=(WIDGET *)'#')) {
		y+=w->wd->y;
		w=w->wd->parent;
	}
	return y;
}


/*** GET/SET WIDGET STATE (SELECTED OR NOT) ***/
static long wid_get_state(WIDGET *w) {
	return w->wd->flags & WID_FLAGS_STATE;
}
static void wid_set_state(WIDGET *w,long new){
	if (new) w->wd->flags = w->wd->flags | WID_FLAGS_STATE;
	else w->wd->flags = w->wd->flags & (WID_FLAGS_STATE^0xffffffff);
	w->wd->update = w->wd->update | WID_UPDATE_STATE;
}


/*** GET/SET FOCUS FLAG (MOUSE OVER WIDGET) ***/
static long wid_get_focus(WIDGET *w) {
	return w->wd->flags & WID_FLAGS_FOCUS;
}
static void wid_set_focus(WIDGET *w,long new){
	if (new) w->wd->flags = w->wd->flags | WID_FLAGS_FOCUS;
	else w->wd->flags = w->wd->flags & (WID_FLAGS_FOCUS^0xffffffff);
	w->wd->update = w->wd->update | WID_UPDATE_FOCUS;
}


/*** GET/SET CURRENT PARENT OF WIDGET ***/
static WIDGET * wid_get_parent(WIDGET *w) {
	return w->wd->parent;
}
static void wid_set_parent(WIDGET *w,void *p)	{
	w->wd->parent=p;
	w->wd->update = w->wd->update | WID_UPDATE_PARENT;
}


/*** GET/SET POINTER TO ASSOCIATED 'USER' DATA ***/
static void *wid_get_context(WIDGET *w) {
	return w->wd->context;
}
static void wid_set_context(WIDGET *w,void *c) {
	w->wd->context=c;
}


/*** GET/SET NEXT/PREVIOUS ELEMENT WHEN THE WIDGET IS USED IN A CONNECTED LIST ***/
static WIDGET *	wid_get_next(WIDGET *w) {
	return w->wd->next;
}
static void wid_set_next(WIDGET *w,WIDGET *n)	{
	w->wd->next=n;
}
static WIDGET *	wid_get_prev(WIDGET *w) {
	return w->wd->prev;
}
static void wid_set_prev(WIDGET *w,WIDGET *p)	{
	w->wd->prev=p;
}


/*** DRAW A WIDGET (DUMMY - MUST BE OVERWRITTEN BY SOMETHING MORE USEFUL ***/
static void wid_draw(WIDGET *w,long x,long y) {
	w=w;x=x;y=y;	/* just to avoid warnings */
}


/*** FIND WIDGET INSIDE A WIDGET AT THE GIVEN POSITION (RELATIVE TO PARENT) ***/
static WIDGET *wid_find(WIDGET *w,long x,long y) {
	if (w) {
		if ((x >= w->wd->x) && (y >= w->wd->y) && 
			(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {
			return w;
		}
	}
	return NULL;
}


/*** DETERMINE THE WINDOW IN WHERE THE WIDGET IS ***/
static WIDGET *wid_get_window(WIDGET *cw) {
	if (!cw) return NULL;
	while ((cw->wd->parent!=NULL) && (cw->wd->parent!=(WIDGET *)'#')) {
		cw=cw->wd->parent;
	}
	/* return value is either the window that contains the */
	/* specified widget (window has '#' as parent) or NULL */
	/* if the widget is homeless */
	return cw;
}


/*** UPDATE WIDGETS WHEN ATTRIBUTES CHANGED ***/
static void wid_update(WIDGET *w,u16 redraw_flag) {
	long nx1,ny1,nx2,ny2;
	long ox1,oy1,ox2,oy2;
	WIDGET *p;
	WIDGET *win=wid_get_window(w);

	if ((w->wd->update & WID_UPDATE_POS) || (w->wd->update & WID_UPDATE_SIZE)) {
		nx1 = wid_get_abs_x(w);
		ny1 = wid_get_abs_y(w);
		nx2 = nx1 + w->wd->w - 1;
		ny2 = ny1 + w->wd->h - 1;
		
		ox1 = nx1 - w->wd->x + w->wd->ox;
		oy1 = ny1 - w->wd->y + w->wd->oy;
		ox2 = ox1 + w->wd->ow - 1;
		oy2 = oy1 + w->wd->oh - 1;
		
		/* if the updated widget is a window use the general draw function */
		if (w==win) {
			if (redraw_flag) redraw->draw_area(NULL,MIN(ox1,nx1),MIN(oy1,ny1),MAX(ox2,nx2),MAX(oy2,ny2));
		}

		/* otherwise limit the drawing to window area */
		else {
			p=w->wd->parent;
			if (p) redraw_flag = p->gen->do_layout(p,w,redraw_flag);
			if (redraw_flag) redraw->draw_area(win,MIN(ox1,nx1),MIN(oy1,ny1),MAX(ox2,nx2),MAX(oy2,ny2));
		}
		
		w->wd->ox=w->wd->x;
		w->wd->oy=w->wd->y;
		w->wd->ow=w->wd->w;
		w->wd->oh=w->wd->h;
	}
	w->wd->update = 0;
}


/*** INC/DECREMENT REFERENCE COUNTER OF WIDGET ***/
static void	wid_inc_ref(WIDGET *w) {
	if (!w) return;
	w->wd->ref_cnt++;
}
static void	wid_dec_ref(WIDGET *w) {
	if (!w) return;
	w->wd->ref_cnt--;
	if (w->wd->ref_cnt<=0) {
		DOPEDEBUG(printf("widget %lu ref_cnt reached zero -> commit suicide\n",(long)w));
		w->gen->destroy(w);
	}
}


/*** CAUSE A REDRAW OF THE WIDGET ***/
static void	wid_force_redraw(WIDGET *cw) {
	redraw->draw_widget(cw);
}


/*** HANDLE EVENTS ***/
static void wid_handle_event(WIDGET *cw,EVENT *e) {
	struct binding_struct *cb = cw->wd->bindings;
	s16 propagate = 1;

	/* check bindings */
	while (cb) {
		if (cb->ev_type == e->type) {
			msg->send_input_event(cw->wd->app_id,e,cb->msg);
			propagate = 0;
		}
		cb=cb->next;
	}

	if (propagate == 0) return;

	/* propagate event to parent widget by default */
	if ((cw->wd->parent) && (cw->wd->parent!=(WIDGET *)'#')) {
		cw=cw->wd->parent;
		cw->gen->handle_event(cw,e);
	}
}


/*** DO THE LAYOUT INSIDE A WIDGET ***/
static u16 wid_do_layout(WIDGET *cw,WIDGET *child,u16 redraw_flag) {

	/* ... we leave the redraw_flag as it is ... */
	
	/* return 1 -> we didnt made a redraw - the caller has to do this */
	/* return 0 -> we made a redraw - the caller dont has to do anything more */

	return redraw_flag;
}


/*** ADD EVENT BINDING FOR A WIDGET ***/
static void wid_bind(WIDGET *cw,char *bind_ident,char *message) {
	struct binding_struct *new;
	
	printf("Widman(bind): create new binding for %s\n",bind_ident);
	new = (struct binding_struct *)mem->alloc(sizeof(struct binding_struct));
	if (!new) {
		ERROR(printf("WidgetManager(bind): out of memory!\n");)
		return;
	}

	new->msg = strdup(message);
	new->next = cw->wd->bindings;
	new->bind_ident = strdup(bind_ident);
	cw->wd->bindings = new;
	
	if (streq(bind_ident,"press")) 	{new->ev_type = EVENT_PRESS; return;}
	if (streq(bind_ident,"release")){new->ev_type = EVENT_RELEASE; return;}
	if (streq(bind_ident,"motion"))	{new->ev_type = EVENT_MOTION; return;}
	if (streq(bind_ident,"leave")) 	{new->ev_type = EVENT_MOUSE_LEAVE;  return;}
	if (streq(bind_ident,"enter")) 	{new->ev_type = EVENT_MOUSE_ENTER;  return;}

	new->bind_ident = strdup(bind_ident);
	new->ev_type = EVENT_ACTION;
}


/*** CHECK IF WIDGET IS BOUND TO A CERTAIN EVENT ***/
static u8 *wid_get_bind_msg(WIDGET *cw,u8 *bind_ident) {
	struct binding_struct *cb = cw->wd->bindings;
	
	while (cb) {
		if (streq(cb->bind_ident,bind_ident)) return cb->msg;
		cb=cb->next;
	}
	return NULL;
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** SET GENERAL WIDGET ATTRIBUTES TO DEFAULT VALUES ***/
static void default_widget_data(struct widget_data *d) {
	d->x =0; d->y =0; d->w =64; d->h =64;
	d->ox=0; d->oy=0; d->ow=64; d->oh=64;
	d->min_w=0;
	d->min_h=0; 
	d->max_w=10000;   
	d->max_h=10000;
	d->update=0;
	d->next=NULL;
	d->prev=NULL;
	d->flags=0;
	d->context=(void *)0;
	d->parent=(void *)0;
	d->click=(void *)0;
	d->ref_cnt=1;
	d->app_id=-1;
	d->bindings=0;
}


/*** SET GENERAL WIDGET METHODS TO DEFAULT METHODS ***/
static void default_widget_methods(struct widget_methods *m) {
	m->destroy		= wid_destroy;
	m->get_app_id	= wid_get_app_id;
	m->set_app_id	= wid_set_app_id;
	m->get_x		= wid_get_x;
	m->set_x		= wid_set_x;
	m->get_y		= wid_get_y;
	m->set_y		= wid_set_y;
	m->get_w		= wid_get_w;
	m->set_w		= wid_set_w;
	m->get_h		= wid_get_h;
	m->set_h		= wid_set_h;
	m->get_min_w	= wid_get_min_w;
	m->get_min_h	= wid_get_min_h;
	m->get_max_w	= wid_get_max_w;
	m->get_max_h	= wid_get_max_h;
	m->get_abs_x	= wid_get_abs_x;
	m->get_abs_y	= wid_get_abs_y;
	m->get_state	= wid_get_state;
	m->set_state	= wid_set_state;
	m->get_parent	= wid_get_parent;
	m->set_parent	= wid_set_parent;
	m->get_context	= wid_get_context;
	m->set_context	= wid_set_context;
	m->get_next		= wid_get_next;
	m->set_next		= wid_set_next;
	m->get_prev		= wid_get_prev;
	m->set_prev		= wid_set_prev;
	m->draw			= wid_draw;
	m->update		= wid_update;
	m->find			= wid_find;
	m->inc_ref		= wid_inc_ref;
	m->dec_ref		= wid_dec_ref;
	m->force_redraw	= wid_force_redraw;
	m->get_focus	= wid_get_focus;
	m->set_focus	= wid_set_focus;
	m->handle_event	= wid_handle_event;
	m->get_window	= wid_get_window;
	m->do_layout	= wid_do_layout;
	m->bind			= wid_bind;
	m->get_bind_msg	= wid_get_bind_msg;
}


static void build_script_lang(void *widtype,struct widget_methods *m) {

	script->reg_widget_attrib(widtype,"string type",m->get_type,NULL,NULL);
	script->reg_widget_attrib(widtype,"long x",m->get_x,m->set_x,m->update);
	script->reg_widget_attrib(widtype,"long y",m->get_y,m->set_y,m->update);
	script->reg_widget_attrib(widtype,"long w",m->get_w,m->set_w,m->update);
	script->reg_widget_attrib(widtype,"long h",m->get_h,m->set_h,m->update);
	script->reg_widget_attrib(widtype,"long state",m->get_state,m->set_state,m->update);

	script->reg_widget_method(widtype,"void bind(string type,string msg)",m->bind);
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct widman_services services = {
	default_widget_data,
	default_widget_methods,
	build_script_lang,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_widman(struct dope_services *d) {

	mem		=	d->get_module("Memory 1.0");	
	redraw	=	d->get_module("RedrawManager 1.0");
	msg		=	d->get_module("Messenger 1.0");
	script	=	d->get_module("Script 1.0");
	
	d->register_module("WidgetManager 1.0",&services);
	return 1;
}
