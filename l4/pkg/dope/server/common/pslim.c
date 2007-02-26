/*
 * \brief   DOpE pSLIM widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * A pSLIM widget represents a  virtual screen
 * buffer that can be accessed via a small set
 * of graphical  primitive  functions  such as
 * fill, copy, etc.
 * Each pSLIM widget starts a server thread to
 * which a client can send pSLIM commands.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct pslim;
#define WIDGET struct pslim

#include "dopestd.h"
#include "event.h"
#include "widget_data.h"
#include "gfx.h"
#include "widget.h"
#include "pslim.h"
#include "redraw.h"
#include "messenger.h"
#include "pslim_server.h"
#include "script.h"
#include "widman.h"

extern u8 lat0_12_psf[];

static struct gfx_services          *gfx;
static struct widman_services       *widman;
static struct script_services       *script;
static struct redraw_services       *redraw;
//static struct image16_services      *img16;
static struct messenger_services    *msg;
static struct pslim_server_services *pslim_server;

#define UPDATE_MODE_EVERYTIME 0x01 /* update after each operation */
#define UPDATE_MODE_REALTIME  0x02 /* cyclic update via rtman*/
#define UPDATE_MODE_EXPLICIT  0x04 /* explicit update call */


struct pslim_data {
	long  update_flags;
	char *server;                  /* associated pslim server identifier */
	s8    bpp;                     /* bits per pixel */
	s32   xres, yres;              /* screen dimensions */
	s32   bytes_per_pixel;         /* bytes per pixel */
	s32   bytes_per_line;          /* bytes per line */
	void *pixels;                  /* pointer to pixel buffer */
//  void *image;                   /* image representation (for drawing) */
	struct gfx_ds *image;   /* image gfx container */
	s16   update_mode;
};

int init_pslim(struct dope_services *d);

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




#define dword_t s32
#define byte_t  s8
#define word_t  s16
#define sword_t s16

/* private types */
/* offsets in pmap[] and bmap[] */
struct pslim_offset {
  dword_t preskip_x;    /* skip pixels at beginning of line */
  dword_t preskip_y;    /* skip lines */
  dword_t endskip_x;    /* skip pixels at end of line */
/* word_t endskip_y; */ /* snip lines */
};



static inline word_t set_rgb16(dword_t r, dword_t g, dword_t b) {
	return ((b >> 3) +
		((g >> 2) << 5) +
		((r >> 3) << 11));
	/* RRRRRGGG GGGBBBBB -> 16*/
}

static inline dword_t set_rgb24(dword_t r, dword_t g, dword_t b) {
	return (b + (g << 8) + (r << 16))& 0x00ffffff;
	/* RRRRRRRR GGGGGGGG BBBBBBBB -> 24 */
}


#define OFFSET(x, y, ptr) ptr += (y) * bwidth + (x) * bytepp;

/* clipping */

static inline int clip_rect(PSLIM *p, pslim_rect_t *rect) {
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


static inline int clip_rect_offset(PSLIM *p,
						pslim_rect_t *rect, struct pslim_offset *offset) {

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


static inline int clip_rect_dxy(PSLIM *p,
							    pslim_rect_t *rect, sword_t *dx, sword_t *dy) {

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
	dword_t nobits=0;
	int i,j, k,kmod;

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
	dword_t nobits=0;
	int i,j, k,kmod;

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
	dword_t nobits=0;
	int i,j, k,kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits)%8;
			if ( bmap[k] & (0x80 >> kmod) ) {
				*(word_t*) (&vfb[3*j]) = (word_t) (fgc & 0xffff);
				vfb[3*j+2] = (byte_t) (fgc >> 16);
			} else {
				*(word_t*) (&vfb[3*j]) = (word_t) (bgc & 0xffff);
				vfb[3*j+2] = (byte_t) (bgc >> 16);
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
	dword_t nobits=0;
	int i,j, k,kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits)%8;
			if ( bmap[k] & (0x01 << kmod) )
				(dword_t) (vfb[4*j]) = (dword_t) (fgc & 0xffffffff);
			else
				(dword_t) (vfb[4*j]) = (dword_t) (bgc & 0xffffffff);
		}
		vfb += bwidth;
	}
}


