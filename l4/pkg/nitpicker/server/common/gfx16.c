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
static void draw_box_16(u16 *dst, int dst_llen, int x1, int y1, int w, int h,
                        u32 rgba) {
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

	dst_line = dst + dst_llen*y1;

	/* solid fill */
	if (alpha & 0x80) {
		for (y = y1; y <= y2; y++) {
			dst = dst_line + x1;
			for (x = x1; x <= x2; x++, dst++)
				*dst = color;
			dst_line += dst_llen;
		}
		
	/* mix colors for alpha mode */
	} else {
		color = (color >> 1) & 0x7bef;    /* 50% alpha mode */
		for (y = y1; y <= y2; y++) {
			dst = dst_line + x1;
			for (x = x1; x <= x2; x++, dst++)
				*dst = ((*dst >> 1) & 0x7bef) + color;
			dst_line += dst_llen;
		}
	}
	return;
}


/*** COPY SINGLE PIXEL COLUMN */
static inline void copy_column(u16 *src, u16 *dst, int h, int src_w, int dst_w) {
	if (h) for (; h--; src += src_w, dst += dst_w) *dst = *src;
}


/*** COPY PIXEL BLOCK 32BIT-WISE ***
 *
 * \param  s     source address
 * \param  d     32bit-aligned destination address
 * \param  w     number of 32bit words to copy per line
 * \param  line  line length in 16bit pixels
 */
static inline void copy_block_32bit(u16 *src, u16 *dst, int w, int h,
                                   int src_w, int dst_w) {
	long d0, d1 ,d2;

	asm volatile ("cld; mov %%ds, %%ax; mov %%ax, %%es" : : : "eax");
	for (; h--; src += src_w, dst += dst_w ) {
		asm volatile ("rep movsl"
		 : "=S" (d0), "=D" (d1), "=c" (d2)
		 : "S" (src), "D" (dst), "c" (w));
	}
}


/*** COPY 32BYTE CHUNKS VIA MMX ***/
static inline void copy_32byte_chunks(void *src, void *dst, int size) {
	int dummy;
	asm volatile (
		"emms                             \n\t"
		"xor    %%ecx,%%ecx               \n\t"
		".align 16                        \n\t"
		"0:                               \n\t"
		"movq   (%%esi,%%ecx,8),%%mm0     \n\t"
		"movq   8(%%esi,%%ecx,8),%%mm1    \n\t"
		"movq   16(%%esi,%%ecx,8),%%mm2   \n\t"
		"movq   24(%%esi,%%ecx,8),%%mm3   \n\t"
		"movntq %%mm0,(%%edi,%%ecx,8)     \n\t"
		"movntq %%mm1,8(%%edi,%%ecx,8)    \n\t"
		"movntq %%mm2,16(%%edi,%%ecx,8)   \n\t"
		"movntq %%mm3,24(%%edi,%%ecx,8)   \n\t"
		"add    $4,%%ecx                  \n\t"
		"dec    %%ebx                     \n\t"
		"jnz    0b                        \n\t"
		"sfence                           \n\t"
		"emms                             \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "b" (size)
		: "eax", "ecx", "memory"
	);
}


static inline void prefetch_32byte_chunks(void *src, int size) {
	int dummy;
	asm volatile (
	    "xor    %%ecx,%%ecx               \n\t"
		"0:                               \n\t"
		"movl   (%%esi,%%ecx,8),%%eax     \n\t"
		"movl   32(%%esi,%%ecx,8),%%eax   \n\t"
		"add    $8,%%ecx                  \n\t"
		"dec    %%ebx                     \n\t"
		"jnz    0b                        \n\t"
		: "=d" (dummy)
		: "S" (src), "b" (size>>1)
		: "eax", "ecx", "memory"
	);
}



/*** COPY PIXEL BLOCK WITH A SIZE OF MULTIPLE OF 16 PIXELS ***
 *
 * \param  w      width in 16 pixel chunks
 * \param  line   line width in pixels
 */
static inline void copy_block_32byte(u16 *src,   u16 *dst, int w, int h,
                                     int  src_w, int  dst_w) {
	int i;
	u16 *s, *d;

//	if (w>>1) for (i = h, s = src; i--; s += line)
//		prefetch_32byte_chunks(s, w);

	if (w) for (i = h, s = src, d = dst; i--; s += src_w, d += dst_w)
		copy_32byte_chunks(s, d, w);
}


