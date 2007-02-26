/*
 * \brief   Proxygon pSLIM implementation
 * \date    2004-09-30
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

/*** GENERAL INCLUDES ***/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "pslim.h"

typedef l4_uint8_t  u8;
typedef l4_uint16_t u16;
typedef l4_uint32_t u32;

typedef l4_int8_t  s8;
typedef l4_int16_t s16;
typedef l4_int32_t s32;

extern u8 lat0_12_psf[];

struct pslim_data {
	s8     bpp;                           /* bits per pixel          */
	s32    xres, yres;                    /* screen dimensions       */
	s32    bytes_per_pixel;               /* bytes per pixel         */
	s32    bytes_per_line;                /* bytes per line          */
	void  *pixels;                        /* pointer to pixel buffer */
	void (*update) (int, int, int, int, void *);  /* update function */
	void  *update_arg;                    /* arg of update function  */
};


/**********************************
 *** FUNCTIONS FOR INTERNAL USE ***
 **********************************/

#define dword_t s32
#define byte_t  s8
#define word_t  s16
#define sword_t s16


static inline void update(struct pslim *p, int x, int y, int w, int h) {
	if (!p || !p->pd || !p->pd->update) return;
	p->pd->update(x, y, w, h, p->pd->update_arg);
}


/* private types */
/* offsets in pmap[] and bmap[] */
struct pslim_offset {
  dword_t preskip_x;    /* skip pixels at beginning of line */
  dword_t preskip_y;    /* skip lines */
  dword_t endskip_x;    /* skip pixels at end of line */
/* word_t endskip_y; */ /* snip lines */
};


static inline word_t set_rgb16(dword_t r, dword_t g, dword_t b) {
	return ((b >> 3) + ((g >> 2) << 5) + ((r >> 3) << 11));
	/* RRRRRGGG GGGBBBBB -> 16 */
}

static inline dword_t set_rgb24(dword_t r, dword_t g, dword_t b) {
	return (b + (g << 8) + (r << 16))& 0x00ffffff;
	/* RRRRRRRR GGGGGGGG BBBBBBBB -> 24 */
}


#define OFFSET(x, y, ptr) ptr += (y) * bwidth + (x) * bytepp;

/*** CLIPPING ***/

static inline int clip_rect(struct pslim *p, pslim_rect_t *rect) {
	if ((rect->x > p->pd->xres) || (rect->y > p->pd->yres))
		return 0; /* not in the frame buffer */

	if (rect->x < 0) {
		if (-rect->x > rect->w)
			return 0; /* not visible - left of border */

		/* clip left */
		rect->w += rect->x;
		rect->x  = 0;
	}
	if (rect->y < 0) {
		if (-rect->y > rect->h)
			return 0; /* not visible - above border */

		/* clip top */
		rect->h += rect->y;
		rect->y  = 0;
	}
	if ((rect->x + rect->w) > p->pd->xres)
		rect->w = p->pd->xres - rect->x;        /* clip right */
		if ((rect->y + rect->h) > p->pd->yres)
			rect->h = p->pd->yres - rect->y;    /* clip bottom */

	/* something is visible */
	return 1;
}


static inline int clip_rect_offset(struct pslim *p, pslim_rect_t *rect,
                                   struct pslim_offset *offset) {

	if ((rect->x > p->pd->xres) || (rect->y > p->pd->yres))
		return 0;   /* not in the frame buffer */

	if (rect->x < 0) {
		if (-rect->x > rect->w) return 0;   /* not visible - left of border */

		/* clip left */
		rect->w += rect->x;
		offset->preskip_x = -rect->x;
		rect->x = 0;
	}

	if (rect->y < 0) {
		if (-rect->y > rect->h) return 0;   /* not visible - above border */

		/* clip top */
		rect->h += rect->y;
		offset->preskip_y = -rect->y;
		rect->y = 0;
    }

	if ((rect->x + rect->w) > p->pd->xres) {
		/* clip right */
		offset->endskip_x  = rect->x + rect->w - p->pd->xres;
		rect->w = p->pd->xres - rect->x;
	}

	if ((rect->y + rect->h) > p->pd->yres)
		/* clip bottom */
		rect->h = p->pd->yres - rect->y;

	/* something is visible */
	return 1;
}


