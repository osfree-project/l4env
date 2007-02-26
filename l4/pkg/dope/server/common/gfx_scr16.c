/*
 * \brief  DOpE gfx 16bit screen handler module
 * \date   2003-04-02
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

#include "dopestd.h"
#include "scrdrv.h"
#include "cache.h"
#include "fontman.h"
#include "clipping.h"
#include "gfx.h"
#include "gfx_handler.h"

#define RGBA_TO_RGB16(c) (((c&0xf8000000)>>16)|((c&0x00fc0000)>>13)|((c&0x0000f800)>>11))

#define MAX(a,b) (a>b?a:b)
#define MIN(a,b) (a<b?a:b)

static struct scrdrv_services   *scrdrv;
static struct fontman_services  *fontman;
static struct clipping_services *clip;
static struct cache_services    *cache;

static CACHE *imgcache = NULL;

#define COL16(r,g,b) (r<<11) + (g<<6) + b
static u16 coltab_16[16] = {
	COL16( 0, 0, 0),COL16(15, 0, 0),COL16( 0,15, 0),COL16(15,15, 0),
	COL16( 0, 0,15),COL16(15, 0,15),COL16( 0,15,15),COL16(15,15,15),
	COL16( 0, 0, 0),COL16(31, 0, 0),COL16( 0,31, 0),COL16(31,31, 0),
	COL16( 0, 0,31),COL16(31, 0,31),COL16( 0,31,31),COL16(31,31,31),
};


static s32  clip_x1, clip_y1, clip_x2, clip_y2;
static u16 *scr_adr;
static s32  scr_width, scr_height, scr_type;

int init_gfxscr16(struct dope_services *d);

/*************************/
/*** PRIVATE FUNCTIONS ***/
/*************************/


/*** DRAW A SOLID HORIZONTAL LINE IN 16BIT COLOR MODE ***/
static inline void solid_hline_16(short *dst,u32 width,u16 col) {
	for (;width--;dst++) *dst = col;
}


/*** DRAW A SOLID VERTICAL LINE IN 16BIT COLOR MODE ***/
static inline void solid_vline_16(u16 *dst,u32 height,u32 scr_w,u16 col) {
	for (;height--;dst+=scr_w) *dst = col;
}


/*** DRAW A TRANSPARENT HORIZONTAL LINE IN 16BIT COLOR MODE ***/
static inline void mixed_hline_16(u16 *dst,u32 width,u16 mixcol) {
	mixcol=(mixcol&0xf7de)>>1;
	for (;width--;dst++) *dst = (((*dst)&0xf7de)>>1) + mixcol;
}


/*** DRAW A TRANSPARENT VERTICAL LINE IN 16BIT COLOR MODE ***/
static inline void mixed_vline_16(u16 *dst,u32 height,u32 scr_w,u16 mixcol) {
	mixcol=(mixcol&0xf7de)>>1;
	for (;height--;dst+=scr_w) *dst = (((*dst)&0xf7de)>>1) + mixcol;
}


/*** DRAW CLIPPED 16BIT IMAGE TO 16BIT SCREEN ***/
static inline void paint_img_16(s32 x, s32 y, s32 img_w, s32 img_h, u16 *src) {
	s32 i,j;
	s32 w = img_w, h = img_h;
	u16 *dst,*s,*d;

	dst = scr_adr + y*scr_width + x;

	/* left clipping */
	if (x < clip_x1) {
		w   -= (clip_x1-x);
		src += (clip_x1-x);
		dst += (clip_x1-x);
		x=clip_x1;
	}

	/* right clipping */
	if (x+w-1 > clip_x2) w -= (x+w-1 - clip_x2);

	/* top clipping */
	if (y < clip_y1) {
		h   -= (clip_y1-y);
		src += (clip_y1-y)*img_w;
		dst += (clip_y1-y)*scr_width;
		y=clip_y1;
	}

	/* bottom clipping */
	if (y+h-1 > clip_y2) h -= (y+h-1 - clip_y2);

	/* anything left? */
	if ((w<0) || (h<0)) return;

	/* paint... */
	for (j=h;j--;) {

		/* copy line from image to screen */
		for (i=w,s=src,d=dst;i--;*(d++)=*(s++));
		src+=img_w;
		dst+=scr_width;
	}
}


