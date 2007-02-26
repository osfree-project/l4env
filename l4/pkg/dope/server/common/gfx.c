/*
 * \brief  DOpE gfx module
 * \date   2003-03-31
 * \author Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module contains the implementation of the
 * graphics hardware abstraction interface. It
 * provides functions to manage the screens, the
 * allocation of data types, basic drawing functions
 * and mouse pointer handling.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "dopestd.h"
#include "gfx_handler.h"
#include "gfx.h"

static struct gfx_handler_services *gfxscr_rgb16;
static struct gfx_handler_services *gfximg_rgb16;
static struct gfx_handler_services *gfximg_rgba32;
static struct gfx_handler_services *gfximg_idx8;
static struct gfx_handler_services *gfximg_yuv420;

static struct gfx_ds_handler gfxscr_rgb16_handler;
static struct gfx_ds_handler gfximg_rgb16_handler;
static struct gfx_ds_handler gfximg_idx8_handler;
static struct gfx_ds_handler gfximg_rgba32_handler;
static struct gfx_ds_handler gfximg_yuv420_handler;

int init_gfx(struct dope_services *d);

/*************************/
/*** PRIVATE FUNCTIONS ***/
/*************************/


/*** DUMMY HANDLER FUNCTIONS USED AS DEFAULTS ***/
static s32   dummy_get_width(struct gfx_ds_data *ds) {return 0;}
static s32   dummy_get_height(struct gfx_ds_data *ds) {return 0;}
static s32   dummy_get_type(struct gfx_ds_data *ds) {return 0;}
static void  dummy_destroy(struct gfx_ds_data *ds) { }
static void *dummy_map(struct gfx_ds_data *ds) {return NULL;}
static void  dummy_unmap(struct gfx_ds_data *ds) { }
static void  dummy_update(struct gfx_ds_data *ds, s32 x, s32 y, s32 w, s32 h) { }
static s32   dummy_share(struct gfx_ds_data *ds, THREAD *dst_thread) {return -1;}
static s32   dummy_get_ident(struct gfx_ds_data *ds, char *dst_ident) {return -1;}
static s32   dummy_draw_hline(struct gfx_ds_data *ds, s16 x, s16 y, s16 w, u32 rgba) {return -1;}
static s32   dummy_draw_vline(struct gfx_ds_data *ds, s16 x, s16 y, s16 h, u32 rgba) {return -1;}
static s32   dummy_draw_fill(struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, u32 rgba) {return -1;}
static s32   dummy_draw_grad(struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, float dr, float dg, float db, float da, u32 rgba) {return -1;}
static s32   dummy_draw_idximg(struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, struct gfx_ds *idximg, struct gfx_ds *pal, u8 alpha) {return -1;}
static s32   dummy_draw_img(struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, struct gfx_ds *img, u8 alpha) {return -1;}
static s32   dummy_draw_string(struct gfx_ds_data *ds, s16 x, s16 y, s32 fg_rgba, u32 bg_rgba, s32 fnt_id, char *str) {return -1;}
static s32   dummy_draw_ansi(struct gfx_ds_data *ds, s16 x, s16 y, s32 fnt_id, char *str, u8 *bgfg) {return -1;}
static void  dummy_push_clipping(struct gfx_ds_data *ds, s32 x, s32 y, s32 w, s32 h) { }
static void  dummy_pop_clipping(struct gfx_ds_data *ds) {};
static void  dummy_reset_clipping(struct gfx_ds_data *ds) {};
static s32   dummy_get_clip_x(struct gfx_ds_data *ds) {return 0;}
static s32   dummy_get_clip_y(struct gfx_ds_data *ds) {return 0;}
static s32   dummy_get_clip_w(struct gfx_ds_data *ds) {return 0;}
static s32   dummy_get_clip_h(struct gfx_ds_data *ds) {return 0;}
static void  dummy_set_mouse_cursor(struct gfx_ds_data *ds, struct gfx_ds *cursor) { }
static void  dummy_set_mouse_pos(struct gfx_ds_data *ds, s32 x, s32 y) { }


