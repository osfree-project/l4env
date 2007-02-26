/*
 * \brief  DOpE Frame widget module
 * \date   2002-11-13
 * \author Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct frame;
#define WIDGET struct frame

#include "dopestd.h"
#include "script.h"
#include "widget_data.h"
#include "widget.h"
#include "scrollbar.h"
#include "background.h"
#include "gfx.h"
#include "frame.h"
#include "widman.h"

static struct widman_services     *widman;
static struct scrollbar_services  *scroll;
static struct gfx_services        *gfx;
static struct background_services *bg;
static struct script_services     *script;

#define FRAME_MODE_SCRX 0x04    /* horizontal scrollbars */
#define FRAME_MODE_SCRY 0x08    /* vertical scrollbars */
#define FRAME_MODE_HIDX 0x10    /* auto-hiding of horizontal scrollbar */
#define FRAME_MODE_HIDY 0x20    /* auto-hiding of vertical scrollbar */


#define FRAME_UPDATE_SCRX        0x01
#define FRAME_UPDATE_SCRY        0x02
#define FRAME_UPDATE_NEW_CONTENT 0x04
#define FRAME_UPDATE_SCR_CONF    0x08

struct frame_data {
	WIDGET     *content;
	u32         mode;
	u16         bg_flag;
	s32         scroll_x,scroll_y;
	SCROLLBAR  *sb_x;
	SCROLLBAR  *sb_y;
	BACKGROUND *corner;
	u32         update_flags;
};

int init_frame(struct dope_services *d);



/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void frame_draw(FRAME *f,struct gfx_ds *ds,long x,long y) {
	WIDGET *cw;
	s32 vw,vh;

	if (!f) return;

	vw=f->wd->w;
	if (f->fd->sb_y) vw -= 13;

	vh=f->wd->h;
	if (f->fd->sb_x) vh -= 13;

	x += f->wd->x;
	y += f->wd->y;

	/* if content exists, draw it */
	gfx->push_clipping(ds,x,y,vw,vh);
	if (f->fd->bg_flag) {
		gfx->draw_box(ds,x,y,vw,vh,0x708090ff);
	}
	cw = f->fd->content;
	if (cw) cw->gen->draw(cw,ds,x,y);
	gfx->pop_clipping(ds);

	if (f->fd->corner) f->fd->corner->gen->draw((WIDGET *)f->fd->corner,ds,x,y);

	/* draw scrollbars */
	if (f->fd->sb_x) f->fd->sb_x->gen->draw((WIDGET *)f->fd->sb_x,ds,x,y);
	if (f->fd->sb_y) f->fd->sb_y->gen->draw((WIDGET *)f->fd->sb_y,ds,x,y);
}

static WIDGET *frame_find(FRAME *f,long x,long y) {
	WIDGET *result;
	WIDGET *cw;

	if (!f) return NULL;

	/* check if position is inside the frame */
	if ((x >= f->wd->x) && (y >= f->wd->y) &&
		(x < f->wd->x+f->wd->w) && (y < f->wd->y+f->wd->h)) {

		/* we are hit - lets check our children */
		if (f->fd->sb_x) {
			result = f->fd->sb_x->gen->find((WIDGET *)f->fd->sb_x, x-f->wd->x, y-f->wd->y);
			if (result) return result;
		}
		if (f->fd->sb_y) {
			result = f->fd->sb_y->gen->find((WIDGET *)f->fd->sb_y, x-f->wd->x, y-f->wd->y);
			if (result) return result;
		}

		cw=f->fd->content;
		if (cw) {
			result=cw->gen->find(cw, x-f->wd->x, y-f->wd->y);
			if (result) return result;
		}
		return f;
	}
	return NULL;
}


