/*
 * \brief   Graphics function for Nitpicker
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


#define RGBA_TO_RGB16(c) (((c&0xf8000000)>>16)|((c&0x00fc0000)>>13)|((c&0x0000f800)>>11))
#define GFX_ALPHA(rgba) (rgba&255)


/*** DRAW FILLED BOX TO 16BIT SCREEN ***/
static void draw_box_16(u16 *dst, int x1, int y1, int w, int h, u32 rgba) {
	int x, y;
	u16 *dst_line;
	u16 color;
	int alpha;
	int x2 = x1 + w - 1;
	int y2 = y1 + h - 1;

	/* check clipping */
	if (x1 < clip_x1) x1 = clip_x1;
	if (y1 < clip_y1) y1 = clip_y1;
	if (x2 > clip_x2) x2 = clip_x2;
	if (y2 > clip_y2) y2 = clip_y2;
	if (x1 > x2) return;
	if (y1 > y2) return;

	color = RGBA_TO_RGB16(rgba);
	alpha = GFX_ALPHA(rgba);

	dst_line = dst + scr_width*y1;

	/* solid fill */
	if (alpha & 0x80) {
		for (y = y1; y <= y2; y++) {
			dst = dst_line + x1;
			for (x = x1; x <= x2; x++, dst++)
				*dst = color;
			dst_line += scr_width;
		}
		
	/* mix colors for alpha mode */
	} else {
		color = (color >> 1) & 0x7bef;    /* 50% alpha mode */
		for (y = y1; y <= y2; y++) {
			dst = dst_line + x1;
			for (x = x1; x <= x2; x++, dst++)
				*dst = ((*dst >> 1) & 0x7bef) + color;
			dst_line += scr_width;
		}
	}
	return;
}


/*** DRAW CLIPPED 16BIT IMAGE TO 16BIT SCREEN ***/
static void draw_img_16(u16 *dst, int x, int y, int img_w, int img_h, u16 *src, int op) {
	int  i, j;
	int  w = img_w, h = img_h;
	u16 *s, *d;

#if 0
	/* flickerdiflacker */
	draw_box_16(dst, x, y, w, h, -1);
	draw_box_16(dst, x, y, w, h, GFX_RGB(0,0,0));
	draw_box_16(dst, x, y, w, h, -1);
	draw_box_16(dst, x, y, w, h, GFX_RGB(0,0,0));
#endif

	dst += y*scr_width + x;

	/* left clipping */
	if (x < clip_x1) {
		w   -= (clip_x1 - x);
		src += (clip_x1 - x);
		dst += (clip_x1 - x);
		x = clip_x1;
	}

	/* right clipping */
	if (x + w - 1 > clip_x2) w -= (x + w - 1 - clip_x2);

	/* top clipping */
	if (y < clip_y1) {
		h   -= (clip_y1 - y);
		src += (clip_y1 - y)*img_w;
		dst += (clip_y1 - y)*scr_width;
		y = clip_y1;
	}

	/* bottom clipping */
	if (y + h - 1 > clip_y2) h -= (y + h - 1 - clip_y2);

	/* anything left? */
	if ((w < 0) || (h < 0)) return;

	/* paint... */
	for (j = h; j--; ) {

		/* copy line from image to screen */
		switch (op) {

			case GFX_OP_DARKEN:
				for (i = w, s = src, d = dst; i--; s++, d++)
					*d = (*s >> 1) & 0x7bef;
				break;

			case GFX_OP_ALPHA:
				for (i = w, s = src, d = dst; i--; s++, d++)
					if (*s) *d = *s;
				break;

			case GFX_OP_SOLID:
				memcpy(dst, src, w*2);
				break;
		}

		src += img_w;
		dst += scr_width;
	}
}


/*** DRAW STRING TO 16BIT SCREEN ***/
static void draw_string_16(u16 *dst, s16 x, s16 y, font *font,
                           s32 rgba, u8 *str) {
	s32 *wtab = font->width_table;
	s32 *otab = font->offset_table;
	int img_w = font->img_w;
	int img_h = font->img_h;
	int w,  h = font->img_h;
	u8  *src  = font->image;
	u8  *s;
	u16 *d;
	int i, j;
	u16 color = RGBA_TO_RGB16(rgba);

	if (!str || !dst) return;

	dst += y*scr_width + x;

	/* check top clipping */
	if (y < clip_y1) {
		src += (clip_y1 - y)*img_w;   /* skip upper lines in font image    */
		h   -= (clip_y1 - y);         /* decrement number of lines to draw */
		dst += (clip_y1 - y)*scr_width;
	}

	/* check bottom clipping */
	if (y + img_h - 1 > clip_y2)
		h -= y + img_h - 1 - clip_y2;  /* decr. number of lines to draw */

	if (h < 1) return;

	/* skip characters that are completely hidden by the left clipping border */
	while ((*str) && (x + wtab[*str] < clip_x1)) {
		x   += wtab[*str];
		dst += wtab[*str];
		str++;
	}

	/* draw left cut character */
	if ((*str) && (x + wtab[*str] - 1 <= clip_x2) && (x < clip_x1)) {
		w = wtab[*str] - (clip_x1 - x);
		s = src + clip_x1 - x + otab[*str];
		d = dst + clip_x1 - x;
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (*(s + i))
					*(d + i) = color;
			}
			s = s + img_w;
			d = d + scr_width;
		}
		dst += wtab[*str];
		x   += wtab[*str];
		str++;
	}

	/* draw horizontally full visible characters */
	while ((*str) && (x + wtab[*str] - 1 < clip_x2)) {
		w = wtab[*str];
		s = src + otab[*str];
		d = dst;
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (*(s + i))
					*(d + i) = color;
			}
			s = s + img_w;
			d = d + scr_width;
		}
		dst += wtab[*str];
		x   += wtab[*str];
		str++;
	}

	/* draw right cut character */
	if (*str) {
		w = wtab[*str];
		s = src + otab[*str];
		d = dst;
		if (x + w - 1 > clip_x2) {
			w -= x + w - 1 - clip_x2;
		}
		if (x < clip_x1) {    /* check if character is also left-cutted */
			w -= clip_x1 - x;
			s += clip_x1 - x;
			d += clip_x1 - x;
		}
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (*(s + i))
					*(d + i) = color;
			}
			s = s + img_w;
			d = d + scr_width;
		}
	}
	return;
}


/*
 * We need to cast the functions to be compliant with
 * the gfx interface because the implementations use
 * typed pointers for source and destination types.
 */

static gfx_interface gfx16 = {
	(void (*)(void *, int, int, int, int, u32))         draw_box_16,
	(void (*)(void *, int, int, int, int, void *, int)) draw_img_16,
	(void (*)(void *, int, int, font *, u32, u8 *))     draw_string_16,
};

gfx_interface *gfx = &gfx16;   /* pointer to generic graphics backend */