/*** FILL HANDLER STRUCTURE WITH DEFAULT CALLBACKS ***/
static void set_handler_defaults(struct gfx_ds_handler *handler) {
	handler->get_width = dummy_get_width;
	handler->get_height = dummy_get_height;
	handler->get_type = dummy_get_type;
	handler->destroy = dummy_destroy;
	handler->map = dummy_map;
	handler->unmap = dummy_unmap;
	handler->update = dummy_update;
	handler->share = dummy_share;
	handler->get_ident = dummy_get_ident;
	handler->draw_hline = dummy_draw_hline;
	handler->draw_vline = dummy_draw_vline;
	handler->draw_fill = dummy_draw_fill;
	handler->draw_grad = dummy_draw_grad;
	handler->draw_idximg = dummy_draw_idximg;
	handler->draw_img = dummy_draw_img;
	handler->draw_string = dummy_draw_string;
	handler->draw_ansi = dummy_draw_ansi;
	handler->push_clipping = dummy_push_clipping;
	handler->pop_clipping = dummy_pop_clipping;
	handler->reset_clipping = dummy_reset_clipping;
	handler->get_clip_x = dummy_get_clip_x;
	handler->get_clip_y = dummy_get_clip_y;
	handler->get_clip_w = dummy_get_clip_w;
	handler->get_clip_h = dummy_get_clip_h;
	handler->set_mouse_cursor = dummy_set_mouse_cursor;
	handler->set_mouse_pos = dummy_set_mouse_pos;
}

/***************************/
/*** INTERFACE FUNCTIONS ***/
/***************************/


/*** ALLOCATE AND INITIALISE GFX DATASPACE FOR SCREEN ***/
static struct gfx_ds *alloc_scr(char *scrmode) {
	struct gfx_ds *new;

	new = malloc(sizeof(struct gfx_ds));
	if (!new) return NULL;

	new->handler = &gfxscr_rgb16_handler;
	new->data = gfxscr_rgb16->create(640,480);
	new->update_cnt = 0;
	return new;
}


/*** ALLOCATE GFX DATASPACE FOR IMAGES ***/
static struct gfx_ds *alloc_img(s16 w, s16 h, s32 img_type) {
	struct gfx_ds *new;

	INFO(printf("Gfx(alloc_img): w=%d, h=%d, type=%d\n",(int)w,(int)h,(int)img_type));
	new = (struct gfx_ds *)malloc(sizeof(struct gfx_ds));
	if (!new) return NULL;

	new->cache_idx = 0;
	new->update_cnt = 0;

	switch (img_type) {
	case GFX_IMG_TYPE_RGBA32:
		new->handler = &gfximg_rgba32_handler;
		new->data = gfximg_rgba32->create(w,h);
		break;
	case GFX_IMG_TYPE_RGB16:
		new->handler = &gfximg_rgb16_handler;
		new->data = gfximg_rgb16->create(w,h);
		break;
	case GFX_IMG_TYPE_INDEX8:
		new->handler = &gfximg_idx8_handler;
		new->data = gfximg_idx8->create(w,h);
		break;
	case GFX_IMG_TYPE_YUV420:
		printf("Gfx(alloc_img): yuv\n");
		new->handler = &gfximg_yuv420_handler;
		new->data = gfximg_yuv420->create(w,h);
		break;
	default:
		return NULL;
	}

	INFO(printf("Gfx(alloc_img): new->data=%x new->handler=%x\n",(int)new->data,(int)new->handler));
	new->update_cnt = 0;
	return new;
}

static struct gfx_ds *alloc_pal(s16 num_entries) {
	return alloc_img(num_entries,1,GFX_IMG_TYPE_RGBA32);
}

static s32 load_fnt(char *fntname) {
	return 0;
}


/*** FORWARD GENERIC FUNCTION CALLS TO SPECIFIC GFX DATASPACE TYPE HANDLER ***/
static s32 get_width(struct gfx_ds *ds) {
	return ds->handler->get_width(ds->data);
}

static s32 get_height(struct gfx_ds *ds) {
	return ds->handler->get_height(ds->data);
}

static s32 get_type(struct gfx_ds *ds) {
	return ds->handler->get_type(ds->data);
}

static void destroy(struct gfx_ds *ds) {
	ds->handler->destroy(ds->data);
	free(ds);
}

static void *map(struct gfx_ds *ds) {
	return ds->handler->map(ds->data);
}

static void unmap(struct gfx_ds *ds) {
	ds->handler->unmap(ds->data);
}

static void update(struct gfx_ds *ds, s32 x, s32 y, s32 w, s32 h) {
	ds->handler->update(ds->data, x, y, w, h);
	ds->update_cnt++;
}

static s32 get_upcnt(struct gfx_ds *ds) {
	return ds->update_cnt;
}

static s32 share(struct gfx_ds *ds, THREAD *dst_thread) {
	return ds->handler->share(ds->data, dst_thread);
}

static s32 get_ident(struct gfx_ds *ds, char *dst_ident) {
	return ds->handler->get_ident(ds->data, dst_ident);
}

static s32 draw_hline(struct gfx_ds *ds, s16 x, s16 y, s16 w, u32 rgba) {
	return ds->handler->draw_hline(ds->data, x, y, w, rgba);
}

static s32 draw_vline(struct gfx_ds *ds, s16 x, s16 y, s16 h, u32 rgba) {
	return ds->handler->draw_vline(ds->data, x, y, h, rgba);
}