static inline int clip_rect_dxy(struct pslim *p, pslim_rect_t *rect,
                                sword_t *dx, sword_t *dy) {

	/* clip source rectangle */
	if ((rect->x > p->pd->xres) || (rect->y > p->pd->yres))
		return 0;       /* not in the frame buffer */

	if (rect->x < 0) {
		if (-rect->x > rect->w)
			return 0;   /* not visible - left of border */

		/* clip left */
		rect->w += rect->x;
		*dx     -= rect->x;
		rect->x  = 0;
	}

	if (rect->y < 0) {
		if (-rect->y > rect->h)
			return 0;   /* not visible - above border */

		/* clip top */
		rect->h += rect->y;
		*dy     -= rect->y;
		rect->y  = 0;
	}

	/* clip right */
	if ((rect->x + rect->w) > p->pd->xres) rect->w = p->pd->xres - rect->x;

	/* clip bottom */
	if ((rect->y + rect->h) > p->pd->yres) rect->h = p->pd->yres - rect->y;

	/* clip destination rectangle */
	if ((*dx > p->pd->xres) || (*dy > p->pd->yres))
		return 0; /* not in the frame buffer */

	if (*dx < 0) {
		if (-*dx > rect->w)
			return 0;   /* not visible - left of border */

		/* clip left */
		rect->w += *dx;
		rect->x -= *dx;
		*dx = 0;
	}

	if (*dy < 0) {
		if (-*dy > rect->h)
			return 0;   /* not visible - above border */

		/* clip top */
		rect->h += *dy;
		rect->y -= *dy;
		*dy = 0;
	}

	/* clip right */
	if ((*dx + rect->w) > p->pd->xres) rect->w = p->pd->xres - *dx;

	/* clip bottom */
	if ((*dy + rect->h) > p->pd->yres) rect->h = p->pd->yres - *dy;

	return 1;   /* something is visible */
}


static inline void _bmap16lsb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc, dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits = 0;
	int i, j, k, kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits)%8;
			if ( bmap[k] & (0x01 << kmod) )
				*(word_t*) (&vfb[2*j]) = (word_t) (fgc & 0xffff);
			else
				*(word_t*) (&vfb[2*j]) = (word_t) (bgc & 0xffff);
		}
		vfb += bwidth;
	}
}


static inline void _bmap16msb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc, dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	int i, j;
	dword_t nobits = offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		unsigned char mask, *b;
		nobits += offset->preskip_x;
		mask = 0x80 >> (nobits % 8);
		b = bmap + nobits / 8;
		for (j = 0; j < w; j++, nobits++) {

			/* gcc is able to code the entire loop without using any jump */
			/* if compiled with -march=i686 (uses cmov instructions then) */

			*(word_t*) (&vfb[2*j]) =
				(*b & mask) ? (word_t) (fgc & 0xffff) : (word_t) (bgc & 0xffff);
			b += mask & 1;
			mask = (mask >> 1) | (mask << 7); /* gcc optimizes this into ROR */
		}
		vfb += bwidth;
		nobits += offset->endskip_x;
	}
}


static inline void _bmap24lsb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc, dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits = 0;
	int i, j, k, kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits)%8;
			if ( bmap[k] & (0x01 << kmod) ) {
				*(word_t*) (&vfb[3*j]) = (word_t) (fgc & 0xffff);
				vfb[3*j+2] = (byte_t) (fgc >> 16);
			} else {
				*(word_t*) (&vfb[3*j]) = (word_t) (bgc & 0xffff);
				vfb[3*j+2] = (byte_t) (bgc >> 16);
			}
		}
		vfb += bwidth;
	}
}


