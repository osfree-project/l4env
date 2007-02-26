/*
 * \brief   DOpE Button widget module
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

struct button;
#define WIDGET struct button

#include "dopestd.h"
#include "gfx.h"
#include "widget_data.h"
#include "event.h"
#include "widget.h"
#include "button.h"
#include "fontman.h"
#include "script.h"
#include "widman.h"
#include "userstate.h"
#include "messenger.h"

static struct widman_services    *widman;
static struct gfx_services       *gfx;
static struct fontman_services   *font;
static struct script_services    *script;
static struct userstate_services *userstate;
static struct messenger_services *msg;

#define BUTTON_UPDATE_TEXT 0x01

struct button_data {
	long   update_flags;
	char  *text;
	s16    style;
	s16    font_id;
	s16    tx,ty;                    /* text position inside the button */
	void (*click)  (WIDGET *);
	void (*release)(WIDGET *);
};

static GFX_CONTAINER *range_img;

static GFX_CONTAINER *normal_bg_pal;
static GFX_CONTAINER *focus_bg_pal;
static GFX_CONTAINER *actwin_bg_pal;

int init_button(struct dope_services *d);

#define BLACK_SOLID GFX_RGBA(0,0,0,255)
#define BLACK_MIXED GFX_RGBA(0,0,0,127)
#define WHITE_SOLID GFX_RGBA(255,255,255,255)
#define WHITE_MIXED GFX_RGBA(255,255,255,127)

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


static void update_text_pos(BUTTON *b) {
	if ((!b) || (!b->bd) || (!b->bd->text)) return;
	b->bd->tx = (b->wd->w - font->calc_str_width (b->bd->font_id,b->bd->text))>>1;
	b->bd->ty = (b->wd->h - font->calc_str_height(b->bd->font_id,b->bd->text))>>1;
}


static inline void draw_focus_frame(GFX_CONTAINER *d,s32 x, s32 y, s32 w, s32 h) {
	gfx->draw_hline(d,x,y,w,WHITE_MIXED);
	gfx->draw_vline(d,x,y,h,WHITE_MIXED);
	gfx->draw_hline(d,x,y+h-1,w,BLACK_SOLID);
	gfx->draw_vline(d,x+w-1,y,h,BLACK_SOLID);
}

static inline void draw_raised_frame(GFX_CONTAINER *d,s32 x, s32 y, s32 w, s32 h) {

	/* outer frame */
	gfx->draw_hline(d,x,y,w,WHITE_MIXED);
	gfx->draw_vline(d,x,y,h,WHITE_MIXED);
	gfx->draw_hline(d,x,y+h-1,w,BLACK_SOLID);
	gfx->draw_vline(d,x+w-1,y,h,BLACK_SOLID);

	/* inner frame */
	gfx->draw_hline(d,x+1,y+1,w-2,WHITE_MIXED);
	gfx->draw_vline(d,x+1,y+1,h-2,WHITE_MIXED);
	gfx->draw_hline(d,x+1,y+1,w-2,WHITE_MIXED);
	gfx->draw_vline(d,x+1,y+1,h-2,WHITE_MIXED);
	gfx->draw_hline(d,x+1,y+h-2,w-2,BLACK_MIXED);
	gfx->draw_vline(d,x+w-2,y+1,h-2,BLACK_MIXED);

	/* spot */
	gfx->draw_hline(d,x+1,y+1,1,WHITE_SOLID);
	gfx->draw_vline(d,x+1,y+1,1,WHITE_SOLID);
}

static inline void draw_pressed_frame(GFX_CONTAINER *d,s32 x, s32 y, s32 w, s32 h) {

	/* outer frame */
	gfx->draw_hline(d,x,y,w,BLACK_SOLID);
	gfx->draw_vline(d,x,y,h,BLACK_SOLID);
	gfx->draw_hline(d,x,y+h-1,w,WHITE_MIXED);
	gfx->draw_vline(d,x+w-1,y,h,WHITE_MIXED);

	/* inner frame */
	gfx->draw_hline(d,x+1,y+1,w-2,BLACK_SOLID);
	gfx->draw_vline(d,x+1,y+1,h-2,BLACK_SOLID);
	gfx->draw_hline(d,x+1,y+h-2,w-2,WHITE_SOLID);
	gfx->draw_vline(d,x+w-2,y+1,h-2,WHITE_SOLID);
}

