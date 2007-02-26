/*
 * \brief   Interface of handler of gfx container type
 * \date    2003-04-01
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


#if !defined(THREAD)
#define THREAD void 
#endif

struct gfx_ds_data;
struct gfx_ds_handler;

struct gfx_handler_services {
	struct gfx_ds_data *(*create) (s32 width, s32 height);
	s32 (*register_gfx_handler) (struct gfx_ds_handler *handler);
};

struct gfx_ds {
	struct gfx_ds_handler *handler;   /* ds type dependent handler callbacks */
	struct gfx_ds_data    *data;      /* ds type dependent data */
	s32 update_cnt;                          /* update-counter (used for ds caching) */
	s32 cache_idx;
};

struct gfx_ds_handler {

	s32 (*get_width)  (struct gfx_ds_data *ds);
	s32 (*get_height) (struct gfx_ds_data *ds);
	s32 (*get_type)   (struct gfx_ds_data *ds);
	
	void  (*destroy)(struct gfx_ds_data *ds);
	void *(*map)    (struct gfx_ds_data *ds);
	void  (*unmap)  (struct gfx_ds_data *ds);
	void  (*update) (struct gfx_ds_data *ds, s32 x, s32 y, s32 w, s32 h);
	s32   (*share)  (struct gfx_ds_data *ds, THREAD *dst_thread);
	s32   (*get_ident) (struct gfx_ds_data *ds, char *dst_ident);
	
	s32 (*draw_hline)  (struct gfx_ds_data *ds, s16 x, s16 y, s16 w, u32 rgba);
	s32 (*draw_vline)  (struct gfx_ds_data *ds, s16 x, s16 y, s16 h, u32 rgba);
	s32 (*draw_fill)   (struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, u32 rgba);
	s32 (*draw_grad)   (struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, float dr, float dg, float db, float da, u32 rgba);
	s32 (*draw_idximg) (struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, struct gfx_ds *idximg, struct gfx_ds *pal, u8 alpha);
	s32 (*draw_img)    (struct gfx_ds_data *ds, s16 x, s16 y, s16 w, s16 h, struct gfx_ds *img, u8 alpha);
	s32 (*draw_string) (struct gfx_ds_data *ds, s16 x, s16 y, s32 fg_rgba, u32 bg_rgba, s32 fnt_id, char *str);
	s32 (*draw_ansi)   (struct gfx_ds_data *ds, s16 x, s16 y, s32 fnt_id, char *str, u8 *bgfg);
	
	void (*push_clipping) (struct gfx_ds_data *ds, s32 x, s32 y, s32 w, s32 h);
	void (*pop_clipping)   (struct gfx_ds_data *ds);
	void (*reset_clipping) (struct gfx_ds_data *ds);

	s32 (*get_clip_x) (struct gfx_ds_data *ds);
	s32 (*get_clip_y) (struct gfx_ds_data *ds);
	s32 (*get_clip_w) (struct gfx_ds_data *ds);
	s32 (*get_clip_h) (struct gfx_ds_data *ds);
	
	void (*set_mouse_cursor) (struct gfx_ds_data *ds, struct gfx_ds *cursor);
	void (*set_mouse_pos)    (struct gfx_ds_data *ds, s32 x, s32 y);
};