static inline void _bmap24msb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc, dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits = 0;
	int i, j, k, kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits) % 8;
			if ( bmap[k] & (0x80 >> kmod) ) {
				*(word_t*) (&vfb[3*j]) = (word_t) (fgc & 0xffff);
				vfb[3*j + 2] = (byte_t) (fgc >> 16);
			} else {
				*(word_t*) (&vfb[3*j]) = (word_t) (bgc & 0xffff);
				vfb[3*j + 2] = (byte_t) (bgc >> 16);
			}
		}
		vfb += bwidth;
		/* length of one line in bmap parsed */
		nobits += offset->endskip_x;
	}
}


static inline void _bmap32lsb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc, dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits = 0;
	int i, j, k, kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits) % 8;
			if ( bmap[k] & (0x01 << kmod) )
				*(dword_t*) (vfb + 4*j) = (dword_t) (fgc & 0xffffffff);
			else
				*(dword_t*) (vfb + 4*j) = (dword_t) (bgc & 0xffffffff);
		}
		vfb += bwidth;
	}
}


static inline void _bmap32msb(byte_t *vfb, byte_t *bmap,
                              dword_t fgc, dword_t bgc,  dword_t w, dword_t h,
                              struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits = 0;
	int i, j, k, kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits) % 8;
			if ( bmap[k] & (0x80 >> kmod) )
				*(dword_t*) (&vfb[4*j]) = (dword_t) (fgc & 0x00ffffff);
			else
				*(dword_t*) (&vfb[4*j]) = (dword_t) (bgc & 0x00ffffff);
		}
		vfb += bwidth;
		/* length of one line in bmap parsed */
		nobits += offset->endskip_x;
	}
}


static inline void _set16(byte_t *vfb, byte_t *pmap, dword_t w, dword_t h,
                          struct pslim_offset* offset,
                          dword_t bytepp, dword_t bwidth) {
	int i;
	for (i = 0; i < h; i++) {
		unsigned dummy;
		pmap += bytepp * offset->preskip_x;
		asm volatile ("rep movsw"
		   :"=S"(dummy), "=D"(dummy), "=c"(dummy)
		   :"0"(pmap), "1"(vfb), "2"(w));
		vfb  += bwidth;
		pmap += bytepp * (w + offset->endskip_x);
	}
}


static inline void _set24(byte_t *vfb, byte_t *pmap, dword_t w, dword_t h,
                          struct pslim_offset* offset,
                          dword_t bytepp, dword_t bwidth) {
	int i;
	for (i = 0; i < h; i++) {
		unsigned dummy;
		pmap += bytepp * offset->preskip_x;
		asm volatile ("rep movsb"
		   :"=S"(dummy), "=D"(dummy), "=c"(dummy)
		   :"0"(pmap), "1"(vfb), "2"(3*w));
		vfb += bwidth;
		pmap += bytepp * (w + offset->endskip_x);
	}
}


static inline void _set32(byte_t *vfb, byte_t *pmap, dword_t w, dword_t h,
                          struct pslim_offset* offset, dword_t bytepp,
                          dword_t bwidth) {
	int i;
	for (i = 0; i < h; i++) {
		unsigned dummy;
		pmap += bytepp * offset->preskip_x;
		asm volatile ("rep movsl"
		   :"=S"(dummy), "=D"(dummy), "=c"(dummy)
		   :"0"(pmap), "1"(vfb), "2"(w));
		vfb  += bwidth;
		pmap += bytepp * (w + offset->endskip_x);
	}
}


static inline void _copy16(byte_t *vfb, sword_t x, sword_t y,
                           sword_t dx, sword_t dy,
                           dword_t w, dword_t h,
                           dword_t bytepp,  dword_t bwidth) {
	int i;
	byte_t *src = vfb, *dest = vfb;

	if (dy == y && dx == x) return;

	if (y >= dy) {
		OFFSET( x,  y, src);
		OFFSET(dx, dy, dest);
		for (i = 0; i < h; i++) {
			/* memmove can deal with overlapping regions */
			memmove(dest, src, 2*w);
			src  += bwidth;
			dest += bwidth;
		}
    } else {
		OFFSET( x,  y + h - 1, src);
		OFFSET(dx, dy + h - 1, dest);
		for (i = 0; i < h; i++) {
			/* memmove can deal with overlapping regions */
			memmove(dest, src, 2*w);
			src  -= bwidth;
			dest -= bwidth;
		}
	}
}