/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void but_draw(BUTTON *b,struct gfx_ds *ds,long x,long y) {
	long tx=b->bd->tx,ty=b->bd->ty;

	x+=b->wd->x;
	y+=b->wd->y;

	gfx->push_clipping(ds,x,y,b->wd->w,b->wd->h);

	if (b->wd->flags & WID_FLAGS_FOCUS) {
		gfx->draw_idximg(ds,x,y,b->wd->w,b->wd->h,range_img,focus_bg_pal,255);

		if (!(b->wd->flags & WID_FLAGS_STATE)) {
			gfx->draw_idximg(ds,x,y,b->wd->w,b->wd->h,range_img,focus_bg_pal,255);
			draw_focus_frame(ds,x,y,b->wd->w,b->wd->h);
		}
	} else {
		if (b->bd->style == 2) {
			gfx->draw_idximg(ds,x,y,b->wd->w,b->wd->h,range_img,actwin_bg_pal,255);
		} else {
			gfx->draw_idximg(ds,x,y,b->wd->w,b->wd->h,range_img,normal_bg_pal,255);
		}

	}

	if (b->wd->flags & WID_FLAGS_STATE) {
		draw_pressed_frame(ds,x,y,b->wd->w,b->wd->h);
	} else {
		if (!(b->wd->flags & WID_FLAGS_FOCUS)) {
			draw_raised_frame(ds,x,y,b->wd->w,b->wd->h);
		}
	}

	tx+=x;ty+=y;
	if (b->wd->flags & WID_FLAGS_FOCUS) {
		tx+=1;ty+=1;
	}
	if (b->wd->flags & WID_FLAGS_STATE) {
		tx+=2;ty+=2;
	}
	gfx->push_clipping(ds,x+2,y+2,b->wd->w-4,b->wd->h-4);

	if (b->bd->text) {
		switch (b->bd->style) {
			case 1:
				gfx->draw_string(ds,tx,ty,BLACK_SOLID,0,b->bd->font_id,b->bd->text);
				break;
			default:
				gfx->draw_string(ds,tx,ty,WHITE_SOLID,0,b->bd->font_id,b->bd->text);
				break;
		}
	}

	gfx->pop_clipping(ds);
	gfx->pop_clipping(ds);
}

static void but_untouch_callback(BUTTON *b, int dx, int dy) {
	u8 *clack_msg;
	
	if (!b->gen->get_state(b)) return;
	
	clack_msg = b->gen->get_bind_msg(b,"clack");
	if (clack_msg) msg->send_action_event(b->gen->get_app_id(b),"clack",clack_msg);
}

void (*orig_handle_event) (BUTTON *b,EVENT *e);

static void but_handle_event(BUTTON *b,EVENT *e) {
	int propagate=1;
	u8 *click_msg, *clack_msg;
	switch (e->type) {
	case EVENT_PRESS:
		if (b->bd->click) {
			b->bd->click(b);
			propagate = 0;
		}
		click_msg = b->gen->get_bind_msg(b,"click");
		clack_msg = b->gen->get_bind_msg(b,"clack");
		
		if (click_msg || clack_msg) {
			userstate->touch(b, NULL, but_untouch_callback);
			propagate = 0;
		}
		if (click_msg) {
			msg->send_action_event(b->gen->get_app_id(b),"click",click_msg);
		}
		break;
	case EVENT_RELEASE:
		if (b->bd->release) {
			b->bd->release(b);
			propagate = 0;
		}
		break;
	}

	if (propagate) orig_handle_event(b,e);
}


static void (*orig_update) (BUTTON *b,u16 redraw_flag);

static void but_update(BUTTON *b,u16 redraw_flag) {
	s16 redraw_needed = 0;

	if ((!b) || (!b->bd)) return;
	if (b->wd->update & (WID_UPDATE_FOCUS|WID_UPDATE_STATE)) redraw_needed = 1;
	if (b->bd->update_flags & BUTTON_UPDATE_TEXT) {
		update_text_pos(b);
		redraw_needed = 1;
	}
	if (b->wd->update & (WID_UPDATE_POS|WID_UPDATE_SIZE)) {
		update_text_pos(b);
		redraw_needed = 0;
	}

	if (redraw_flag && redraw_needed) b->gen->force_redraw(b);
	orig_update(b,redraw_flag);
	b->bd->update_flags=0;
}



/*******************************/
/*** BUTTON SPECIFIC METHODS ***/
/*******************************/

