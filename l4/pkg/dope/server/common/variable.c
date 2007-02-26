/*
 * \brief   DOpE Variable widget module
 * \date    2002-05-15
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

struct variable;
#define WIDGET struct variable

#include "dopestd.h"
#include "gfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "variable.h"
#include "fontman.h"
#include "script.h"
#include "widman.h"

static struct widman_services  *widman;
static struct gfx_services     *gfx;
static struct fontman_services *font;
static struct script_services  *script;

#define VAR_UPDATE_VALUE 0x01

#define VARIABLE_ALIGN_LEFT   0x01
#define VARIABLE_ALIGN_RIGHT  0x02
#define VARIABLE_ALIGN_TOP    0x04
#define VARIABLE_ALIGN_BOTTOM 0x08

struct variable_connection;
struct variable_connection {
	WIDGET *widget;
	void (*notify)(WIDGET *w, VARIABLE *v);
	struct variable_connection *next;
};

struct variable_data {
	long   update_flags;
	char  *text;
	s16    font_id;
	s16    flags;
	struct variable_connection *connections;
};

int init_variable(struct dope_services *d);

#define BLACK_SOLID GFX_RGBA(0,0,0,255)

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

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
		result = malloc(strl+2);
		if (!result) return NULL;
		d=result;
		while (*s) *(d++)=*(s++);
		*d=0;
		return result;
	}
	return NULL;
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void var_draw(VARIABLE *v,struct gfx_ds *ds,long x,long y) {
	int tx = x + v->wd->x + 2;
	int ty = y + v->wd->y + 2;

//  gfx->push_clipping(ds,x+2,y+2,l->wd->w-4,l->wd->h-4);
	if (v->vd->text) {
		gfx->draw_string(ds,tx,ty,BLACK_SOLID,0,v->vd->font_id,v->vd->text);
	} else {
		gfx->draw_string(ds,tx,ty,BLACK_SOLID,0,v->vd->font_id,"<no value>");
	}
//  gfx->pop_clipping(ds);
}

static void (*orig_update) (VARIABLE *v,u16 redraw_flag);

static void var_update(VARIABLE *v,u16 redraw_flag) {
	s16 newlayout_needed = 0;
	s16 redraw_done = 0;
	WIDGET *parent;

	if ((!v) || (!v->vd)) return;
	if (v->vd->update_flags & VAR_UPDATE_VALUE) {
		v->wd->update |= WID_UPDATE_SIZE;
	}
	if (v->wd->update & (WID_UPDATE_POS|WID_UPDATE_SIZE)) {
		char *txt = "<no_value>";
		if (v->vd->text) txt = v->vd->text;

		v->wd->min_w = v->wd->max_w = font->calc_str_width (v->vd->font_id,txt) + 4;
		v->wd->min_h = v->wd->max_h = font->calc_str_height(v->vd->font_id,txt) + 4;
		newlayout_needed = 1;
	}

	orig_update(v,0);
	v->vd->update_flags=0;

	if (newlayout_needed) {
		parent = v->gen->get_parent(v);
		if (parent) {
			if (parent->gen->do_layout(parent,v,1) == 0) redraw_done = 1;
		}
	}
	if (redraw_flag && !redraw_done) v->gen->force_redraw(v);
}



/*******************************/
/*** VARIABLE SPECIFIC METHODS ***/
/*******************************/

static void var_set_value(VARIABLE *v,char *new_txt) {
	struct variable_connection *cc;

	if ((!v) || (!v->vd)) return;
	if (v->vd->text) free(v->vd->text);
	v->vd->text = strdup(new_txt);
	v->vd->update_flags = v->vd->update_flags | VAR_UPDATE_VALUE;

	/* notify all connected widgets */
	cc = v->vd->connections;
	while (cc) {
		cc->notify(cc->widget, v);
		cc = cc->next;
	}
}


static char *var_get_value(VARIABLE *v) {
	if ((!v) || (!v->vd) || (!v->vd->text)) return "<undefined>";
	return v->vd->text;
}


static void var_connect(VARIABLE *v, WIDGET *w, void (*notify)(WIDGET *w, VARIABLE *v)) {
	struct variable_connection *new;

	if (!w || !v || !notify) return;

	new = (struct variable_connection *)malloc(sizeof(struct variable_connection));
	new->widget = w;
	new->notify = notify;
	new->next = v->vd->connections;
	v->vd->connections = new;
	w->gen->inc_ref(w);
}

static void var_disconnect(VARIABLE *v, WIDGET *w) {

}

static struct widget_methods gen_methods;
static struct variable_methods var_methods = {
	var_set_value,
	var_get_value,
	var_connect,
	var_disconnect,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static VARIABLE *create(void) {

	/* allocate memory for new widget */
	VARIABLE *new = (VARIABLE *)malloc(sizeof(struct variable)
	                                 + sizeof(struct widget_data)
	                                 + sizeof(struct variable_data));
	if (!new) {
		INFO(printf("Variable(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;    /* pointer to general widget methods */
	new->var = &var_methods;    /* pointer to variable specific methods */
	new->wd = (struct widget_data *)  ((long)new + sizeof(struct variable));
	new->vd = (struct variable_data *)((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);

	/* set variable specific attributes */
	new->vd->text  = NULL;
	new->vd->flags = 0;
	new->vd->font_id = 1;
	new->vd->connections = NULL;

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct variable_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Variable",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"string value",var_get_value,var_set_value,gen_methods.update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_variable(struct dope_services *d) {
	widman  = d->get_module("WidgetManager 1.0");
	gfx     = d->get_module("Gfx 1.0");
	font    = d->get_module("FontManager 1.0");
	script  = d->get_module("Script 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;

	gen_methods.draw=var_draw;
	gen_methods.update=var_update;

	build_script_lang();

	d->register_module("Variable 1.0",&services);
	return 1;
}