static inline void _copy24(byte_t *vfb, sword_t x, sword_t y,
                           sword_t dx, sword_t dy,
                           dword_t w, dword_t h,
                           dword_t bytepp, dword_t bwidth) {
	int i, j;
	byte_t *src = vfb, *dest = vfb;

	if (y >= dy) {
		if (y == dy && dx >= x) {   /* tricky */
			if (x == dx)  return;

			/* my way: start right go left */
			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = w; j >= 0; --j) {
					*(word_t*) (&dest[3*j]) = *(word_t*) (&src[3*j]);
					dest[3*j + 2] = src[3*j + 2];
				}
				src  += bwidth;
				dest += bwidth;
			}
		} else {    /* copy from top to bottom */

			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = 0; j < w; j++) {
					*(word_t*) (&dest[3*j]) = *(word_t*) (&src[3*j]);
					dest[3*j + 2] = src[3*j + 2];
				}
				src  += bwidth;
				dest += bwidth;
			}
		}
	} else {    /* copy from bottom to top */
		OFFSET( x,  y + h, src);
		OFFSET(dx, dy + h, dest);
		for (i = 0; i < h; i++) {
			src  -= bwidth;
			dest -= bwidth;
			for (j = 0; j < w; j++) {
				*(word_t*) (&dest[3*j]) = *(word_t*) (&src[3*j]);
				dest[3*j + 2] = src[3*j + 2];
			}
		}
	}
}


static inline void _copy32(byte_t *vfb, sword_t x, sword_t y,
                           sword_t dx, sword_t dy,
                           dword_t w, dword_t h,
                           dword_t bytepp, dword_t bwidth) {
	int i, j;
	byte_t *src = vfb, *dest = vfb;

	if (y >= dy) {
		if (y == dy && dx >= x) {   /* tricky */
			if (x == dx) return;

			/* my way: start right go left */
			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = w; j >= 0; --j)
					*(dword_t*) (&dest[4*j]) = *(dword_t*) (&src[4*j]);
				src  += bwidth;
				dest += bwidth;
			}
		} else {    /* copy from top to bottom */
			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = 0; j < w; j++)
					*(dword_t*) (&dest[4*j]) = *(dword_t*) (&src[4*j]);
				src  += bwidth;
				dest += bwidth;
			}
		}
	} else {    /* copy from bottom to top */
		OFFSET( x,  y + h, src);
		OFFSET(dx, dy + h, dest);
		for (i = 0; i < h; i++) {
			src  -= bwidth;
			dest -= bwidth;
			for (j = 0; j < w; j++)
				*(dword_t*) (&dest[4*j]) = *(dword_t*) (&src[4*j]);
		}
	}
}


static inline void _fill16(byte_t *vfb, dword_t w, dword_t h, dword_t color,
                           dword_t bwidth) {
	int i, dummy;
	for (i = 0; i < h; i++) {
		asm volatile ("rep stosw"
		:"=D"(dummy), "=c"(dummy)
		:"0"(vfb), "1"(w), "a"(color & 0xffff));
		vfb += bwidth;
	}
}


static inline void _fill24(byte_t *vfb, dword_t w, dword_t h, dword_t color,
                           dword_t bwidth) {
	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			*(word_t*) (&vfb[3*j]) = (word_t) (color & 0xffff);
			vfb[3*j+2] = (byte_t) (color >> 16);
		}
		vfb += bwidth;
	}
}


static inline void _fill32(byte_t *vfb, dword_t w, dword_t h, dword_t color,
                           dword_t bwidth) {
	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++)
		*(dword_t*) (&vfb[4*j]) = (dword_t) (color & 0xffffffff);
		vfb += bwidth;
	}
}

#define VSUBSAMP(mode)  (((mode) >> 3) & 0x07)
#define HSUBSAMP(mode)  ((mode) & 0x07)