static inline void _bmap32msb(byte_t *vfb, byte_t *bmap,
							  dword_t fgc, dword_t bgc,  dword_t w, dword_t h,
							  struct pslim_offset* offset, dword_t bwidth) {
	dword_t nobits=0;
	int i,j, k,kmod;

	/* length of one line in bmap (bits!) */
	nobits += offset->preskip_y * (w + offset->preskip_x + offset->endskip_x);

	for (i = 0; i < h; i++) {
		nobits += offset->preskip_x;
		for (j = 0; j < w; j++, nobits++) {
			k = nobits>>3;
			kmod = (nobits)%8;
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
						  dword_t bytepp,dword_t bwidth) {
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
		vfb += bwidth;
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
			src += bwidth;
			dest += bwidth;
		}
    } else {
		OFFSET( x,  y + h - 1, src);
		OFFSET(dx, dy + h - 1, dest);
		for (i = 0; i < h; i++) {
			/* memmove can deal with overlapping regions */
			memmove(dest, src, 2*w);
			src -= bwidth;
			dest -= bwidth;
		}
	}
}


static inline void _copy24(byte_t *vfb, sword_t x, sword_t y,
						   sword_t dx, sword_t dy,
						   dword_t w, dword_t h,
						   dword_t bytepp, dword_t bwidth) {
	int i,j;
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
					dest[3*j+2] = src[3*j+2];
				}
				src += bwidth;
				dest += bwidth;
			}
		} else {    /* copy from top to bottom */

			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = 0; j < w; j++) {
					*(word_t*) (&dest[3*j]) = *(word_t*) (&src[3*j]);
					dest[3*j+2] = src[3*j+2];
				}
				src += bwidth;
				dest += bwidth;
			}
		}
	} else {    /* copy from bottom to top */
		OFFSET( x,  y + h, src);
		OFFSET(dx, dy + h, dest);
		for (i = 0; i < h; i++) {
			src -= bwidth;
			dest -= bwidth;
			for (j = 0; j < w; j++) {
				*(word_t*) (&dest[3*j]) = *(word_t*) (&src[3*j]);
				dest[3*j+2] = src[3*j+2];
			}
		}
	}
}


static inline void _copy32(byte_t *vfb, sword_t x, sword_t y,
						   sword_t dx, sword_t dy,
						   dword_t w, dword_t h,
						   dword_t bytepp, dword_t bwidth) {
	int i,j;
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
				src += bwidth;
				dest += bwidth;
			}
		} else {    /* copy from top to bottom */
			OFFSET( x,  y, src);
			OFFSET(dx, dy, dest);
			for (i = 0; i < h; i++) {
				for (j = 0; j < w; j++)
					*(dword_t*) (&dest[4*j]) = *(dword_t*) (&src[4*j]);
				src += bwidth;
				dest += bwidth;
			}
		}
	} else {    /* copy from bottom to top */
		OFFSET( x,  y + h, src);
		OFFSET(dx, dy + h, dest);
		for (i = 0; i < h; i++) {
			src -= bwidth;
			dest -= bwidth;
			for (j = 0; j < w; j++)
				*(dword_t*) (&dest[4*j]) = *(dword_t*) (&src[4*j]);
		}
	}
}

static inline void _fill16(byte_t *vfb, dword_t w, dword_t h, dword_t color,
						   dword_t bwidth) {
	int i,dummy;
	for (i = 0; i < h; i++) {
		asm volatile ("rep stosw"
		:"=D"(dummy), "=c"(dummy)
		:"0"(vfb), "1"(w), "a"(color & 0xffff));
		vfb += bwidth;
	}
}