static s32 scale_xbuf[2000];

/*** DRAW SCALED AND CLIPPED 16BIT IMAGE TO 16BIT SCREEN ***/
static inline void paint_scaled_img_16(s32 x,s32 y,s32 w,s32 h, s32 img_w, s32 img_h, u16 *src) {
	float mx,my;
	long i,j;
	float sx = 0.0 ,sy = 0.0;
	u16 *dst,*s,*d;

	if (w) mx = (float)img_w / (float)w;
	else mx=0.0;

	if (h) my = (float)img_h / (float)h;
	else my=0.0;

	dst = scr_adr + y*scr_width + x;

	/* left clipping */
	if (x < clip_x1) {
		w   -= (clip_x1-x);
		sx  += (float)(clip_x1-x) * mx;
		dst += (clip_x1-x);
		x    = clip_x1;
	}

	/* right clipping */
	if (x+w-1 > clip_x2) w -= (x+w-1 - clip_x2);

	/* top clipping */
	if (y < clip_y1) {
		h   -= (clip_y1-y);
		sy  += (float)(clip_y1-y) * my;
		dst += (clip_y1-y)*scr_width;
		y    = clip_y1;
	}

	/* bottom clipping */
	if (y+h-1 > clip_y2) h -= (y+h-1 - clip_y2);

	/* anything left? */
	if ((w<0) || (h<0)) return;

	/* calculate x offsets */
	for (i=w;i--;) {
		scale_xbuf[i]=(long)sx;
		sx += mx;
	}

	/* draw scaled image */
	for (j=h;j--;) {
		s=src + ((long)sy*img_w);
		d=dst;
		for (i=w;i--;) *(d++) = *(s + scale_xbuf[i]);
		sy += my;
		dst+= scr_width;
	}
}

/*** CONVERT AN INDEXED 8-BIT IMAGE TO A 16BIT HICOLOR IMAGE ***/
static u16 coltab[256];
static void convert_8i_to_16(s32 w,s32 h,u8 *src_idx,u32 *src_col, u16 *dst_pix) {
	u32 col24;
	s32 i;

	INFO(printf("GfxImg8i(convert_8i_to_16)\n");)

	/* convert color table from 24bit to 16bit */
	for (i=0;i<256;i++) {
		col24=*(src_col++);
		coltab[i] = ((col24&0xf8000000)>>(16))  |
		            ((col24&0x00fc0000)>>(8+5)) |
		            ((col24&0x0000f800)>>(8+3));
	}

	/* convert pixels using color table */
	for (i=w*h; i--;) {
		*(dst_pix++) = coltab[*(src_idx++)];
	}
}


/*** CONVERT A YUV420 IMAGE TO A 16-BIT HICOLOR IMAGE ***/