#define range(x, mi, ma) ( (((x) > (mi)) && ((x) < (ma))) ? \
			   (x) : \
			   ( ((x) < (mi)) ? \
			     (mi) : \
			     (ma)))

static inline void _cscs16(byte_t *vfb, dword_t w, dword_t h,
                           const byte_t *Y, const byte_t *U, const byte_t *V,
                           dword_t bwidth, dword_t mode) {
	int i, j;
	dword_t color32;
	int cy, cu, cv;
	int r, g, b;
	dword_t yidx, uvidx, yidx0, uvidx0;

	for (i = 0; i < h; i++) {
		color32 = 0;
		uvidx0 =  i / VSUBSAMP(mode) * w / VSUBSAMP(mode);
		yidx0 = i * w;
		for (j = 0; j < w; j++) {
			yidx = yidx0 + j;
			uvidx = uvidx0 + j / HSUBSAMP(mode);
			cy = Y[yidx];
			cu = U[uvidx] - 128;
			cv = V[uvidx] - 128;

			cy = range (cy, 16, 235);

			r = (1024 * cy + (1404 * cv)) / 1024;
			g = (1024 * cy - ( 344 * cu) - (716 * cv)) / 1024;
			b = (1024 * cy + (1774 * cu)) / 1024;

			r = range(r, 0, 255);
			g = range(g, 0, 255);
			b = range(b, 0, 255);

			*(word_t*) (&vfb[2*j]) = set_rgb16(r, g, b);
		}
		vfb += bwidth;
	}
}


static inline void _cscs24(byte_t *vfb, dword_t w, dword_t h,
                           const byte_t *Y, const byte_t *U, const byte_t *V,
                           dword_t bwidth, dword_t mode) {
	int i, j;
	dword_t color32, color;
	int cy, cu, cv;
	int r, g, b;
	dword_t yidx, uvidx, yidx0, uvidx0;

	for (i = 0; i < h; i++) {
		color32 = 0;
		uvidx0 =  i / VSUBSAMP(mode) * w / VSUBSAMP(mode);
		yidx0 = i * w;
		for (j = 0; j < w; j++) {
			yidx = yidx0 + j;
			uvidx = uvidx0 + j / HSUBSAMP(mode);
			cy = Y[yidx];
			cu = U[uvidx] - 128;
			cv = V[uvidx] - 128;

			cy = range (cy, 16, 235);

			r = (1024 * cy + (1404 * cv)) / 1024;
			g = (1024 * cy - ( 344 * cu) - (716 * cv)) / 1024;
			b = (1024 * cy + (1774 * cu)) / 1024;

			r = range(r, 0, 255);
			g = range(g, 0, 255);
			b = range(b, 0, 255);

			color = set_rgb24(r, g, b);

			*(word_t*) (&vfb[3*j]) = color >> 16;
			vfb[3*j + 3] = color >> 8;
		}
		vfb += bwidth;
	}
}


static inline void _cscs32(byte_t *vfb, dword_t w, dword_t h,
                           const byte_t *Y, const byte_t *U, const byte_t *V,
                           dword_t bwidth,  dword_t mode) {
	int i, j;
	dword_t color32;
	int cy, cu, cv;
	int r, g, b;
	dword_t yidx, uvidx, yidx0, uvidx0;

	for (i = 0; i < h; i++) {
		color32 = 0;
		uvidx0 =  i / VSUBSAMP(mode) * w / VSUBSAMP(mode);
		yidx0 = i * w;
		for (j = 0; j < w; j++) {
			yidx = yidx0 + j;
			uvidx = uvidx0 + j / HSUBSAMP(mode);
			cy = Y[yidx];
			cu = U[uvidx] - 128;
			cv = V[uvidx] - 128;

			cy = range (cy, 16, 235);

			r = (1024 * cy + (1404 * cv)) / 1024;
			g = (1024 * cy - ( 344 * cu) - (716 * cv)) / 1024;
			b = (1024 * cy + (1774 * cu)) / 1024;

			r = range(r, 0, 255);
			g = range(g, 0, 255);
			b = range(b, 0, 255);

			*(dword_t*) (&vfb[4*j]) = set_rgb24(r, g, b);
		}
		vfb += bwidth;
	}
}