static inline void _fill24(byte_t *vfb, dword_t w, dword_t h, dword_t color,
						   dword_t bwidth) {
	int i,j;
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
	int i,j;
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
						   byte_t *Y, byte_t *U, byte_t *V,
						   dword_t bwidth,byte_t mode) {
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
			cu = U[uvidx]-128;
			cv = V[uvidx]-128;

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
						   byte_t *Y, byte_t *U, byte_t *V,
						   dword_t bwidth, byte_t mode) {
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
			cu = U[uvidx]-128;
			cv = V[uvidx]-128;

			cy = range (cy, 16, 235);

			r = (1024 * cy + (1404 * cv)) / 1024;
			g = (1024 * cy - ( 344 * cu) - (716 * cv)) / 1024;
			b = (1024 * cy + (1774 * cu)) / 1024;

			r = range(r, 0, 255);
			g = range(g, 0, 255);
			b = range(b, 0, 255);

			color = set_rgb24(r, g, b);

			*(word_t*) (&vfb[3*j]) = color >> 16;
			vfb[3*j+3] = color >> 8;
		}
		vfb += bwidth;
	}
}


static inline void _cscs32(byte_t *vfb, dword_t w, dword_t h,
						   byte_t *Y, byte_t *U, byte_t *V,
						   dword_t bwidth,  byte_t mode) {
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
			cu = U[uvidx]-128;
			cv = V[uvidx]-128;

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


static inline long _str16(u16 *dst,s32 x,s32 y,s32 xres,
						  const char *s,s32 max_chars,u16 fg,u16 bg) {
	s16 num_chars=0,i,j;
	u16 *d;
	u8  *src,charline,mask;

	dst+= y*xres + x;

	while ((*s) && (num_chars<=max_chars)) {
		src = &lat0_12_psf[pSLIM_FONT_CHAR_H*(*s)+4];
		d=dst;
		for (j=pSLIM_FONT_CHAR_H;j--;) {
			mask=0x80;
			charline=*(src++);
			for (i=pSLIM_FONT_CHAR_W;i--;) {
				if (charline & mask) *(d++)=fg;
				else *(d++)=bg;
				mask = mask>>1;
			}
			d+=xres-pSLIM_FONT_CHAR_W;
		}
		dst+=pSLIM_FONT_CHAR_W;
		s++;
		num_chars++;
	}
	return num_chars;
}


static inline long _str_attr16(u16 *dst,s32 x,s32 y,s32 xres,
							   const u8 *s,s32 max_chars) {

	static pslim_color_t color_tab16[16] = {
		0x0000, 0x0015, 0x05a0, 0x05b5, 0xa800, 0xa815, 0xaaa0, 0xad55,
		0x52aa, 0x52bf, 0x57ea, 0x57ff, 0xfaaa, 0xfabf, 0xffea, 0xffff
	};

	s16 num_chars=0,i,j;
	u16 *d,bg,c;
	s32 fg;
	u8  *src,charline,mask;

	dst  += y*xres + x;
	xres -=pSLIM_FONT_CHAR_W;

	while ((*s) && (*(s+1)) && (num_chars<=max_chars)) {
		src = &lat0_12_psf[pSLIM_FONT_CHAR_H*(*s)+4];

		c = *((u16 *)s);
		fg = color_tab16[(c & 0x0F00) >> 8];
		bg = color_tab16[(c & 0xF000) >> 8];

		d=dst;
		for (j=pSLIM_FONT_CHAR_H;j--;) {
			mask=0x80;
			charline=*(src++);
			for (i=pSLIM_FONT_CHAR_W;i--;) {
				*(d++) = (charline & mask) ? fg : bg;
				mask = mask>>1;
			}
			if (j&11) fg-=0x0841;
			d+=xres;
		}
		dst+=pSLIM_FONT_CHAR_W;
		s+=2;
		num_chars++;
	}
	return num_chars;
}


/*** CONVERT PSLIM X COORDINATE TO SCREEN X COORDINATE ***/
static s32 x2scr(PSLIM *p,long x) {
	return (s32)((float)x * (float)p->wd->w / (float)p->pd->xres);
}


/*** CONVERT PSLIM Y COORDINATE TO SCREEN X COORDINATE ***/
static s32 y2scr(PSLIM *p,long y) {
	return (s32)((float)y * (float)p->wd->h / (float)p->pd->yres);
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void pslim_draw(PSLIM *p,struct gfx_ds *ds,long x,long y) {

	if (!p->pd->image) return;

	x+=p->wd->x;
	y+=p->wd->y;

	gfx->push_clipping(ds,x,y,p->wd->w,p->wd->h);
	gfx->draw_img(ds,x,y,p->wd->w,p->wd->h,p->pd->image,255);
	gfx->pop_clipping(ds);

}


static void (*orig_update) (PSLIM *p,u16 redraw_flag);

static void pslim_update(PSLIM *p,u16 redraw_flag) {
	orig_update(p,redraw_flag);
	p->pd->update_flags=0;
}

/******************************/
/*** PSLIM SPECIFIC METHODS ***/
/******************************/


/*** REGISTER PSLIM WIDGET SERVER ***/
static void pslim_reg_server(PSLIM *p,char *server_ident) {
	if (!p || !server_ident) return;
	printf("PSLIM(reg_server): register PSLIM server with ident=%s\n",server_ident);
	p->pd->server = strdup(server_ident);
}


/*** RETURN PSLIM WIDGET SERVER ***/
static u8 *pslim_get_server(PSLIM *p) {
	if (!p) return "<noserver>";
	return p->pd->server;
}


/*** TEST IF A GIVEN GRAPHICS MODE IS VALID ***/
static s32 pslim_probe_mode(PSLIM *p,s32 width, s32 height, s32 depth) {
	if (!p) return 0;
	if (depth != 16) return 0;
	if (width*height <= 0) return 0;
	return 1;
}


/*** SET GRAPHICS MODE ***/
static s32 pslim_set_mode(PSLIM *p,s32 width, s32 height, s32 depth) {
	if (!p) return 0;
	if (!pslim_probe_mode(p,width,height,depth)) return 0;

	/* destroy old image buffer and reset values */
	if (p->pd->image) gfx->destroy(p->pd->image);

	p->pd->bpp  = 0;
	p->pd->xres = 0;
	p->pd->yres = 0;
	p->pd->bytes_per_pixel = 0;
	p->pd->bytes_per_line  = 0;
	p->pd->pixels = NULL;
	p->pd->image  = NULL;

	/* create new frame buffer image */
	switch (depth) {
	case 16:
		if ((p->pd->image = gfx->alloc_img(width, height, GFX_IMG_TYPE_RGB16))) {
			p->pd->bpp = 16;
			p->pd->xres = width;
			p->pd->yres = height;
			p->pd->bytes_per_pixel = 2;
			p->pd->bytes_per_line  = p->pd->bytes_per_pixel * width;
			p->pd->pixels = gfx->map(p->pd->image);
		} else {
			INFO(printf("PSLIM(set_mode): out of memory!\n");)
			return 0;
		}
	}

	p->gen->set_w(p,width);
	p->gen->set_h(p,height);
	p->gen->update(p,1);
	printf("set mode finished!!!\n");
	return 1;
}


/*** PSLIM PROTOCOL FUNCTION: FILL AREA ***/
static s32 pslim_fill(PSLIM *p,const pslim_rect_t *rect,pslim_color_t color){
	u8  *dst;
//  printf("pslim_fill(xywh = %d,%d,%d,%d), color = %d\n",
//          (int)rect->x,(int)rect->y,(int)rect->w,(int)rect->h,(int)color);
	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect(p, (pslim_rect_t *)rect)) return 0;  /* nothing visible */

    dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;

	switch(p->pd->bpp) {
    case 24: _fill24(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
    case 32: _fill32(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
    case 16: _fill16(dst, rect->w, rect->h, color, p->pd->bytes_per_line); break;
    }
	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,rect->x),y2scr(p,rect->y),
							  x2scr(p,rect->x+rect->w),
							  y2scr(p,rect->y+rect->h));
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: COPY AREA ***/
static s32 pslim_copy(PSLIM *p,const pslim_rect_t *rect,s32 dx,s32 dy){
	u8  *dst;

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect(p, (pslim_rect_t *)rect)) return 0;  /* nothing visible */

	switch(p->pd->bpp) {
    case 16:
		_copy16(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
				p->pd->bytes_per_pixel,p->pd->bytes_per_line);
		break;
    case 24:
		_copy24(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
				p->pd->bytes_per_pixel,p->pd->bytes_per_line);
		break;
    case 32:
		_copy32(dst, rect->x, rect->y, dx, dy, rect->w, rect->h,
				p->pd->bytes_per_pixel,p->pd->bytes_per_line);
		break;
    }

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,dx),y2scr(p,dy),
							  x2scr(p,dx+rect->w),
							  y2scr(p,dy+rect->h));
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: PAINT BITMAP ***/
static s32 pslim_bmap(PSLIM *p,const pslim_rect_t *rect,pslim_color_t fg,pslim_color_t bg,void *bmap,u8 type){
	u8  *dst,*src=(u8 *)bmap;
    struct pslim_offset offset = {0,0,0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

    dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;

	switch (type) {
	case pSLIM_BMAP_START_MSB:
		switch(p->pd->bpp) {
		case 32:
			_bmap32msb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
			break;
		case 24:
			_bmap24msb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
			break;
		case 16:
			_bmap16msb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
		}
		break;
	case pSLIM_BMAP_START_LSB:
    default:
		switch(p->pd->bpp) {
		case 32:
			_bmap32lsb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
			break;
		case 24:
			_bmap24lsb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
			break;
		case 16:
			_bmap16lsb(dst,src,fg,bg,rect->w,rect->h,&offset,p->pd->bytes_per_line);
		}
	}

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,rect->x),y2scr(p,rect->y),
							  x2scr(p,rect->x+rect->w),
							  y2scr(p,rect->y+rect->h));
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: PAINT PIXMAP ***/
static s32 pslim_set(PSLIM *p,const pslim_rect_t *rect,void *pmap){
	u8  *dst,*src=(u8 *)pmap;
	struct pslim_offset offset = {0,0,0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

    dst += rect->y * p->pd->bytes_per_line + rect->x * p->pd->bytes_per_pixel;
	src += p->pd->bytes_per_pixel * offset.preskip_y * (rect->w + offset.preskip_x + offset.endskip_x);

	switch(p->pd->bpp) {
	case 32:
		_set32(dst,src,rect->w,rect->h,&offset,p->pd->bytes_per_pixel,p->pd->bytes_per_line);
		break;
	case 24:
		_set24(dst,src,rect->w,rect->h,&offset,p->pd->bytes_per_pixel,p->pd->bytes_per_line);
		break;
	case 16:
		_set16(dst,src,rect->w,rect->h,&offset,p->pd->bytes_per_pixel,p->pd->bytes_per_line);
	}

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,rect->x),y2scr(p,rect->y),
							  x2scr(p,rect->x+rect->w),
							  y2scr(p,rect->y+rect->h));
	return 0;
}