static const s32 Inverse_Table_6_9[8][4] = {
	{117504, 138453, 13954, 34903}, /* no sequence_display_extension */
	{117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
	{104597, 132201, 25675, 53279}, /* unspecified */
	{104597, 132201, 25675, 53279}, /* reserved */
	{104448, 132798, 24759, 53109}, /* FCC */
	{104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
	{104597, 132201, 25675, 53279}, /* SMPTE 170M */
	{117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};

static u16 tab_16[197 + 2*682 + 256 + 132];
static void * table_rV[256];
static void * table_gU[256];
static int table_gV[256];
static void * table_bU[256];

#define RGB(i)\
U = pu_[i];\
	V = pv_[i];\
	r = table_rV[V];\
	g = (void *) (((u8 *)table_gU[U]) + table_gV[V]);\
	b = table_bU[U];

#define DST1(i)\
Y = py_1[2*i];\
	dst_1[2*i] = r[Y] + g[Y] + b[Y];\
	Y = py_1[2*i+1];\
	dst_1[2*i+1] = r[Y] + g[Y] + b[Y];

#define DST2(i)\
Y = py_2[2*i];\
	dst_2[2*i] = r[Y] + g[Y] + b[Y];\
	Y = py_2[2*i+1];\
	dst_2[2*i+1] = r[Y] + g[Y] + b[Y];

/* This is exactly the same code as yuv2rgb_c_32 except for the types of */
/* r, g, b, dst_1, dst_2 */

static void convert_yuv420_to_16(int width, int height, u8 *src_yuv420, u16 *dst_rgb16) {
	void *dst =dst_rgb16;
	u8 *py = src_yuv420;
	u8 *pu = src_yuv420 + width*height;
	u8 *pv = src_yuv420 + width*height + width*height/4;
	int rgb_stride = width*2;
	int y_stride = width;
	int uv_stride = width/2;
	int U, V, Y;
	u16 * r, * g, * b;
	u16 * dst_1, * dst_2;

	height >>= 1;
	do {

		u8 * py_1 = py;
		u8 * py_2 = py + y_stride;
		u8 * pu_  = pu;
		u8 * pv_  = pv;
		void * _dst_1 = dst;
		void * _dst_2 = ((u8 *)dst) + rgb_stride;
		int width_ = width;

		width_ >>= 3;
		dst_1 = _dst_1;
		dst_2 = _dst_2;

		do {
			RGB(0); DST1(0); DST2(0);
			RGB(1); DST2(1); DST1(1);
			RGB(2); DST1(2); DST2(2);
			RGB(3); DST2(3); DST1(3);

			pu_ += 4;
			pv_ += 4;
			py_1 += 8;
			py_2 += 8;
			dst_1 += 8;
			dst_2 += 8;
		} while (--width_);

		py += 2 * y_stride;
		pu += uv_stride;
		pv += uv_stride;
		dst = ((u8 *)dst) + 2 * rgb_stride;
	} while (--height);
}

static int div_r(int dividend, int divisor) {
	if (dividend > 0)
		return (dividend + (divisor>>1)) / divisor;
	else
		return -((-dividend + (divisor>>1)) / divisor);
}

static int init_yuv2rgb(void) {
	int i;
	u8 table_Y[1024];
	u16 * table_16 = &tab_16[0];
	void *table_r = 0, *table_g = 0, *table_b = 0;

	int crv =  Inverse_Table_6_9[6][0];
	int cbu =  Inverse_Table_6_9[6][1];
	int cgu = -Inverse_Table_6_9[6][2];
	int cgv = -Inverse_Table_6_9[6][3];

	for (i = 0; i < 1024; i++) {
		int j;
		j = (76309 * (i - 384 - 16) + 32768) >> 16;
		j = (j < 0) ? 0 : ((j > 255) ? 255 : j);
		table_Y[i] = j;
	}

	table_r = table_16 + 197;
	table_b = table_16 + 197 + 685;
	table_g = table_16 + 197 + 2*682;

	for (i = -197; i < 256+197; i++)
		((u16 *)table_r)[i] = (table_Y[i+384] >> 3) << 11;

	for (i = -132; i < 256+132; i++)
		((u16 *)table_g)[i] = (table_Y[i+384] >> 2) << 5;

	for (i = -232; i < 256+232; i++)
		((u16 *)table_b)[i] = (table_Y[i+384] >> 3);

	for (i = 0; i < 256; i++) {
		table_rV[i] = (((u8 *)table_r) + 2 * div_r (crv * (i-128), 76309));
		table_gU[i] = (((u8 *)table_g) + 2 * div_r (cgu * (i-128), 76309));
		table_gV[i] = 2 * div_r (cgv * (i-128), 76309);
		table_bU[i] = (((u8 *)table_b) + 2 * div_r (cbu * (i-128), 76309));
	}
	return 0;
}


/*****************************/
/*** GFX HANDLER FUNCTIONS ***/
/*****************************/

static s32 scr_get_width(struct gfx_ds_data *s) {
	return scr_width;
}

static s32 scr_get_height(struct gfx_ds_data *s) {
	return scr_height;
}

static s32 scr_get_type(struct gfx_ds_data *s) {
	return GFX_IMG_TYPE_RGB16;
}

static void scr_destroy(struct gfx_ds_data *s) {
	scrdrv->restore_screen();
}

static void *scr_map(struct gfx_ds_data *s) {
	return scr_adr;
}

static void scr_update(struct gfx_ds_data *s, s32 x, s32 y, s32 w, s32 h) {
	scrdrv->update_area(x, y, x+w-1, y+h-1);
}

static s32 scr_draw_hline_16(struct gfx_ds_data *s, s16 x, s16 y, s16 w, u32 rgba) {
	int beg_x, end_x;

	if ((clip_y1>y) || (clip_y2<y)) return 0;
	beg_x = MAX(x,clip_x1);
	end_x = MIN(x+w-1,clip_x2);

	if (beg_x > end_x) return 0;

	if (GFX_ALPHA(rgba)>127) {
		solid_hline_16(scr_adr+y*scr_width+beg_x, end_x-beg_x+1, RGBA_TO_RGB16(rgba));
	} else {
		mixed_hline_16(scr_adr+y*scr_width+beg_x, end_x-beg_x+1, RGBA_TO_RGB16(rgba));
	}
	return 0;
}

static s32 scr_draw_vline_16(struct gfx_ds_data *s, s16 x, s16 y, s16 h, u32 rgba) {
	s32 beg_y, end_y;

	if ((clip_x1>x) || (clip_x2<x)) return 0;
	beg_y = MAX(y,clip_y1);
	end_y = MIN(y+h-1,clip_y2);
	if (beg_y > end_y) return 0;

	if (GFX_ALPHA(rgba)>127) {
		solid_vline_16(scr_adr+beg_y*scr_width+x, end_y-beg_y+1, scr_width, RGBA_TO_RGB16(rgba));
	} else {
		mixed_vline_16(scr_adr+beg_y*scr_width+x, end_y-beg_y+1, scr_width, RGBA_TO_RGB16(rgba));
	}
	return 0;
}

static s32 scr_draw_fill_16(struct gfx_ds_data *s, s16 x1, s16 y1, s16 w, s16 h, u32 rgba) {
	static s16 x,y;
	static u16 *dst,*dst_line;
	static u16 color;
	s16 x2 = x1 + w - 1;
	s16 y2 = y1 + h - 1;

	/* check clipping */
	if (x1<clip_x1) x1 = clip_x1;
	if (y1<clip_y1) y1 = clip_y1;
	if (x2>clip_x2) x2 = clip_x2;
	if (y2>clip_y2) y2 = clip_y2;
	if (x1>x2) return -1;
	if (y1>y2) return -1;

	color = RGBA_TO_RGB16(rgba);

	dst_line = scr_adr + scr_width*y1;
	for (y = y1; y <= y2; y++) {
		dst=dst_line + x1;
		for (x = x1; x <= x2; x++) {
			*(dst++) = color;
		}
		dst_line += scr_width;
	}
	return 0;
}


static s32 scr_draw_idximg_16(struct gfx_ds_data *s, s16 x, s16 y, s16 w, s16 h,
                              struct gfx_ds *idximg, struct gfx_ds *pal,
                              u8 alpha) {
	s32  img_w  = idximg->handler->get_width(idximg->data);
	s32  img_h  = idximg->handler->get_height(idximg->data);
	u8  *pix8i  = idximg->handler->map(idximg->data);
	u32 *colors = pal->handler->map(pal->data);

	s32  cache_index = pal->cache_idx;
	s32  cache_ident =  ((idximg->update_cnt<<16) | (pal->update_cnt))
	                 ^ ((s32)idximg) ^ ((s32)pal);

	u16 *pix16  = cache->get_elem(imgcache,cache_index,cache_ident);

	if (!pix16) {
		pix16 = malloc(img_w*img_h*2);
		convert_8i_to_16(img_w,img_h,pix8i,colors,pix16);
		pal->cache_idx = cache->add_elem(imgcache,pix16,img_w*img_h*2,
		                    cache_ident, NULL);
	}

	if ((img_w == w) && (img_h == h)) {
		paint_img_16(x, y, img_w, img_h, pix16);
	} else {
		paint_scaled_img_16(x, y, w, h, img_w, img_h, pix16);
	}
	return 0;
}

static s32 scr_draw_img_16(struct gfx_ds_data *s, s16 x, s16 y, s16 w, s16 h,
                           struct gfx_ds *img, u8 alpha) {

	s32 type  = img->handler->get_type(img->data);
	s32 img_w = img->handler->get_width(img->data);
	s32 img_h = img->handler->get_height(img->data);
	u16 *src = NULL;

	switch (type) {
	case GFX_IMG_TYPE_RGB16:
		src = img->handler->map(img->data);
		break;
	case GFX_IMG_TYPE_YUV420:
		src = cache->get_elem(imgcache,img->cache_idx,123);
		if (!src) {
			src = malloc(img_w*img_h*2);
			img->cache_idx = cache->add_elem(imgcache,src,img_w*img_h*2,123,NULL);
		}
		convert_yuv420_to_16(img_w,img_h,img->handler->map(img->data),src);
	}

	if (!src) return -1;

	if ((img_w == w) && (img_h == h)) {
		paint_img_16(x, y, img_w, img_h, src);
	} else {
		paint_scaled_img_16(x, y, w, h, img_w, img_h, src);
	}

	return 0;
}

static s32 scr_draw_string_16(struct gfx_ds_data *ds, s16 x, s16 y, s32 fg_rgba, u32 bg_rgba, s32 fnt_id, char *str) {
	struct font *font = fontman->get_by_id(fnt_id);
	s32 *wtab=font->width_table;
	s32 *otab=font->offset_table;
	s32 img_w=font->img_w;
	s32 img_h=font->img_h;
	u16 *dst = scr_adr + y*scr_width + x;
	u8  *src = font->image;
	u8  *s;
	u16 *d;
	s16 i,j;
	s32 w;
	s32 h = font->img_h;
	u16 color = RGBA_TO_RGB16(fg_rgba);
	if (!str) return -1;

	/* check top clipping */
	if (y<clip_y1) {
		src += (clip_y1-y)*img_w;   /* skip upper lines in font image */
		h   -= (clip_y1-y);         /* decrement number of lines to draw */
		dst += (clip_y1-y)*scr_width;
	}

	/* check bottom clipping */
	if (y+img_h-1 > clip_y2) h-= (y+img_h-1-clip_y2);  /* decr. number of lines to draw */

	if (h<1) return -1;

	/* skip characters that are completely hidden by the left clipping border */
	while ((*str) && (x+wtab[(int)(*str)] < clip_x1)) {
		x+=wtab[(int)(*str)];
		dst+=wtab[(int)(*str)];
		str++;
	}

	/* draw left cutted character */
	if ((*str) && (x+wtab[(int)(*str)]-1 <= clip_x2) && (x<clip_x1)) {
		w=wtab[(int)(*str)] - (clip_x1-x);
		s=src + otab[(int)(*str)] + (clip_x1-x);
		d=dst + (clip_x1-x);
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=color;
			}
			s=s+img_w;
			d=d+scr_width;
		}
		dst+= wtab[(int)(*str)];
		x+=wtab[(int)(*str)];
		str++;
	}

	/* draw horizontally full visible characters */
	while ((*str) && (x+wtab[(int)(*str)]-1 < clip_x2)) {
		w=wtab[(int)(*str)];
		s=src + otab[(int)(*str)];
		d=dst;
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=color;
			}
			s=s+img_w;
			d=d+scr_width;
		}
		dst+= wtab[(int)(*str)];
		x+=wtab[(int)(*str)];
		str++;
	}

	/* draw right cutted character */
	if (*str) {
		w=wtab[(int)(*str)];
		s=src + otab[(int)(*str)];
		d=dst;
		if (x+w-1> clip_x2) {
			w-= x+w-1 - clip_x2;
		}
		if (x<clip_x1) {    /* check if character is also left-cutted */
			w-=(clip_x1-x);
			s+=(clip_x1-x);
			d+=(clip_x1-x);
		}
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=color;
			}
			s=s+img_w;
			d=d+scr_width;
		}
	}
	return 0;
}