static inline long _str16(u16 *dst, s32 x, s32 y, s32 xres,
                          const char *s, s32 max_chars, u16 fg, u16 bg) {
	s16 num_chars = 0, i, j;
	u16 *d;
	u8  *src, charline, mask;

	dst += y*xres + x;

	while ((*s) && (num_chars <= max_chars)) {
		src = &lat0_12_psf[pSLIM_FONT_CHAR_H*(*s) + 4];
		d = dst;
		for (j = pSLIM_FONT_CHAR_H; j--; ) {
			mask = 0x80;
			charline = *(src++);
			for (i = pSLIM_FONT_CHAR_W; i--; ) {
				if (charline & mask) *(d++) = fg;
				else *(d++) = bg;
				mask = mask>>1;
			}
			d += xres - pSLIM_FONT_CHAR_W;
		}
		dst += pSLIM_FONT_CHAR_W;
		s++;
		num_chars++;
	}
	return num_chars;
}


static inline long _str_attr16(u16 *dst, s32 x, s32 y, s32 xres,
                               const u8 *s, s32 max_chars) {

	static pslim_color_t color_tab16[16] = {
		0x0000, 0x0015, 0x05a0, 0x05b5, 0xa800, 0xa815, 0xaaa0, 0xad55,
		0x52aa, 0x52bf, 0x57ea, 0x57ff, 0xfaaa, 0xfabf, 0xffea, 0xffff
	};

	s16 num_chars = 0, i, j;
	u16 *d, bg, c;
	s32 fg;
	u8  *src, charline, mask;

	dst  += y * xres + x;
	xres -= pSLIM_FONT_CHAR_W;

	while ((*s) && (*(s+1)) && (num_chars <= max_chars)) {
		src = &lat0_12_psf[pSLIM_FONT_CHAR_H * (*s) + 4];

		c = *((u16 *)s);
		fg = color_tab16[(c & 0x0F00) >> 8];
		bg = color_tab16[(c & 0xF000) >> 8];

		d = dst;
		for (j = pSLIM_FONT_CHAR_H; j--; ) {
			mask = 0x80;
			charline = *(src++);
			for (i = pSLIM_FONT_CHAR_W; i--; ) {
				*(d++) = (charline & mask) ? fg : bg;
				mask = mask>>1;
			}
			if (j & 11) fg -= 0x0841;
			d += xres;
		}
		dst += pSLIM_FONT_CHAR_W;
		s += 2;
		num_chars++;
	}
	return num_chars;
}


/******************************
 *** PSLIM SPECIFIC METHODS ***
 ******************************/

/*** TEST IF A GIVEN GRAPHICS MODE IS VALID ***/
static int pslim_probe_mode(struct pslim *p, int width, int height, int depth) {
	if (!p) return 0;
	if (depth != 16) return 0;
	if (width*height <= 0) return 0;
	return 1;
}