static void frame_update(FRAME *f,u16 redraw_flag) {
	SCROLLBAR *sb;
	s32 vw,vh;
	WIDGET *c=f->fd->content;

	vh=f->wd->h;
	if (f->fd->sb_x) vh -= 13;

	vw=f->wd->w;
	if (f->fd->sb_y) vw -= 13;

	if (f->fd->update_flags & FRAME_UPDATE_NEW_CONTENT) {
		if (c) {
			f->wd->min_w = 0;
			f->wd->max_w = c->gen->get_w(c);
			if (f->fd->sb_y) {
				f->wd->min_w += 13;
				f->wd->max_w += 13;
			}

			f->wd->min_h = 0;
			f->wd->max_h = c->gen->get_h(c);
			if (f->fd->sb_x) {
				f->wd->min_h += 13;
				f->wd->max_h += 13;
			}

			/* XXX propagate do_layout to parent? */

			c->gen->set_x(c,0);
			c->gen->set_y(c,0);
			c->gen->update(c,0);
		}
		f->fd->scroll_x  = 0;
		f->fd->scroll_y  = 0;
	}

	if ((f->fd->update_flags & (FRAME_UPDATE_SCRX | FRAME_UPDATE_SCR_CONF)) |
		(f->wd->update & WID_UPDATE_SIZE)) {

		if (f->fd->sb_x) {
			sb=f->fd->sb_x;
			sb->gen->set_x((WIDGET *)sb,0);
			sb->gen->set_y((WIDGET *)sb,vh);
			sb->gen->set_w((WIDGET *)sb,vw);
			sb->gen->set_h((WIDGET *)sb,13);
			sb->scroll->set_type(sb,SCROLLBAR_HOR);
			if (c) sb->scroll->set_real_size(sb,c->gen->get_w(c));
			sb->scroll->set_view_size(sb,vw);
			sb->scroll->set_view_offset(sb,f->fd->scroll_x);
			sb->gen->update((WIDGET *)sb,0);
		}
	}

	if ((f->fd->update_flags & (FRAME_UPDATE_SCRY | FRAME_UPDATE_SCR_CONF)) |
		(f->wd->update & WID_UPDATE_SIZE)) {

		if (f->fd->sb_y) {
			sb=f->fd->sb_y;
			sb->gen->set_x((WIDGET *)sb,vw);
			sb->gen->set_y((WIDGET *)sb,0);
			sb->gen->set_w((WIDGET *)sb,13);
			sb->gen->set_h((WIDGET *)sb,vh);
			sb->scroll->set_type(sb,SCROLLBAR_VER);
			if (c) sb->scroll->set_real_size(sb,c->gen->get_h(c));
			sb->scroll->set_view_size(sb,vh);
			sb->scroll->set_view_offset(sb,f->fd->scroll_y);
			sb->gen->update((WIDGET *)sb,0);
		}
	}

	if ((f->fd->update_flags & FRAME_UPDATE_SCRX) |
		(f->fd->update_flags & FRAME_UPDATE_SCRY) |
		(f->wd->update & WID_UPDATE_SIZE)) {

		if (f->fd->corner) {
			f->fd->corner->gen->set_x((WIDGET *)f->fd->corner,vw-1);
			f->fd->corner->gen->set_y((WIDGET *)f->fd->corner,vh-1);
		}
	}

	if (f->fd->update_flags & (FRAME_UPDATE_NEW_CONTENT | FRAME_UPDATE_SCR_CONF |
	                                  FRAME_UPDATE_SCRX | FRAME_UPDATE_SCRY)) {
		f->gen->force_redraw(f);
	}

	f->fd->update_flags=0;
}


/*** HANDLE CHILD-DEPENDENT LAYOUT CHANGE ***/
static u16 frame_do_layout(FRAME *f,WIDGET *c,u16 redraw_flag) {
	SCROLLBAR *sb;
	if (!f) return 0;
	if (f->fd->content != c) return 0;

	f->wd->min_w = 0;
	f->wd->max_w = c->gen->get_w(c);

	f->wd->min_h = 0;
	f->wd->max_h = c->gen->get_h(c);

	if (f->fd->sb_x) {
		sb=f->fd->sb_x;
		sb->scroll->set_real_size(sb, c->gen->get_w(c));
		sb->gen->update((WIDGET *)sb, 0);
		f->wd->min_h+=sb->scroll->get_arrow_size(sb);
		f->wd->max_h+=sb->scroll->get_arrow_size(sb);
	}

	if (f->fd->sb_y) {
		sb=f->fd->sb_y;
		sb->scroll->set_real_size(sb, c->gen->get_h(c));
		sb->gen->update((WIDGET *)sb, 0);
		f->wd->min_w += sb->scroll->get_arrow_size(sb);
		f->wd->max_w += sb->scroll->get_arrow_size(sb);
	}

	/* XXX check min/max of child and propagate do_layout to parent */

	if (redraw_flag) f->gen->force_redraw(f);

	/* we did the redrawing job already... a zero tells the calling */
	/* child that it doesnt need to redraw anything */
	return 0;
}


static char *frame_get_type(FRAME *f) {
	return "Frame";
}



/******************************/
/*** FRAME SPECIFIC METHODS ***/
/******************************/