static s32 draw_fill(struct gfx_ds *ds, s16 x, s16 y, s16 w, s16 h, u32 rgba) {
	return ds->handler->draw_fill(ds->data, x, y, w, h, rgba);
}

static s32 draw_grad(struct gfx_ds *ds, s16 x, s16 y, s16 w, s16 h,
                         float dr, float dg, float db, float da, u32 rgba) {
	return ds->handler->draw_grad(ds->data, x, y, w, h, dr, dg, db, da, rgba);
}

static s32 draw_idximg(struct gfx_ds *ds, s16 x, s16 y, s16 w, s16 h,
                       struct gfx_ds *idximg, struct gfx_ds *pal, u8 alpha) {
	return ds->handler->draw_idximg(ds->data, x, y, w, h, idximg, pal, alpha);
}

static s32 draw_img(struct gfx_ds *ds, s16 x, s16 y, s16 w, s16 h,
                    struct gfx_ds *img, u8 alpha) {
	return ds->handler->draw_img(ds->data, x, y, w, h, img, alpha);
}

static s32 draw_string(struct gfx_ds *ds, s16 x, s16 y, u32 fg_rgba, u32 bg_rgba,
                       s32 font_id, char *str) {
	return ds->handler->draw_string(ds->data, x, y , fg_rgba, bg_rgba, font_id, str);
}

static s32 draw_ansi(struct gfx_ds *ds, s16 x, s16 y, s32 font_id,
                     char *str, u8 *bgfg) {
	return ds->handler->draw_ansi(ds->data, x, y, font_id, str, bgfg);
}

static void push_clipping(struct gfx_ds *ds, s32 x, s32 y, s32 w, s32 h) {
	ds->handler->push_clipping(ds->data, x ,y ,w ,h);
}

static void pop_clipping(struct gfx_ds *ds) {
	ds->handler->pop_clipping(ds->data);
}

static void reset_clipping(struct gfx_ds *ds) {
	ds->handler->reset_clipping(ds->data);
}

static s32 get_clip_x(struct gfx_ds *ds) {
	return ds->handler->get_clip_x(ds->data);
}

static s32 get_clip_y(struct gfx_ds *ds) {
	return ds->handler->get_clip_x(ds->data);
}

static s32 get_clip_w(struct gfx_ds *ds) {
	return ds->handler->get_clip_w(ds->data);
}

static s32 get_clip_h(struct gfx_ds *ds) {
	return ds->handler->get_clip_h(ds->data);
}

static void set_mouse_cursor(struct gfx_ds *ds, struct gfx_ds *cursor) {
	ds->handler->set_mouse_cursor(ds->data, cursor);
}

static void set_mouse_pos(struct gfx_ds *ds, s32 x, s32 y) {
	ds->handler->set_mouse_pos(ds->data, x, y);
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct gfx_services services = {
	alloc_scr,     alloc_img,      alloc_pal,      load_fnt,
	get_width,     get_height,     get_type,
	destroy,       map,            unmap,          update,
	share,         get_ident,
	get_upcnt,
	draw_hline,    draw_vline,     draw_fill,      draw_grad,
	draw_idximg,   draw_img,       draw_string,    draw_ansi,
	push_clipping,
	pop_clipping,
	reset_clipping,
	get_clip_x,    get_clip_y,     get_clip_w,     get_clip_h,
	set_mouse_cursor, set_mouse_pos
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_gfx(struct dope_services *d) {

	gfxscr_rgb16  = d->get_module("GfxScreen16 1.0");
	gfximg_rgb16  = d->get_module("GfxImage16 1.0");
	gfximg_idx8   = d->get_module("GfxImage8i 1.0");
	gfximg_rgba32 = d->get_module("GfxImage32 1.0");
	gfximg_yuv420 = d->get_module("GfxImageYUV420 1.0");

	set_handler_defaults(&gfxscr_rgb16_handler);
	gfxscr_rgb16->register_gfx_handler(&gfxscr_rgb16_handler);

	set_handler_defaults(&gfximg_rgba32_handler);
	gfximg_rgba32->register_gfx_handler(&gfximg_rgba32_handler);

	set_handler_defaults(&gfximg_rgb16_handler);
	gfximg_rgb16->register_gfx_handler(&gfximg_rgb16_handler);

	set_handler_defaults(&gfximg_idx8_handler);
	gfximg_idx8->register_gfx_handler(&gfximg_idx8_handler);

	set_handler_defaults(&gfximg_yuv420_handler);
	gfximg_yuv420->register_gfx_handler(&gfximg_yuv420_handler);

	d->register_module("Gfx 1.0",&services);
	return 1;
}