static void but_set_text(BUTTON *b,char *new_txt) {
	if ((!b) || (!b->bd)) return;
	if (b->bd->text) free(b->bd->text);
	b->bd->text = strdup(new_txt);
	b->bd->update_flags = b->bd->update_flags | BUTTON_UPDATE_TEXT;
}


static char *but_get_text(BUTTON *b) {
	if ((!b) || (!b->bd)) return 0;
	return b->bd->text;
}


static void but_set_font(BUTTON *b,s32 font_id) {
	if ((!b) || (!b->bd)) return;
	b->bd->font_id=font_id;
	b->bd->update_flags = b->bd->update_flags | BUTTON_UPDATE_TEXT;
}


static s32 but_get_font(BUTTON *b) {
	if ((!b) || (!b->bd)) return 0;
	return b->bd->font_id;
}


static void but_set_style(BUTTON *b,s32 style) {
	if ((!b) || (!b->bd)) return;
	b->bd->style=style;
	b->bd->update_flags = b->bd->update_flags | BUTTON_UPDATE_TEXT;
}


static s32 but_get_style(BUTTON *b) {
	if ((!b) || (!b->bd)) return 0;
	return b->bd->font_id;
}


static void but_set_click(BUTTON *b,void (*callback)(BUTTON *b)) {
	b->bd->click=callback;
}


static void but_set_release(BUTTON *b,void (*callback)(BUTTON *b)) {
	b->bd->release=callback;
}


static struct widget_methods gen_methods;
static struct button_methods but_methods={
	but_set_text,
	but_get_text,
	but_set_font,
	but_get_font,
	but_set_style,
	but_get_style,
	but_set_click,
	but_set_release
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static BUTTON *create(void) {

	/* allocate memory for new widget */
	BUTTON *new = (BUTTON *)malloc(sizeof(struct button)
	                             + sizeof(struct widget_data)
	                             + sizeof(struct button_data));
	if (!new) {
		INFO(printf("Button(create): out of memory\n"));
		return NULL;
	}
	new->gen = &gen_methods;    /* pointer to general widget methods */
	new->but = &but_methods;    /* pointer to button specific methods */
	new->wd = (struct widget_data *)((long)new + sizeof(struct button));
	new->bd = (struct button_data *)((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);

	/* set button specific attributes */
	new->bd->text = NULL;
	new->bd->font_id = 0;
	new->bd->click = NULL;
	new->bd->release = NULL;
	new->bd->style = 1;
	new->bd->tx = 0;
	new->bd->ty = 0;
	new->bd->update_flags = 0;
	update_text_pos(new);

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct button_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Button",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"string text",but_get_text,but_set_text,gen_methods.update);
	script->reg_widget_attrib(widtype,"long font",but_get_font,but_set_font,gen_methods.update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_button(struct dope_services *d) {
	s32 i,j;
	u32 *c;
	u8  *e;

	widman    = d->get_module("WidgetManager 1.0");
	gfx       = d->get_module("Gfx 1.0");
	font      = d->get_module("FontManager 1.0");
	script    = d->get_module("Script 1.0");
	userstate = d->get_module("UserState 1.0");
	msg       = d->get_module("Messenger 1.0");

	range_img  = gfx->alloc_img(100,100,GFX_IMG_TYPE_INDEX8);

	normal_bg_pal = gfx->alloc_pal(256);
	actwin_bg_pal = gfx->alloc_pal(256);
	focus_bg_pal  = gfx->alloc_pal(256);

	c = gfx->map(normal_bg_pal);
	for (i=0;i<256;i++) *(c++) = GFX_RGB(i/2+90,i/2+90,i/2+110);
	c = gfx->map(focus_bg_pal);
	for (i=0;i<256;i++) *(c++) = GFX_RGB(i/2+100,i/2+100,i/2+140);
	c = gfx->map(actwin_bg_pal);
	for (i=0;i<256;i++) *(c++) = GFX_RGB(i/2+110,i/2+100,i/3+90);

	e = gfx->map(range_img);
	for (j=0;j<100;j++) {
		for (i=0;i<100;i++) {
			*(e++) = i+j;
		}
	}

	gfx->unmap(range_img);
	gfx->unmap(normal_bg_pal);
	gfx->unmap(actwin_bg_pal);
	gfx->unmap(focus_bg_pal);

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;
	orig_handle_event=gen_methods.handle_event;

	gen_methods.draw=but_draw;
	gen_methods.update=but_update;
	gen_methods.handle_event=but_handle_event;

	build_script_lang();


	d->register_module("Button 1.0",&services);
	return 1;
}