/*** PSLIM PROTOCOL FUNCTION: COLOR SPACE CONVERSION ***/
static s32 pslim_cscs(PSLIM *p,const pslim_rect_t *rect,void *yuv,s8 yuv_type,u8 scale){
	u8 *dst, *y, *u, *v;
	struct pslim_offset offset = {0,0,0};

	if (!p || !rect) return 0;
	if (!(dst = (u8 *) p->pd->pixels)) return 0;
	if (!clip_rect_offset(p, (pslim_rect_t *)rect, &offset)) return 0;

	y = (u8 *) yuv;
	u = ((u8 *) yuv) + rect->w * rect->h;
	v = u + ((rect->w / HSUBSAMP(yuv_type)) * (rect->h / VSUBSAMP(yuv_type)));

	switch(p->pd->bpp) {
	case 32:
		_cscs32(dst,rect->w,rect->h,y,u,v,p->pd->bytes_per_line,yuv_type);
		break;
	case 24:
		_cscs24(dst,rect->w,rect->h,y,u,v,p->pd->bytes_per_line,yuv_type);
		break;
	case 16:
		_cscs16(dst,rect->w,rect->h,y,u,v,p->pd->bytes_per_line,yuv_type);
	}

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,rect->x),y2scr(p,rect->y),
							  x2scr(p,rect->x+rect->w),
							  y2scr(p,rect->y+rect->h));
	return 0;
}


