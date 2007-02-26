/*
 * \brief   DOpE Scale widget module
 * \date    2002-06-05
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This widget type handles numeric scales.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct scale;
#define WIDGET struct scale

#include "dopestd.h"
#include "button.h"
#include "variable.h"
#include "gfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "widman.h"
#include "input.h"
#include "scale.h"
#include "script.h"
#include "userstate.h"
#include "messenger.h"

static struct input_services     *input;
static struct gfx_services       *gfx;
static struct widman_services    *widman;
static struct button_services    *but;
static struct script_services    *script;
static struct userstate_services *userstate;
static struct messenger_services *msg;

#define SCALE_UPDATE_TYPE   0x01
#define SCALE_UPDATE_SLIDER 0x02
#define SCALE_UPDATE_VALUES 0x02

#define SCALE_SIZE 14

struct scale_data {
	u32       type;           /* bitmask of scales properties */
	WIDGET   *slider_bg;      /* slider background */
	WIDGET   *slider;         /* slider */
	VARIABLE *var;            /* associated variable */
	float     value;          /* current scale value */
	float     from,to;        /* boundary of the scale value */
	s16       padx,pady;      /* space around the widget */
	u32       update_flags;   /* scale specific update flags */
};


int init_scale(struct dope_services *d);


/**********************************/
/*** INTERNAL UTILITY FUNCTIONS ***/
/**********************************/

static s32 calc_slider_pos(float from, float to, float value, s32 size) {
	if (from == to) return 0;
	return ((value-from)/(to-from))*size;
}

static float calc_slider_value(float from, float to, s32 size, s32 pos) {
	if (size == 0) return 0;
	return from + (((float)pos)*(to-from))/(float)size;
}


static float check_value(float from, float to, float value) {
	if (from == to) value = from;
	if (from < to)  {
		if (value < from) value = from;
		if (value > to)   value = to;
	}
	if (from > to) {
		if (value > from) value = from;
		if (value < to)   value = to;
	}
	return value;
}

/***********************************/
/*** USERSTATE HANDLER FUNCTIONS ***/
/***********************************/

/*** VARIABLES FOR USERSTATE HANDLING ***/
static int    osx, osy;         /* slider position when begin of dragging */
static SCALE *curr_scale;       /* currently modified scale widget */

static void slider_motion_callback(WIDGET *w, int dx, int dy) {
	float from, to, value;
	s32 pos, size, app_id;
	u8 *m;

	if (!(curr_scale->scale->get_type(curr_scale) & SCALE_VER)) {
		pos  = osx + dx;
		size = curr_scale->gen->get_w(curr_scale) - w->gen->get_w(w);
	} else {
		pos  = osy + dy;
		size = curr_scale->gen->get_h(curr_scale) - w->gen->get_h(w);
	}

	/* determine the new scale value */
	from  = curr_scale->sd->from;
	to    = curr_scale->sd->to;
	value = calc_slider_value(from, to, size, pos);
	
	m = curr_scale->gen->get_bind_msg(curr_scale, "slide");
	app_id = curr_scale->gen->get_app_id(curr_scale);
	if (m) msg->send_action_event(app_id, "slide", m);

	curr_scale->scale->set_value(curr_scale, value);
	curr_scale->gen->update((WIDGET *)curr_scale, WID_UPDATE_REDRAW);
}


