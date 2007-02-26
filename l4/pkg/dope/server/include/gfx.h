/*
 * \brief   Interface of DOpE gfx layer
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

#define GFX_IMG_TYPE_RGB16    1
#define GFX_IMG_TYPE_RGBA32   2
#define GFX_IMG_TYPE_INDEX8   3
#define GFX_IMG_TYPE_YUV420   4

#define GFX_RGBA(r,g,b,a) (((r)<<24)|((g)<<16)|((b)<<8)|(a))
#define GFX_RGB(r,g,b) (((r)<<24)|((g)<<16)|((b)<<8)|255)

#define GFX_ALPHA(rgba) (rgba&255)

struct gfx_ds;

#define GFX_CONTAINER struct gfx_ds

#if !defined(THREAD)
#define THREAD void 
#endif

struct gfx_services {
	GFX_CONTAINER *(*alloc_scr) (char *scrmode);
	GFX_CONTAINER *(*alloc_img) (s16 w, s16 h, s32 img_type);
	GFX_CONTAINER *(*alloc_pal) (s16 num_entries);
	
	s32 (*load_fnt) (char *fntname);
	
	s32   (*get_width)  (GFX_CONTAINER *);
	s32   (*get_height) (GFX_CONTAINER *);
	s32   (*get_type)   (GFX_CONTAINER *);
	
	void  (*destroy)    (GFX_CONTAINER *);
	void *(*map)        (GFX_CONTAINER *);
	void  (*unmap)      (GFX_CONTAINER *);
	void  (*update)     (GFX_CONTAINER *, s32 x, s32 y, s32 w, s32 h);
	s32   (*share)      (GFX_CONTAINER *, THREAD *dst_thread);
	s32   (*get_ident)  (GFX_CONTAINER *,char *dst);
	s32   (*get_upcnt)  (GFX_CONTAINER *);
	
	s32   (*draw_hline) (GFX_CONTAINER *,s16 x,s16 y,s16 w,u32 rgba);
	s32   (*draw_vline) (GFX_CONTAINER *,s16 x,s16 y,s16 h,u32 rgba);
	s32   (*draw_box)   (GFX_CONTAINER *,s16 x,s16 y,s16 w,s16 h,u32 rgba);
	s32   (*draw_grad)  (GFX_CONTAINER *,s16 x,s16 y,s16 w,s16 h,float dr, float dg,float db, float da, u32 rgba);
	s32   (*draw_idximg)(GFX_CONTAINER *,s16 x,s16 y,s16 w,s16 h,GFX_CONTAINER *idximg,GFX_CONTAINER *pal, u8 alpha);
	s32   (*draw_img)   (GFX_CONTAINER *,s16 x,s16 y,s16 w,s16 h,GFX_CONTAINER *img,u8 alpha);
	s32   (*draw_string)(GFX_CONTAINER *,s16 x,s16 y,u32 fg_rgba,u32 bg_rgba,s32 fnt_id,char *str);
	s32   (*draw_ansi)  (GFX_CONTAINER *,s16 x,s16 y,s32 fnt_id,char *str,u8 *bgfg);
	
	void  (*push_clipping)  (GFX_CONTAINER *, s32 x, s32 y, s32 w, s32 h);
	void  (*pop_clipping)   (GFX_CONTAINER *);
	void  (*reset_clipping) (GFX_CONTAINER *);

	s32   (*get_clip_x) (GFX_CONTAINER *);
	s32   (*get_clip_y) (GFX_CONTAINER *);
	s32   (*get_clip_w) (GFX_CONTAINER *);
	s32   (*get_clip_h) (GFX_CONTAINER *);
	
	void  (*set_mouse_cursor) (GFX_CONTAINER *, GFX_CONTAINER *cursor);
	void  (*set_mouse_pos)    (GFX_CONTAINER *, s32 x, s32 y);
};