static void frame_set_content(FRAME *f,WIDGET *new_content) {
	if (!f || !f->fd) return;
	if (f->fd->content) {
		f->fd->content->gen->set_parent(f->fd->content,NULL);
		f->fd->content->gen->dec_ref(f->fd->content);
	}
	f->fd->content=new_content;
	if (new_content) {
		new_content->gen->set_parent(new_content,f);
		new_content->gen->inc_ref(new_content);
		f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_NEW_CONTENT;
	}
}


static WIDGET *frame_get_content(FRAME *f) {
	if (!f || !f->fd) return NULL;
	return f->fd->content;
}


static void frame_set_background(FRAME *f,int bg_flag) {
	if (!f || !f->fd) return;
	f->fd->bg_flag = bg_flag;
}


static int frame_get_background(FRAME *f) {
	if (!f || !f->fd) return 0;
	return f->fd->bg_flag;
}


static void frame_xscroll_update(FRAME *f,u16 redraw_flag) {
	WIDGET *cw;
	s32 new_scroll_x=0;
	if (!f) return;
	if (f->fd->sb_x) {
		new_scroll_x = f->fd->sb_x->scroll->get_view_offset(f->fd->sb_x);
		if ((cw=f->fd->content)) cw->gen->set_x(cw,cw->gen->get_x(cw)
		                                  - new_scroll_x + f->fd->scroll_x);
		if (f->fd->sb_x) f->fd->scroll_x = new_scroll_x;
	}
	if (redraw_flag) f->gen->force_redraw(f);
}


static void frame_yscroll_update(FRAME *f,u16 redraw_flag) {
	WIDGET *cw;
	s32 new_scroll_y=0;
	if (!f || !f->fd) return;
	if (f->fd->sb_y) {
		new_scroll_y = f->fd->sb_y->scroll->get_view_offset(f->fd->sb_y);
		if ((cw=f->fd->content)) cw->gen->set_y(cw,cw->gen->get_y(cw)
		                                      - new_scroll_y + f->fd->scroll_y);
		if (f->fd->sb_y) f->fd->scroll_y = new_scroll_y;
	}
	if (redraw_flag) f->gen->force_redraw(f);
}


static void check_corner(FRAME *f) {
	if (f->fd->sb_x && f->fd->sb_y) {
		if (!f->fd->corner) f->fd->corner=bg->create();
		f->fd->corner->gen->set_w((WIDGET *)f->fd->corner,15);
		f->fd->corner->gen->set_h((WIDGET *)f->fd->corner,15);
	} else {
		if (f->fd->corner) {
			f->fd->corner->gen->dec_ref((WIDGET *)f->fd->corner);
			f->fd->corner=NULL;
		}
	}
}