static s32 scr_draw_ansi_16(struct gfx_ds_data *ds, s16 x, s16 y,
                         s32 font_id, char *str, u8 *bgfg) {
	struct font *font = fontman->get_by_id(font_id);
	s32 *wtab=font->width_table;
	s32 *otab=font->offset_table;
	s32 img_w=font->img_w;
	s32 img_h=font->img_h;
	u16 *dst = scr_adr + y*scr_width + x;
	u8  *src = font->image;
	u8  *s;
	u16 *d;
	s16 i,j;
	s32 w;
	s32 h = font->img_h;
	u16 fg_col=-1;
	u16 bg_col=-1;

	if (!str) return -1;

	/* check top clipping */
	if (y<clip_y1) {
		src+= (clip_y1-y)*img_w;    /* skip upper lines in font image */
		h-= (clip_y1-y);            /* decrement number of lines to draw */
		dst+= (clip_y1-y)*scr_width;
	}

	/* check bottom clipping */
	if (y+img_h-1>clip_y2) {
		h-= (y+img_h-1-clip_y2);    /* decrement number of lines to draw */
	}

	if (h<1) return -1;

	/* skip characters that are completely hidden by the left clipping border */
	while ((*str) && (x+wtab[(int)(*str)] < clip_x1)) {
		x+=wtab[(int)(*str)];
		dst+=wtab[(int)(*str)];
		str++;
		bgfg++;
	}

	/* draw left cutted character */
	if ((*str) && (x+wtab[(int)(*str)]-1 <= clip_x2) && (x<clip_x1)) {
		w=wtab[(int)(*str)] - (clip_x1-x);
		s=src + otab[(int)(*str)] + (clip_x1-x);
		d=dst + (clip_x1-x);
		fg_col=coltab_16[*bgfg >> 4];
		bg_col=coltab_16[*bgfg & 0x0f];
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=fg_col;
				else *(d+i)=bg_col;
			}
			s=s+img_w;
			d=d+scr_width;
		}
		dst+= wtab[(int)(*str)];
		x+=wtab[(int)(*str)];
		str++;
		bgfg++;
	}

	/* draw horizontally full visible characters */
	while ((*str) && (x+wtab[(int)(*str)]-1 < clip_x2)) {
		w=wtab[(int)(*str)];
		s= src + otab[(int)(*str)];
		d=dst;
		fg_col=coltab_16[*bgfg >> 4];
		bg_col=coltab_16[*bgfg & 0x0f];
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=fg_col;
				else *(d+i)=bg_col;
			}
			s=s+img_w;
			d=d+scr_width;
		}
		dst+= wtab[(int)(*str)];
		x+=wtab[(int)(*str)];
		str++;
		bgfg++;
	}

	/* draw right cutted character */
	if (*str) {
		w=wtab[(int)(*str)];
		s=src + otab[(int)(*str)];
		d=dst;
		fg_col=coltab_16[*bgfg >> 4];
		bg_col=coltab_16[*bgfg & 0x0f];
		if (x+w-1> clip_x2) {
			w-= x+w-1 - clip_x2;
		}
		if (x<clip_x1) {    /* check if character is also left-cutted */
			w-=(clip_x1-x);
			s+=(clip_x1-x);
			d+=(clip_x1-x);
		}
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=fg_col;
				else *(d+i)=bg_col;
			}
			s=s+img_w;
			d=d+scr_width;
		}
	}
	return 0;
}