static void slider_release_callback(WIDGET *s, int dx, int dy) {
	u8 *m = curr_scale->gen->get_bind_msg(curr_scale, "slid");
	int app_id = curr_scale->gen->get_app_id(curr_scale);
	if (m) msg->send_action_event(app_id, "slid", m);
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void scale_draw(SCALE *w,struct gfx_ds *ds,long x,long y) {
	static long x1,y1,x2,y2;

	if (!w) return;

	x1=w->wd->x+x;
	y1=w->wd->y+y;
	x2=x1 + w->wd->w - 1;
	y2=y1 + w->wd->h - 1;

	/* draw elements of the scale */
	w->sd->slider_bg->gen->draw(w->sd->slider_bg,ds,w->wd->x+x,w->wd->y+y);
	w->sd->slider->gen->draw(w->sd->slider,ds,w->wd->x+x,w->wd->y+y);

	if (w->sd->type & SCALE_VER) {
//      gfx->draw_hline
		int x = x1 + w->sd->padx + 4;
		int y = y1 + w->sd->slider->wd->y + (w->sd->slider->wd->h>>1);
		int l = w->sd->slider->wd->w - 4;
		gfx->draw_hline(ds, x, y-1, l, GFX_RGBA(0,0,0,127));
		gfx->draw_hline(ds, x, y+1, l, GFX_RGBA(255,255,255,127));
	} else {
		int x = x1 + w->sd->slider->wd->x + (w->sd->slider->wd->w>>1);
		int y = y1 + w->sd->pady + 4;
		int l = w->sd->slider->wd->h - 4;
		gfx->draw_vline(ds, x-1, y, l, GFX_RGBA(0,0,0,127));
		gfx->draw_vline(ds, x+1, y, l, GFX_RGBA(255,255,255,127));
	}
}



/*** SET SIZES AND POSITIONS OF SCALE CONTROL ELEMENTS ***/
static void refresh_elements(SCALE *w,u16 redraw_flag) {
	static s32 sbg_size;
	static s32 sld_pos;
	static s32 sld_size;
	WIDGET *slider;
	WIDGET *slider_bg;

	if (!w) return;

//  if (w->view_offset + w->view_size > w->real_size) {
//      w->view_offset = w->real_size - w->view_size;
//      if (w->view_offset < 0) w->view_offset = 0;
//  }
//
	slider = w->sd->slider;
	slider_bg = w->sd->slider_bg;

	if (w->sd->type & SCALE_VER) {
		w->wd->min_w = w->wd->max_w = SCALE_SIZE + w->sd->padx*2;
		w->wd->min_h = w->sd->pady*2 + SCALE_SIZE*4;
		w->wd->max_h = 99999;
	} else {
		w->wd->min_w = w->sd->padx*2 + SCALE_SIZE*4;
		w->wd->max_w = 99999;
		w->wd->min_h = w->wd->max_h = SCALE_SIZE + w->sd->pady*2;
	}

	if (w->wd->w > w->wd->max_w) w->wd->w = w->wd->max_w;
	if (w->wd->w < w->wd->min_w) w->wd->w = w->wd->min_w;
	if (w->wd->h > w->wd->max_h) w->wd->h = w->wd->max_h;
	if (w->wd->h < w->wd->min_h) w->wd->h = w->wd->min_h;

	if (w->sd->type & SCALE_VER) {
		sbg_size = w->wd->h - w->sd->pady*2;
		sld_size = (w->wd->w - 2*2 - w->sd->padx*2) * 3;
		sld_pos  = calc_slider_pos(w->sd->from,w->sd->to,w->sd->value,sbg_size-sld_size-4);

		slider->gen->set_x(slider,w->sd->padx + 2);
		slider->gen->set_y(slider,w->sd->pady + 2 + sld_pos);
		slider->gen->set_w(slider,w->wd->w - 2*2 - w->sd->padx*2);
		slider->gen->set_h(slider,sld_size);
		slider->gen->update(slider,redraw_flag);

		slider_bg->gen->set_x(slider_bg,w->sd->padx);
		slider_bg->gen->set_y(slider_bg,w->sd->pady);
		slider_bg->gen->set_w(slider_bg,w->wd->w - w->sd->padx*2);
		slider_bg->gen->set_h(slider_bg,sbg_size);
		slider_bg->gen->update(slider_bg,redraw_flag);

		return;

	} else {

		sbg_size = w->wd->w - w->sd->padx*2;
		sld_size = (w->wd->h - 2*2 - w->sd->pady*2) * 3;
		sld_pos  = calc_slider_pos(w->sd->from,w->sd->to,w->sd->value,sbg_size-sld_size-4);

		slider->gen->set_x(slider,w->sd->padx + 2 + sld_pos);
		slider->gen->set_y(slider,w->sd->pady + 2);
		slider->gen->set_w(slider,sld_size);
		slider->gen->set_h(slider,w->wd->h - 2*2 - w->sd->pady*2);
		slider->gen->update(slider,redraw_flag);

		slider_bg->gen->set_x(slider_bg,w->sd->padx);
		slider_bg->gen->set_y(slider_bg,w->sd->pady);
		slider_bg->gen->set_w(slider_bg,sbg_size);
		slider_bg->gen->set_h(slider_bg,w->wd->h - w->sd->pady*2);
		slider_bg->gen->update(slider_bg,redraw_flag);

		return;
	}
}


static void (*orig_scroll_update) (SCALE *w,u16 redraw_flag);

static void scale_update(SCALE *s,u16 redraw_flag) {
	int redraw_done = 0;

	refresh_elements(s,0);
	orig_scroll_update(s,0);

	if (s->sd->update_flags & SCALE_UPDATE_TYPE) {
		WIDGET *parent = s->gen->get_parent(s);
		if (parent) {
			if (parent->gen->do_layout(parent,s,redraw_flag) == 0) redraw_done = 1;
		}
		s->sd->update_flags = 0;
	}

	if (redraw_flag && !redraw_done) s->gen->force_redraw(s);

//  if (w->scroll_update) s->scroll_update(s->scroll_update_arg,redraw_flag);
}


static WIDGET *scale_find(SCALE *w,long x,long y) {
	WIDGET *result;

	if (!w) return NULL;

	/* check if position is inside the scale */
	if ((x >= w->wd->x) && (y >= w->wd->y) &&
		(x < w->wd->x+w->wd->w) && (y < w->wd->y+w->wd->h)) {

		/* we are hit - lets check our children */
		result=w->sd->slider->gen->find(w->sd->slider, x-w->wd->x, y-w->wd->y);
		if (result) return result;

		result=w->sd->slider_bg->gen->find(w->sd->slider_bg, x-w->wd->x, y-w->wd->y);
		if (result) return result;
	}
	return NULL;
}


/******************************/
/*** SCALE SPECIFIC METHODS ***/
/******************************/


/*** GET/SET TYPE OF SCALE (VERTICAL OR HORIZONTAL) ***/
static void scale_set_type (SCALE *s,u32 new_type) {
	if (!s) return;

	if (new_type != s->sd->type) {
		s->sd->type=new_type;
	}
	s->sd->update_flags = s->sd->update_flags | SCALE_UPDATE_TYPE;
}
static u32 scale_get_type (SCALE *s) {
	if (!s) return 0;
	return s->sd->type;
}


/*** GET/SET SCALE VALUE ***/
static void scale_set_value(SCALE *s,float new_value) {
	static char strbuf[24];
	int app_id;
	u8 *m;

	if (!s) return;
	s->sd->value = check_value(s->sd->from, s->sd->to, new_value);
	dope_ftoa(s->sd->value, 2, strbuf, 24);
	if (s->sd->var) s->sd->var->var->set_string(s->sd->var, &strbuf[0]);
	s->sd->update_flags = s->sd->update_flags | SCALE_UPDATE_VALUES;
	
	/* notify client that bound an "change"-event */
	m = s->gen->get_bind_msg(s,"change");
	app_id = s->gen->get_app_id(s);
	if (m) msg->send_action_event(app_id,"change",m);
	
}
static float scale_get_value (SCALE *s) {
	if (!s) return 0;
	return s->sd->value;
}


/*** GET/SET MIN SCALE VALUE ***/
static void scale_set_from(SCALE *s,float new_from) {
	if (!s) return;
	s->sd->from = new_from;
	s->sd->value = check_value(s->sd->from, s->sd->to, s->sd->value);
	s->sd->update_flags = s->sd->update_flags | SCALE_UPDATE_VALUES;
}
static float scale_get_from(SCALE *s) {
	if (!s) return 0;
	return s->sd->from;
}


/*** GET/SET MAX SCALE VALUE ***/
static void scale_set_to(SCALE *s,float new_to) {
	if (!s) return;
	s->sd->to= new_to;
	s->sd->value = check_value(s->sd->from, s->sd->to, s->sd->value);
	s->sd->update_flags = s->sd->update_flags | SCALE_UPDATE_VALUES;
}
static float scale_get_to(SCALE *s) {
	if (!s) return 0;
	return s->sd->to;
}


static void scale_var_notify(SCALE *s, VARIABLE *v) {
	static int depth;
	if (depth == 0) {
		depth++;
		s->scale->set_value(s, strtod(v->var->get_string(v), (char **)NULL));
		depth--;
	}
	s->gen->update((WIDGET *)s, WID_UPDATE_REDRAW);
}


/*** CONNECT/DISCONNECT TO VARIABLE ***/
static void scale_set_var(SCALE *s, VARIABLE *v) {

	if (s->sd->var) {
		s->sd->var->gen->dec_ref((WIDGET *)s->sd->var);
		s->sd->var->var->disconnect(s->sd->var, s);
	}

	if (!v) return;

	s->sd->var = v;
	v->gen->inc_ref((WIDGET *)v);
	v->var->connect(v, s, scale_var_notify);
	s->scale->set_value(s, strtod(v->var->get_string(v), (char **)NULL));
}

static VARIABLE *scale_get_var(SCALE *s) {
	return s->sd->var;
}

static struct widget_methods gen_methods;
static struct scale_methods scale_methods = {
	scale_set_type,
	scale_get_type,
	scale_set_value,
	scale_get_value,
	scale_set_from,
	scale_get_from,
	scale_set_to,
	scale_get_to,
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
	curr_scale = b->gen->get_parent((WIDGET *)b);
	osx = b->gen->get_x((WIDGET *)b);
	osy = b->gen->get_y((WIDGET *)b);
	userstate->drag((WIDGET *)b, slider_motion_callback, NULL, slider_release_callback);
}


static void slider_bg_callback(BUTTON *b) {
	SCALE  *s;
	u16 dir=1;        /* default right/bottom direction */

	if (!b) return;
	s = (SCALE *)b->gen->get_parent((WIDGET *)b);
	if (!s) return;

	/* check if mouse is on the left or right side of the slider */
	if (s->sd->type & SCALE_VER) {
		if (input->get_my() < s->sd->slider->gen->get_abs_y(s->sd->slider)) dir=0;
	} else {
		if (input->get_mx() < s->sd->slider->gen->get_abs_x(s->sd->slider)) dir=0;
	}

	/* change view offset dependent on the scroll direction */
//  if (dir) {
//      s->view_offset += s->view_size;
//      if (s->view_offset > s->real_size - s->view_size)
//          s->view_offset = s->real_size - s->view_size;
//  } else {
//      s->view_offset -= s->view_size;
//      if (s->view_offset < 0) s->view_offset = 0;
//  }

	s->gen->update(s,WID_UPDATE_REDRAW);
}


static SCALE *create(void) {

	/* allocate memory for new widget */
	SCALE *new = (SCALE *)malloc(sizeof(struct scale)
	                           + sizeof(struct widget_data)
	                           + sizeof(struct scale_data));
	if (!new) {
		INFO(printf("Scale(create): out of memory\n"));
		return NULL;
	}
	new->gen   = &gen_methods;     /* pointer to general widget methods */
	new->scale = &scale_methods;   /* pointer to scale specific methods */
	new->wd    = (struct widget_data *) ((long)new + sizeof(struct scale));
	new->sd    = (struct scale_data *)  ((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);

	/* default scale attributes */
	new->sd->type  = 0;
	new->sd->value = 50;
	new->sd->from  = 0;
	new->sd->to    = 100;
	new->sd->padx  = 2;
	new->sd->pady  = 2;

	/* create scale element widgets */
	new->sd->slider    = new_button(NULL,slider_callback,0);
	new->sd->slider_bg = new_button(NULL,slider_bg_callback,0);
	new->sd->var       = NULL;

	new->sd->slider_bg->gen->set_state(new->sd->slider_bg,1);
	new->sd->slider->gen->set_parent(new->sd->slider,new);
	new->sd->slider_bg->gen->set_parent(new->sd->slider_bg,new);

	refresh_elements(new,WID_UPDATE_HIDDEN);
	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct scale_services services = {
	create
};


/************************/
/*** COMMAND LANGUAGE ***/
/************************/

static void scale_set_orient(SCALE *s, char *orient) {
	if (!strcmp("vertical",orient)) {
		s->scale->set_type(s,s->scale->get_type(s) | SCALE_VER);
		return;
	}
	if (!strcmp("horizontal",orient)) {
		s->scale->set_type(s,s->scale->get_type(s) & (-1^SCALE_VER));
		return;
	}
}

static char *scale_get_orient(SCALE *s) {
	if (s->scale->get_type(s) & SCALE_VER) return "vertical";
	else return "horizontal";
}

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Scale",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"Variable variable",scale_get_var,scale_set_var,gen_methods.update);
	script->reg_widget_attrib(widtype,"float value",scale_get_value,scale_set_value,gen_methods.update);
	script->reg_widget_attrib(widtype,"float from",scale_get_from,scale_set_from,gen_methods.update);
	script->reg_widget_attrib(widtype,"float to",scale_get_to,scale_set_to,gen_methods.update);
	script->reg_widget_attrib(widtype,"string orient",scale_get_orient,scale_set_orient,gen_methods.update);

	widman->build_script_lang(widtype,&gen_methods);
}


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_scale(struct dope_services *d) {

	gfx       = d->get_module("Gfx 1.0");
	but       = d->get_module("Button 1.0");
	input     = d->get_module("Input 1.0");
	script    = d->get_module("Script 1.0");
	widman    = d->get_module("WidgetManager 1.0");
	userstate = d->get_module("UserState 1.0");
	msg       = d->get_module("Messenger 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_scroll_update = gen_methods.update;
	gen_methods.draw   = &scale_draw;
	gen_methods.update = &scale_update;
	gen_methods.find   = &scale_find;

	build_script_lang();

	d->register_module("Scale 1.0",&services);
	return 1;
}