/*** SET GRAPHICS MODE ***/
static int pslim_set_mode(struct pslim *p, int width, int height, int depth, void *buf) {

	if (!p || !pslim_probe_mode(p, width, height, depth)) return -1;

	p->pd->bpp  = 0;
	p->pd->xres = 0;
	p->pd->yres = 0;
	p->pd->bytes_per_pixel = 0;
	p->pd->bytes_per_line  = 0;
	p->pd->pixels = NULL;

	/* create new frame buffer image */
	switch (depth) {
	case 16:
		p->pd->bpp = 16;
		p->pd->xres = width;
		p->pd->yres = height;
		p->pd->bytes_per_pixel = 2;
		p->pd->bytes_per_line  = p->pd->bytes_per_pixel * width;
		p->pd->pixels = buf;
	}
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: FILL AREA ***/
static int pslim_fill(struct pslim *p, const pslim_rect_t *rect, pslim_color_t color){
	u8 *dst;

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect(p, (pslim_rect_t *)rect)) return 0;  /* nothing visible */

	dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;

	switch(p->pd->bpp) {
		case 24: _fill24(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
		case 32: _fill32(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
		case 16: _fill16(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
	}
	update(p, rect->x, rect->y, rect->w, rect->h);
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: COPY AREA ***/
static int pslim_copy(struct pslim *p, const pslim_rect_t *rect, int dx, int dy){
	u8  *dst;

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect(p, (pslim_rect_t *)rect)) return 0;  /* nothing visible */

	switch(p->pd->bpp) {
		case 16:
			_copy16(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
			        p->pd->bytes_per_pixel, p->pd->bytes_per_line);
			break;
		case 24:
			_copy24(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
			        p->pd->bytes_per_pixel, p->pd->bytes_per_line);
			break;
		case 32:
			_copy32(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
			        p->pd->bytes_per_pixel, p->pd->bytes_per_line);
			break;
	}
	update(p, dx, dy, rect->w, rect->h);
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: PAINT BITMAP ***/
static int pslim_bmap(struct pslim *p, const pslim_rect_t *rect, pslim_color_t fg,
                      pslim_color_t bg, const void *bmap, char type){
	u8 *dst, *src = (u8 *)bmap;
	struct pslim_offset offset = {0, 0, 0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

	dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;

	switch (type) {
		case pSLIM_BMAP_START_MSB:
			switch(p->pd->bpp) {
				case 32:
					_bmap32msb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
					break;
				case 24:
					_bmap24msb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
					break;
				case 16:
					_bmap16msb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
			}
			break;
		case pSLIM_BMAP_START_LSB:
		default:
			switch(p->pd->bpp) {
				case 32:
					_bmap32lsb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
					break;
				case 24:
					_bmap24lsb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
					break;
				case 16:
					_bmap16lsb(dst, src, fg, bg, rect->w, rect->h, &offset, p->pd->bytes_per_line);
			}
	}
	update(p, rect->x, rect->y, rect->w, rect->h);
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: PAINT PIXMAP ***/
static int pslim_set(struct pslim *p, const pslim_rect_t *rect, const void *pmap){
	u8 *dst, *src = (u8 *)pmap;
	struct pslim_offset offset = {0, 0, 0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

	dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;
	src += p->pd->bytes_per_pixel * offset.preskip_y * (rect->w + offset.preskip_x + offset.endskip_x);

	switch(p->pd->bpp) {
	case 32:
		_set32(dst, src, rect->w, rect->h, &offset, p->pd->bytes_per_pixel, p->pd->bytes_per_line);
		break;
	case 24:
		_set24(dst, src, rect->w, rect->h, &offset, p->pd->bytes_per_pixel, p->pd->bytes_per_line);
		break;
	case 16:
		_set16(dst, src, rect->w, rect->h, &offset, p->pd->bytes_per_pixel, p->pd->bytes_per_line);
	}
	update(p, rect->x, rect->y, rect->w, rect->h);
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: COLOR SPACE CONVERSION ***/
static int pslim_cscs(struct pslim *p, const pslim_rect_t *rect, const char *y,
                      const char *u, const char *v, long yuv_type, char scale){
	u8 *dst;
	struct pslim_offset offset = {0, 0, 0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

	switch(p->pd->bpp) {
	case 32:
		_cscs32(dst, rect->w, rect->h, y, u, v, p->pd->bytes_per_line, yuv_type);
		break;
	case 24:
		_cscs24(dst, rect->w, rect->h, y, u, v, p->pd->bytes_per_line, yuv_type);
		break;
	case 16:
		_cscs16(dst, rect->w, rect->h, y, u, v, p->pd->bytes_per_line, yuv_type);
	}
	update(p, rect->x, rect->y, rect->w, rect->h);
	return 0;
}


/*** PSLIM PROTOCOL EXTENSION FUNCTION: PUT STRING WITH DEFINED FORE/BACKGROUND COLOR ***/
static int pslim_puts(struct pslim *p, const char *s, int len, int x, int y,
                      pslim_color_t fg, pslim_color_t bg) {

	s32 char_cnt = 0;
	s32 xmax, ymax, max_chars;
	int i = 0;

	if (!p) return -1;
	if (!p->pd->pixels) return -1;

	xmax = p->pd->xres - pSLIM_FONT_CHAR_W + 1;
	ymax = p->pd->yres - pSLIM_FONT_CHAR_H + 1;

	/* check is the string is vertically visible */
	if ((y < 0) || (y > ymax)) return 0;

	/* skip characters, that are behind the left screen border */
	while ((x < 0) && s[i] && (i < len)) { x += pSLIM_FONT_CHAR_W; i++;}

    max_chars = (xmax - x) / pSLIM_FONT_CHAR_W;
	if (max_chars > len - i - 1) max_chars = len - i - 1;
	if (max_chars < 0) return 0;
	switch (p->pd->bpp) {
	case 16:
		char_cnt = _str16(p->pd->pixels, x, y, p->pd->xres, s, max_chars, fg, bg);
		break;
	default:
		return 0;
	}
	update(p, x, y, char_cnt * pSLIM_FONT_CHAR_W, pSLIM_FONT_CHAR_H);
	return 0;
}


/*** PSLIM PROTOCOL EXTENSION FUNCTION: PUT STRING WITH DEFINED FORE/BACKGROUND COLOR ***/
static int pslim_puts_attr(struct pslim *p, const char *s, int len, int x, int y){

	s32 char_cnt = 0;
	s32 xmax, ymax, max_chars;
	int i = 0;

	len = len >> 1;

	if (!p) return -1;
	if (!p->pd->pixels) return -1;

	xmax = p->pd->xres-pSLIM_FONT_CHAR_W + 1;
	ymax = p->pd->yres-pSLIM_FONT_CHAR_H + 1;

	/* check is the string is vertically visible */
	if ((y < 0) || (y > ymax)) return 0;

	/* ignore requests with a negative x value */
	if (x < 0) return 0;

	max_chars = (xmax - x)/pSLIM_FONT_CHAR_W;
	if (max_chars > len - i - 1) max_chars = len - i - 1;
	if (max_chars < 0) return 0;

	switch (p->pd->bpp) {
	case 16:
		char_cnt = _str_attr16(p->pd->pixels, x, y, p->pd->xres, s, max_chars);
		break;
	default:
		return 0;
	}
	update(p, x, y, char_cnt * pSLIM_FONT_CHAR_W, pSLIM_FONT_CHAR_H);
	return 0;
}


/*** FREE PSLIM CANVAS ***/
static void pslim_destroy(struct pslim *p) {
	if (!p) return;
	if (p->pd) free(p->pd);
	free(p);
}


/*** CREATE A NEW PSLIM CANVAS ***/
struct pslim *create_pslim_canvas(void (*update) (int, int, int, int, void *),
                                  void *update_arg) {

	struct pslim *new = malloc(sizeof(struct pslim));

	if (!new) {
		printf("Error: create_pslim_canvas: unable to allocate pslim struct\n");
		return NULL;
	}

	if (!update) {
		printf("Error: create_pslim_canvas: no update function supplied\n");
		return NULL;
	}

	memset(new, 0, sizeof(struct pslim));
	new->probe_mode = pslim_probe_mode;
	new->set_mode   = pslim_set_mode;
	new->fill       = pslim_fill;
	new->copy       = pslim_copy;
	new->bmap       = pslim_bmap;
	new->set        = pslim_set;
	new->cscs       = pslim_cscs;
	new->puts       = pslim_puts;
	new->puts_attr  = pslim_puts_attr;
	new->pd         = malloc(sizeof(struct pslim_data));
	new->destroy    = pslim_destroy;

	if (!new->pd) {
		printf("Error: create_pslim_canvas: unable to allocate pslim data struct\n");
		return NULL;
	}

	memset(new->pd, 0, sizeof(struct pslim_data));
	new->pd->update     = update;
	new->pd->update_arg = update_arg;

	return new;
}