static void scr_push_clipping(struct gfx_ds_data *s, s32 x, s32 y, s32 w, s32 h) {
	clip->push(x, y, x+w-1, y+h-1);
	clip_x1 = clip->get_x1();
	clip_y1 = clip->get_y1();
	clip_x2 = clip->get_x2();
	clip_y2 = clip->get_y2();
}

static void scr_pop_clipping(struct gfx_ds_data *s) {
	clip->pop();
	clip_x1 = clip->get_x1();
	clip_y1 = clip->get_y1();
	clip_x2 = clip->get_x2();
	clip_y2 = clip->get_y2();
}

static void scr_reset_clipping(struct gfx_ds_data *s) {
	clip->reset();
	clip_x1 = clip->get_x1();
	clip_y1 = clip->get_y1();
	clip_x2 = clip->get_x2();
	clip_y2 = clip->get_y2();
}

static s32 scr_get_clip_x(struct gfx_ds_data *s) {
	return clip_x1;
}

static s32 scr_get_clip_y(struct gfx_ds_data *s) {
	return clip_y1;
}

static s32 scr_get_clip_w(struct gfx_ds_data *s) {
	return clip_x2 - clip_x1 + 1;
}

static s32 scr_get_clip_h(struct gfx_ds_data *s) {
	return clip_y2 - clip_y1 + 1;
}