/*** DRAW CLIPPED 16BIT IMAGE TO 16BIT SCREEN ***/
static void draw_img_16(u16 *dst, int dst_llen, int x, int y, int img_w,
                        int img_h, u16 *src, int op, u32 rgba) {
	int  i, j;
	int  w = img_w, h = img_h;
	u16 *s, *d;
	u16  tint = (RGBA_TO_RGB16(rgba) >> 1) & 0x7bef;

#if 0
	/* flickerdiflacker */
	draw_box_16(dst, x, y, w, h, -1);
	draw_box_16(dst, x, y, w, h, GFX_RGB(0,0,0));
	draw_box_16(dst, x, y, w, h, -1);
	draw_box_16(dst, x, y, w, h, GFX_RGB(0,0,0));
#endif

	dst += y*dst_llen + x;

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
		dst += (clip_y1 - y)*dst_llen;
		y = clip_y1;
	}

	/* bottom clipping */
	if (y + h - 1 > clip_y2) h -= (y + h - 1 - clip_y2);

	/* anything left? */
	if ((w <= 0) || (h <= 0)) return;

	/* paint... */
	switch (op) {

		case GFX_OP_DARKEN:
			for (j = h; j--; src += img_w, dst += dst_llen)
				for (i = w, s = src, d = dst; i--; s++, d++)
					*d = (*s >> 1) & 0x7bef;
			break;

		case GFX_OP_ALPHA:
			for (j = h; j--; src += img_w, dst += dst_llen)
				for (i = w, s = src, d = dst; i--; s++, d++)
					if (*s) *d = *s;
			break;

		case GFX_OP_TINT:
			for (j = h; j--; src += img_w, dst += dst_llen)
				for (i = w, s = src, d = dst; i--; s++, d++)
					*d = ((*s >> 1) & 0x7bef) + tint;
			break;

		case GFX_OP_SOLID:
			{
				/* copy unaligned column */
				if (w && ((long)dst & 2)) {
					copy_column(src, dst, h, img_w, dst_llen);
					w--; src++; dst++;
				}

				/* now, we are on a 32bit aligned destination address */

				/* copy 16byte chunks */
				if (w >> 4) {
					copy_block_32byte(src, dst, w >> 4, h, img_w, dst_llen);
					src += w & ~15;
					dst += w & ~15;
					w    = w &  15;
				}

				/* copy two-pixel chunks */
				if (w >> 1) {
					copy_block_32bit(src, dst, w >> 1, h, img_w, dst_llen);
					src += w & ~1;
					dst += w & ~1;
					w    = w &  1;
				}

				/* handle trailing row */
				if (w) copy_column(src, dst, h, img_w, dst_llen);
			}
			break;
	}
}


/*** DRAW STRING TO 16BIT SCREEN ***/
static void draw_string_16(u16 *dst, int dst_llen, s16 x, s16 y, font *font,
                           s32 rgba, u8 *str) {
	s32 *wtab = font->width_table;
	s32 *otab = font->offset_table;
	int img_w = font->img_w;
	int w,  h = font->img_h;
	u8  *src  = font->image;
	u8  *s;
	u16 *d;
	int i, j;
	u16 color = RGBA_TO_RGB16(rgba);

	if (!str || !dst) return;

	/* check top clipping */
	if (y    <  clip_y1) {
		src += (clip_y1 - y)*img_w;   /* skip upper lines in font image    */
		h   -= (clip_y1 - y);         /* decrement number of lines to draw */
		y   += (clip_y1 - y);
	}

	/* check bottom clipping */
	if (h > clip_y2 - y + 1)
		h = clip_y2 - y + 1;

	if (h < 1) return;

	dst += y*dst_llen + x;

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
			d = d + dst_llen;
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
			d = d + dst_llen;
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
			d = d + dst_llen;
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
	(void (*)(void *, int, int, int, int, int, u32))              draw_box_16,
	(void (*)(void *, int, int, int, int, int, void *, int, u32)) draw_img_16,
	(void (*)(void *, int, int, int, font *, u32, u8 *))          draw_string_16,
};

gfx_interface *gfx = &gfx16;   /* pointer to generic graphics backend */