static void frame_set_scrollx(FRAME *f,int flag) {
	if (flag) {
		f->fd->mode = f->fd->mode | FRAME_MODE_SCRX;
		if (!f->fd->sb_x) {
			f->fd->sb_x = scroll->create();
			f->fd->sb_x->gen->set_parent((WIDGET *)f->fd->sb_x,f);
			f->fd->sb_x->scroll->reg_scroll_update(f->fd->sb_x,(void (*)(void *,u16))frame_xscroll_update,f);
			f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	} else {
		f->fd->mode = f->fd->mode & (FRAME_MODE_SCRX^0xffffffff);
		if (f->fd->sb_x) {
			f->fd->sb_x->gen->dec_ref((WIDGET *)f->fd->sb_x);
			f->fd->sb_x=NULL;
			f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	}
	check_corner(f);
}


static int frame_get_scrollx(FRAME *f) {
	if (!f || !f->fd) return -1;
	if (f->fd->mode & FRAME_MODE_SCRX) return 1;
	else return 0;
}


static void frame_set_scrolly(FRAME *f,int flag) {
	if (flag) {
		f->fd->mode = f->fd->mode | FRAME_MODE_SCRY;
		if (!f->fd->sb_y) {
			f->fd->sb_y = scroll->create();
			f->fd->sb_y->gen->set_parent((WIDGET *)f->fd->sb_y,f);
			f->fd->sb_y->scroll->reg_scroll_update(f->fd->sb_y,(void (*)(void *,u16))frame_yscroll_update,f);
			f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	} else {
		f->fd->mode = f->fd->mode & (FRAME_MODE_SCRY^0xffffffff);
		if (f->fd->sb_y) {
			f->fd->sb_y->gen->dec_ref((WIDGET *)f->fd->sb_y);
			f->fd->sb_y=NULL;
			f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCR_CONF;
		}
	}
	check_corner(f);
}


static int frame_get_scrolly(FRAME *f) {
	if (!f || !f->fd) return -1;
	if (f->fd->mode & FRAME_MODE_SCRY) return 1;
	else return 0;
}


static void frame_set_xview(FRAME *f,s32 new_scroll_x) {
	WIDGET *cw;
	if (!f || !f->fd) return;
	if ((cw=f->fd->content)) cw->gen->set_x(cw,cw->gen->get_x(cw)
	                                      - new_scroll_x + f->fd->scroll_x);
	f->fd->scroll_x=new_scroll_x;
	f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCRX;
}


static s32 frame_get_xview(FRAME *f) {
	if (!f || !f->fd) return 0;
	return f->fd->scroll_x;
}


static void frame_set_yview(FRAME *f,s32 new_scroll_y) {
	WIDGET *cw;
	if (!f || !f->fd) return;
	if ((cw=f->fd->content)) cw->gen->set_y(cw,cw->gen->get_y(cw)
	                                      - new_scroll_y + f->fd->scroll_y);
	f->fd->scroll_y=new_scroll_y;
	f->fd->update_flags = f->fd->update_flags | FRAME_UPDATE_SCRY;
}


static s32 frame_get_yview(FRAME *f) {
	if (!f || !f->fd) return 0;
	return f->fd->scroll_y;
}



static struct widget_methods gen_methods;
static struct frame_methods  frame_methods = {
	frame_set_content,
	frame_get_content,
	frame_set_scrollx,
	frame_get_scrollx,
	frame_set_scrolly,
	frame_get_scrolly,
	frame_set_xview,
	frame_get_xview,
	frame_set_yview,
	frame_get_yview,
	frame_set_background,
	frame_get_background,
	frame_xscroll_update,
	frame_yscroll_update
};



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static FRAME *create(void) {

	/* allocate memory for new widget */
	FRAME *new = (FRAME *)malloc(sizeof(FRAME)
	                           + sizeof(struct widget_data)
	                           + sizeof(struct frame_data));
	if (!new) {
		INFO(printf("Frame(create): out of memory\n"));
		return NULL;
	}
	new->gen   = &gen_methods;           /* pointer to general widget methods */
	new->frame = &frame_methods;         /* pointer to frame specific methods */
	new->wd    = (struct widget_data *)((long)new     + sizeof(FRAME));
	new->fd    = (struct frame_data *) ((long)new->wd + sizeof(struct widget_data));

	/* set general widget attributes */
	widman->default_widget_data(new->wd);
	new->wd->min_w=0;
	new->wd->min_h=0;

	/* set frame specific default attributes */
	new->fd->content=NULL;
	new->fd->mode=0;
	new->fd->bg_flag=0;
	new->fd->scroll_x=0;
	new->fd->scroll_y=0;
	new->fd->sb_x = NULL;
	new->fd->sb_y = NULL;
	new->fd->corner=NULL;
	new->fd->update_flags=0;

	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct frame_services services = {
	create
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Frame",(void *(*)(void))create);

	script->reg_widget_attrib(widtype,"Widget content",frame_get_content,frame_set_content,frame_update);
	script->reg_widget_attrib(widtype,"boolean background",frame_get_background,frame_set_background,frame_update);
	script->reg_widget_attrib(widtype,"boolean scrollx",frame_get_scrollx,frame_set_scrollx,frame_update);
	script->reg_widget_attrib(widtype,"boolean scrolly",frame_get_scrolly,frame_set_scrolly,frame_update);
	script->reg_widget_attrib(widtype,"long xview",frame_get_xview,frame_set_xview,frame_update);
	script->reg_widget_attrib(widtype,"long yview",frame_get_yview,frame_set_yview,frame_update);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_frame(struct dope_services *d) {

	widman  = d->get_module("WidgetManager 1.0");
	scroll  = d->get_module("Scrollbar 1.0");
	bg      = d->get_module("Background 1.0");
	gfx     = d->get_module("Gfx 1.0");
	script  = d->get_module("Script 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	gen_methods.get_type  = frame_get_type;
	gen_methods.draw      = frame_draw;
	gen_methods.find      = frame_find;
	gen_methods.update    = frame_update;
	gen_methods.do_layout = frame_do_layout;

	build_script_lang();

	d->register_module("Frame 1.0",&services);
	return 1;
}