static void scr_set_mouse_pos(struct gfx_ds_data *s, s32 x, s32 y) {
	scrdrv->set_mouse_pos(x, y);
}

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static struct gfx_ds_data *create(s32 width, s32 height) {
	scrdrv->set_screen(width, height ,16);
	scr_adr    = scrdrv->get_buf_adr();
	scr_width  = scrdrv->get_scr_width();
	scr_height = scrdrv->get_scr_height();

	if (scrdrv->get_scr_depth() == 16) scr_type = GFX_IMG_TYPE_RGB16;
	else return NULL;

	clip->set_range(0, 0, scr_width-1, scr_height-1);
	return (void *)1;
}

static s32 register_gfx_handler(struct gfx_ds_handler *handler) {
	handler->get_width = scr_get_width;
	handler->get_height = scr_get_height;
	handler->get_type = scr_get_type;
	handler->destroy = scr_destroy;
	handler->map = scr_map;
	handler->update = scr_update;
	handler->draw_hline = scr_draw_hline_16;
	handler->draw_vline = scr_draw_vline_16;
	handler->draw_fill = scr_draw_fill_16;
	handler->draw_img = scr_draw_img_16;
	handler->draw_idximg = scr_draw_idximg_16;
	handler->draw_string = scr_draw_string_16;
	handler->draw_ansi = scr_draw_ansi_16;
	handler->push_clipping = scr_push_clipping;
	handler->pop_clipping = scr_pop_clipping;
	handler->reset_clipping = scr_reset_clipping;
	handler->get_clip_x = scr_get_clip_x;
	handler->get_clip_y = scr_get_clip_y;
	handler->get_clip_w = scr_get_clip_w;
	handler->get_clip_h = scr_get_clip_h;
	handler->set_mouse_pos = scr_set_mouse_pos;
	return 0;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct gfx_handler_services services = {
	create,
	register_gfx_handler,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_gfxscr16(struct dope_services *d) {

	scrdrv  = d->get_module("ScreenDriver 1.0");
	fontman = d->get_module("FontManager 1.0");
	clip    = d->get_module("Clipping 1.0");
	cache   = d->get_module("Cache 1.0");

	imgcache = cache->create(100,1000*1000);

	init_yuv2rgb();

	d->register_module("GfxScreen16 1.0",&services);
	return 1;
}
