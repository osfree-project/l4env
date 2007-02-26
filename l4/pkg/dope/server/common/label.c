/*
 * \brief   DOpE Label widget module
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

struct label;
#define WIDGET struct label

#include "dopestd.h"
#include "gfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "variable.h"
#include "label.h"
#include "fontman.h"
#include "script.h"
#include "widman.h"

static struct widman_services  *widman;
static struct gfx_services     *gfx;
static struct fontman_services *font;
static struct script_services  *script;

#define LABEL_UPDATE_TEXT 0x01

#define LABEL_ALIGN_LEFT   0x01
#define LABEL_ALIGN_RIGHT  0x02
#define LABEL_ALIGN_TOP    0x04
#define LABEL_ALIGN_BOTTOM 0x08

struct label_data {
	long      update_flags;
	char     *text;
	s16       font_id;
	s16      tx,ty;                    /* text position inside the label cell */
	s16       flags;
	VARIABLE *var;
};

int init_label(struct dope_services *d);

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


static void update_text_pos(LABEL *l) {
	if ((!l) || (!l->ld) || (!l->ld->text)) return;
	l->ld->tx = (l->wd->w - font->calc_str_width (l->ld->font_id,l->ld->text))>>1;
	l->ld->ty = (l->wd->h - font->calc_str_height(l->ld->font_id,l->ld->text))>>1;
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void lab_draw(LABEL *l,struct gfx_ds *ds,long x,long y) {
	int tx = x + l->wd->x + l->ld->tx;
	int ty = y + l->wd->y + l->ld->ty;

//  gfx->push_clipping(ds,x+2,y+2,l->wd->w-4,l->wd->h-4);
	if (l->ld->text) {
		gfx->draw_string(ds,tx,ty,BLACK_SOLID,0,l->ld->font_id,l->ld->text);
	}
//  gfx->pop_clipping(ds);
}

static void (*orig_update) (LABEL *l,u16 redraw_flag);

static void lab_update(LABEL *l,u16 redraw_flag) {
	s16 redraw_needed = 0;

	if ((!l) || (!l->ld)) return;
	if (l->wd->update & (WID_UPDATE_FOCUS|WID_UPDATE_STATE)) redraw_needed = 1;
	if (l->ld->update_flags & LABEL_UPDATE_TEXT) {
		update_text_pos(l);
		redraw_needed = 1;
	}
	if (l->wd->update & (WID_UPDATE_POS|WID_UPDATE_SIZE)) {
		update_text_pos(l);
		redraw_needed = 0;
	}

	if (redraw_flag && redraw_needed) l->gen->force_redraw(l);
	orig_update(l,redraw_flag);
	l->ld->update_flags=0;
}



/*******************************/
/*** LABEL SPECIFIC METHODS ***/
/*******************************/

static void lab_set_text(LABEL *l,char *new_txt) {
	if ((!l) || (!l->ld)) return;
	if (l->ld->text) free(l->ld->text);
	l->ld->text = strdup(new_txt);
	l->ld->update_flags = l->ld->update_flags | LABEL_UPDATE_TEXT;
}


static char *lab_get_text(LABEL *l) {
	if ((!l) || (!l->ld)) return 0;
	return l->ld->text;
}


static void lab_set_font(LABEL *l,s32 font_id) {
	if ((!l) || (!l->ld)) return;
	l->ld->font_id=font_id;
	l->ld->update_flags = l->ld->update_flags | LABEL_UPDATE_TEXT;
}


static s32 lab_get_font(LABEL *l) {
	if ((!l) || (!l->ld)) return 0;
	return l->ld->font_id;
}


static void lab_set_align(LABEL *l,char *align) {
	if ((!l) || (!l->ld) || (!align)) return;
	l->ld->flags = 0;
	while (*align) {
		switch (*(align++)) {
		case 'l': l->ld->flags |= LABEL_ALIGN_LEFT;   break;
		case 'r': l->ld->flags |= LABEL_ALIGN_RIGHT;  break;
		case 't': l->ld->flags |= LABEL_ALIGN_TOP;    break;
		case 'b': l->ld->flags |= LABEL_ALIGN_BOTTOM; break;
		}
	}
	l->ld->update_flags = l->ld->update_flags | LABEL_UPDATE_TEXT;
}


static char *lab_get_align(LABEL *l) {
	static char alignstr[8];
	char  *d = &alignstr[0];

	if ((!l) || (!l->ld)) return 0;

	if (l->ld->flags & LABEL_ALIGN_LEFT)   *(d++) = 'l';
	if (l->ld->flags & LABEL_ALIGN_RIGHT)  *(d++) = 'r';
	if (l->ld->flags & LABEL_ALIGN_TOP)    *(d++) = 't';
	if (l->ld->flags & LABEL_ALIGN_BOTTOM) *(d++) = 'b';
	*(d++) = 0;

	return &alignstr[0];
}


static void lab_var_notify(LABEL *l, VARIABLE *v) {
	lab_set_text(l, v->var->get_string(v));
	lab_update(l, WID_UPDATE_REDRAW);
}

static void lab_set_var(LABEL *l, VARIABLE *v) {
	if (l->ld->var) {
		l->ld->var->gen->dec_ref((WIDGET *)l->ld->var);
		l->ld->var->var->disconnect(l->ld->var, l);
	}

	l->ld->var = v;
	if (v) {
		v->gen->inc_ref((WIDGET *)v);
		v->var->connect(v, l, lab_var_notify);
		lab_set_text(l, v->var->get_string(v));
	} else {
		lab_set_text(l,"");
	}
}

static VARIABLE *lab_get_var(LABEL *l) {
	return l->ld->var;
}

static struct widget_methods gen_methods;
static struct label_methods lab_methods={
	lab_set_text,
	lab_get_text,
	lab_set_font,
	lab_get_font,
	lab_set_align,
	lab_get_align,
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static LABEL *create(void) {

	/* allocate memory for new widget */
	LABEL *new = (LABEL *)malloc(sizeof(struct label)
	                           + sizeof(struct widget_data)
	                           + sizeof(struct label_data));
	if (!new) {
		INFO(printf("Label(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;    /* pointer to general widget methods */
	new->lab = &lab_methods;    /* pointer to label specific methods */
	new->wd = (struct widget_data *)((long)new + sizeof(struct label));
	new->ld = (struct label_data *)((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);

	/* set label specific attributes */
	new->ld->text = NULL;
	new->ld->font_id = 0;
	new->ld->tx=0;
	new->ld->ty=0;
	new->ld->flags=0;
	update_text_pos(new);

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct label_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Label",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"string text",lab_get_text,lab_set_text,gen_methods.update);
	script->reg_widget_attrib(widtype,"string align",lab_get_align,lab_set_align,gen_methods.update);
	script->reg_widget_attrib(widtype,"long font",lab_get_font,lab_set_font,gen_methods.update);
	script->reg_widget_attrib(widtype,"Variable variable",lab_get_var,lab_set_var,gen_methods.update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_label(struct dope_services *d) {
	widman  = d->get_module("WidgetManager 1.0");
	gfx     = d->get_module("Gfx 1.0");
	font    = d->get_module("FontManager 1.0");
	script  = d->get_module("Script 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;

	gen_methods.draw=lab_draw;
	gen_methods.update=lab_update;

	build_script_lang();

	d->register_module("Label 1.0",&services);
	return 1;
}