/*** PSLIM PROTOCOL EXTENSION FUNCTION: PUT STRING WITH DEFINED FORE/BACKGROUND COLOR ***/
static s32 pslim_puts(PSLIM *p,const char *s,s16 x,s16 y,pslim_color_t fg,pslim_color_t bg){

	s32 char_cnt=0;
	s32 xmax,ymax,max_chars;

	if (!p) return -1;
	if (!p->pd->pixels) return -1;

	xmax=p->pd->xres-pSLIM_FONT_CHAR_W+1;
	ymax=p->pd->yres-pSLIM_FONT_CHAR_H+1;

	/* check is the string is vertically visible */
	if ((y<0) || (y>ymax)) return 0;

	/* skip characters, that are behind the left screen border */
	while ((x<0) && (*s)) { x+=pSLIM_FONT_CHAR_W;s++;}

    max_chars = (xmax-x)/pSLIM_FONT_CHAR_W;
	switch (p->pd->bpp) {
	case 16:
		char_cnt=_str16(p->pd->pixels,x,y,p->pd->xres,s,max_chars,fg,bg);
		break;
	default:
		return 0;
	}

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,x),y2scr(p,y),
							  x2scr(p,x+char_cnt*pSLIM_FONT_CHAR_W)+1,
							  y2scr(p,y+pSLIM_FONT_CHAR_H)+1);
	return 0;
}


/*** PSLIM PROTOCOL EXTENSION FUNCTION: PUT STRING WITH DEFINED FORE/BACKGROUND COLOR ***/
static s32 pslim_puts_attr(PSLIM *p,const char *s,s16 x,s16 y){

	s32 char_cnt=0;
	s32 xmax,ymax,max_chars;

	if (!p) return -1;
	if (!p->pd->pixels) return -1;

	xmax=p->pd->xres-pSLIM_FONT_CHAR_W+1;
	ymax=p->pd->yres-pSLIM_FONT_CHAR_H+1;

	/* check is the string is vertically visible */
	if ((y<0) || (y>ymax)) return 0;

	/* skip characters, that are behind the left screen border */
	while ((x<0) && (*s) && (*(s+1))) { x+=pSLIM_FONT_CHAR_W;s+=2;}

    max_chars = (xmax-x)/pSLIM_FONT_CHAR_W;
	switch (p->pd->bpp) {
	case 16:
		char_cnt=_str_attr16(p->pd->pixels,x,y,p->pd->xres,s,max_chars);
		break;
	default:
		return 0;
	}

	if (p->pd->update_mode != UPDATE_MODE_EVERYTIME) return 0;
	redraw->draw_widgetarea(p,x2scr(p,x),y2scr(p,y),
							  x2scr(p,x+char_cnt*pSLIM_FONT_CHAR_W)+1,
							  y2scr(p,y+pSLIM_FONT_CHAR_H)+1);
	return 0;
}


static struct widget_methods gen_methods;
static struct pslim_methods  pslim_methods={
	pslim_reg_server,
	pslim_get_server,

	pslim_probe_mode,
	pslim_set_mode,

	pslim_fill,     /* PSLIM protocol functions */
	pslim_copy,
	pslim_bmap,
	pslim_set,
	pslim_cscs,

	pslim_puts,     /* protocol extensions */
	pslim_puts_attr,

};

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static PSLIM *create(void) {

	/* allocate memory for new widget */
	PSLIM *new = (PSLIM *)malloc(sizeof(struct pslim)
	            + sizeof(struct widget_data)
	            + sizeof(struct pslim_data));
	if (!new) {
		ERROR(printf("PSLIM(create): out of memory\n"));
		return NULL;
	}
	new->gen    = &gen_methods;     /* pointer to general widget methods */
	new->pslim  = &pslim_methods;   /* pointer to pslim specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *) ((long)new + sizeof(struct pslim));
	new->pd = (struct pslim_data *)  ((long)new->wd + sizeof(struct widget_data));
	widman->default_widget_data(new->wd);
	new->wd->max_w = 100000;
	new->wd->max_h = 100000;

	new->pd->update_flags=0;
	new->pd->server = NULL;
	new->pd->bpp    = 0;
	new->pd->xres   = 0;
	new->pd->yres   = 0;
	new->pd->bytes_per_pixel = 0;
	new->pd->bytes_per_line  = 0;
	new->pd->pixels = NULL;
	new->pd->image  = NULL;
	new->pd->update_mode = UPDATE_MODE_EVERYTIME;

	if (pslim_server) pslim_server->start(new);

	/* wait until server registered */
	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct pslim_services services = {
	create
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/


static void script_fill(PSLIM *p,long x,long y,long w,long h,long color) {
	pslim_rect_t r;
	r.x=x;r.y=y;r.w=w,r.h=h;
	pslim_fill(p,&r,(pslim_color_t)color);
}


static void script_copy(PSLIM *p,long x,long y,long w,long h,long dx,long dy) {
	pslim_rect_t r;
	r.x=x;r.y=y;r.w=w,r.h=h;
	pslim_copy(p,&r,dx,dy);
}


static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("PSLIM",(void *(*)(void))create);

	script->reg_widget_method(widtype,"long probemode(long width,long height,long depth)",pslim_probe_mode);
	script->reg_widget_method(widtype,"long setmode(long width,long height,long depth)",pslim_set_mode);
	script->reg_widget_method(widtype,"long fill(long x,long y,long w,long h,long color)",script_fill);
	script->reg_widget_method(widtype,"long copy(long x,long y,long w,long h,long dx,long sy)",script_copy);
	script->reg_widget_method(widtype,"long puts(string text,long x,long y,long fg,long bg)",pslim_puts);
	script->reg_widget_method(widtype,"long puts_attr(string text,long x,long y)",pslim_puts_attr);
	script->reg_widget_method(widtype,"string getserver()",pslim_get_server);

	widman->build_script_lang(widtype,&gen_methods);
}


int init_pslim(struct dope_services *d) {

	msg    = d->get_module("Messenger 1.0");
	gfx    = d->get_module("Gfx 1.0");
	script = d->get_module("Script 1.0");
	widman = d->get_module("WidgetManager 1.0");
	redraw = d->get_module("RedrawManager 1.0");
	pslim_server= d->get_module("PSLIMServer 1.0");

	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);

	orig_update=gen_methods.update;

	gen_methods.draw=pslim_draw;
	gen_methods.update=pslim_update;
//  gen_methods.handle_event=pslim_handle_event;

	build_script_lang();

	d->register_module("PSLIM 1.0",&services);
	return 1;
}
