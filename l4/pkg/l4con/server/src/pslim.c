/* $Id$ */

/**
 * \file	con/server/src/pslim.c
 * \brief	implementation of pSLIM protocol
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdlib.h>
#include <string.h>		/* needed for memmove */

/* local includes */
#include "main.h"
#include "l4con.h"
#include "pslim.h"
#include "con_hw/init.h"
#include "con_yuv2rgb/yuv2rgb.h"

/* private types */
/** offsets in pmap[] and bmap[] */
struct pslim_offset
{
  l4_uint32_t preskip_x;	/**< skip pixels at beginning of line */
  l4_uint32_t preskip_y;	/**< skip lines */
  l4_uint32_t endskip_x;	/**< skip pixels at end of line */
/* word_t endskip_y; */	/* snip lines */
};

extern l4_offs_t vis_offs;

static inline void _bmap16msb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _bmap24msb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _bmap32msb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _bmap16lsb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _bmap24lsb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _bmap32lsb(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			      l4_uint32_t, l4_uint32_t, l4_uint32_t,
			      struct pslim_offset*, l4_uint32_t);
static inline void _set16(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			  l4_uint32_t, struct pslim_offset*,
			  l4_uint32_t, l4_uint32_t);
static inline void _set24(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			  l4_uint32_t, struct pslim_offset*,
			  l4_uint32_t, l4_uint32_t);
static inline void _set32(l4_uint8_t*, l4_uint8_t*, l4_uint32_t,
			  l4_uint32_t, struct pslim_offset*,
			  l4_uint32_t, l4_uint32_t);
static inline void _copy16(l4_uint8_t*, l4_int16_t, l4_int16_t, l4_int16_t,
			   l4_int16_t, l4_uint32_t, l4_uint32_t, l4_uint32_t);
static inline void _copy24(l4_uint8_t*, l4_int16_t, l4_int16_t, l4_int16_t,
			   l4_int16_t, l4_uint32_t, l4_uint32_t, l4_uint32_t);
static inline void _copy32(l4_uint8_t*, l4_int16_t, l4_int16_t, l4_int16_t,
			   l4_int16_t, l4_uint32_t, l4_uint32_t, l4_uint32_t);

static inline void _fill16(l4_uint8_t*, l4_uint32_t, l4_uint32_t,
			   l4_uint32_t, l4_uint32_t);
static inline void _fill24(l4_uint8_t*, l4_uint32_t, l4_uint32_t,
			   l4_uint32_t, l4_uint32_t);
static inline void _fill32(l4_uint8_t*, l4_uint32_t, l4_uint32_t,
			   l4_uint32_t, l4_uint32_t);

/* all pslim_*-functions call the vc->g_mode specific functions */
static inline void sw_bmap(struct l4con_vc*, l4_int16_t, l4_int16_t,
			   l4_uint32_t, l4_uint32_t,
			   l4_uint8_t *bmap, l4_uint32_t fgc, l4_uint32_t bgc,
			   struct pslim_offset*, l4_uint8_t mode);
static inline void  sw_set(struct l4con_vc*, l4_int16_t, l4_int16_t,
			   l4_uint32_t, l4_uint32_t, l4_uint32_t, l4_uint32_t,
			   l4_uint8_t *pmap, struct pslim_offset*);
static inline void sw_cscs(struct l4con_vc*, l4_int16_t, l4_int16_t,
			   l4_uint32_t, l4_uint32_t,
			   l4_uint8_t *y, l4_uint8_t *u, l4_uint8_t *v,
			   l4_uint32_t scale, struct pslim_offset*,
			   l4_uint8_t mode);

static inline l4_uint16_t
set_rgb16(l4_uint32_t r, l4_uint32_t g, l4_uint32_t b)
{
  return ((b >> 3) + ((g >> 2) << 5) + ((r >> 3) << 11));
  /* RRRRRGGG GGGBBBBB -> 16*/
}

static inline l4_uint32_t
set_rgb24(l4_uint32_t r, l4_uint32_t g, l4_uint32_t b)
{
  return (b + (g << 8) + (r << 16));
  /* RRRRRRRR GGGGGGGG BBBBBBBB -> 24 */
}


#define OFFSET(x, y, ptr, bytepp) ptr += (y) * bwidth + (x) * (bytepp);

/* clipping */

static inline int
clip_rect(struct l4con_vc *vc, int from_user, l4con_pslim_rect_t *rect)
{
  int max_x = vc->xres;
  int max_y = vc->yres;

  if (from_user)
    {
      rect->x += vc->client_xofs;
      rect->y += vc->client_yofs;
      max_x    = vc->client_xres;
      max_y    = vc->client_yres;
    }

  if ((rect->x > max_x) || (rect->y > max_y))
    /* not in the frame buffer */
    return 0;
  if (rect->x < 0)
    {
      if (-rect->x >= rect->w)
	/* not visible - left of border */
	return 0;
      /* clip left */
      rect->w += rect->x;
      rect->x  = 0;
    }
  if (rect->y < 0)
    {
      if (-rect->y >= rect->h)
	/* not visible - above border */
	return 0;
      /* clip top */
      rect->h += rect->y;
      rect->y  = 0;
    }
  if ((rect->x + rect->w) > max_x)
    /* clip right */
    rect->w = max_x - rect->x;
  if ((rect->y + rect->h) > max_y)
    /* clip bottom */
    rect->h = max_y - rect->y;

  /* something is visible */
  return 1;
}

static inline int
clip_rect_offset(struct l4con_vc *vc, int from_user,
		 l4con_pslim_rect_t *rect, struct pslim_offset *offset)
{
  int max_x = vc->xres;
  int max_y = vc->yres;

  if (from_user)
    {
      rect->x += vc->client_xofs;
      rect->y += vc->client_yofs;
      max_x    = vc->client_xres;
      max_y    = vc->client_yres;
    }

  if ((rect->x > max_x) || (rect->y > max_y))
    /* not in the frame buffer */
    return 0;
  if (rect->x < 0)
    {
      if (-rect->x > rect->w)
	/* not visible - left of border */
	return 0;
      /* clip left */
      rect->w           += rect->x;
      offset->preskip_x  = -rect->x;
      rect->x            = 0;
    }
  if (rect->y < 0)
    {
      if (-rect->y > rect->h)
	/* not visible - above border */
	return 0;
      /* clip top */
      rect->h           += rect->y;
      offset->preskip_y  = -rect->y;
      rect->y            = 0;
    }
  if ((rect->x + rect->w) > max_x)
    {
      /* clip right */
      offset->endskip_x  = rect->x + rect->w - max_x;
      rect->w = max_x - rect->x;
    }
  if ((rect->y + rect->h) > max_y)
    /* clip bottom */
    rect->h = max_y - rect->y;

  /* something is visible */
  return 1;
}

static inline int
clip_rect_dxy(struct l4con_vc *vc, int from_user,
	      l4con_pslim_rect_t *rect, l4_int16_t *dx, l4_int16_t *dy)
{
  int max_x = vc->xres;
  int max_y = vc->yres;

  if (from_user)
    {
      rect->x += vc->client_xofs;
      rect->y += vc->client_yofs;
      max_x    = vc->client_xres;
      max_y    = vc->client_yres;
    }

  /* clip source rectangle */
  if ((rect->x > max_x) || (rect->y > max_y))
    /* not in the frame buffer */
    return 0;
  if (rect->x < 0)
    {
      if (-rect->x > rect->w)
	/* not visible - left of border */
	return 0;
      /* clip left */
      rect->w += rect->x;
      *dx     -= rect->x;
      rect->x  = 0;
    }
  if (rect->y < 0)
    {
      if (-rect->y > rect->h)
	/* not visible - above border */
	return 0;
      /* clip top */
      rect->h += rect->y;
      *dy     -= rect->y;
      rect->y  = 0;
    }
  if ((rect->x + rect->w) > max_x)
    /* clip right */
    rect->w = max_x - rect->x;
  if ((rect->y + rect->h) > max_y)
    /* clip bottom */
    rect->h = max_y - rect->y;

  /* clip destination rectangle */
  if ((*dx > max_x) || (*dy > max_y))
    /* not in the frame buffer */
    return 0;
  if (*dx < 0)
    {
      if (-*dx > rect->w)
	/* not visible - left of border */
	return 0;
      /* clip left */
      rect->w += *dx;
      rect->x -= *dx;
      *dx = 0;
    }
  if (*dy < 0)
    {
      if (-*dy > rect->h)
	/* not visible - above border */
	return 0;
      /* clip top */
      rect->h += *dy;
      rect->y -= *dy;
      *dy = 0;
    }
  if ((*dx + rect->w) > max_x)
    /* clip right */
    rect->w = max_x - *dx;
  if ((*dy + rect->h) > max_y)
    /* clip bottom */
    rect->h = max_y - *dy;

  /* something is visible */
  return 1;
}

static inline void
_bmap16lsb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
   l4_uint32_t nobits=0;
   int i,j, k,kmod;

   nobits += offset->preskip_y
      * (w + offset->preskip_x + offset->endskip_x);
   /* length of one line in bmap (bits!) */

   for (i = 0; i < h; i++) {
      nobits += offset->preskip_x;
      for (j = 0; j < w; j++, nobits++) {
	 k = nobits>>3;
	 kmod = (nobits)%8;
	 if ( bmap[k] & (0x01 << kmod) )
	    *(l4_uint16_t*) (&vfb[2*j]) = (l4_uint16_t) (fgc & 0xffff);
	 else
	    *(l4_uint16_t*) (&vfb[2*j]) = (l4_uint16_t) (bgc & 0xffff);
      }
      vfb += bwidth;
   }
}

static inline void
_bmap16msb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
  int i, j;
  l4_uint32_t nobits = offset->preskip_y
		   * (w + offset->preskip_x + offset->endskip_x);

  for (i = 0; i < h; i++)
    {
      unsigned char mask, *b;
      nobits += offset->preskip_x;
      mask = 0x80 >> (nobits % 8);
      b = bmap + nobits / 8;
      for (j = 0; j < w; j++, nobits++)
	{
	  /* gcc is able to code the entire loop without using any jump
	   * if compiled with -march=i686 (uses cmov instructions then) */
	  *(l4_uint16_t*) (&vfb[2*j]) = (*b & mask)
					? (l4_uint16_t) (fgc & 0xffff)
					: (l4_uint16_t) (bgc & 0xffff);
	  b += mask & 1;
	  mask = (mask >> 1) | (mask << 7); /* gcc optimizes this into ROR */
	}
      vfb += bwidth;
      nobits += offset->endskip_x;
   }
}

static inline void
_bmap24lsb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
   l4_uint32_t nobits=0;
   int i,j, k,kmod;

   nobits += offset->preskip_y
      * (w + offset->preskip_x + offset->endskip_x);
   /* length of one line in bmap (bits!) */

   for (i = 0; i < h; i++) {
      nobits += offset->preskip_x;
      for (j = 0; j < w; j++, nobits++) {
	 k = nobits>>3;
	 kmod = (nobits)%8;
	 if ( bmap[k] & (0x01 << kmod) ) {
	    *(l4_uint16_t*) (&vfb[3*j]) = (l4_uint16_t) (fgc & 0xffff);
	    vfb[3*j+2] = (l4_uint8_t) (fgc >> 16);
	 }
	 else {
	    *(l4_uint16_t*) (&vfb[3*j]) = (l4_uint16_t) (bgc & 0xffff);
	    vfb[3*j+2] = (l4_uint8_t) (bgc >> 16);
	 }
      }
      vfb += bwidth;
   }
}

static inline void
_bmap24msb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
   l4_uint32_t nobits=0;
   int i,j, k,kmod;

   nobits += offset->preskip_y
      * (w + offset->preskip_x + offset->endskip_x);
   /* length of one line in bmap (bits!) */

   for (i = 0; i < h; i++) {
      nobits += offset->preskip_x;
      for (j = 0; j < w; j++, nobits++) {
	 k = nobits>>3;
	 kmod = (nobits)%8;
	 if ( bmap[k] & (0x80 >> kmod) ) {
	    *(l4_uint16_t*) (&vfb[3*j]) = (l4_uint16_t) (fgc & 0xffff);
	    vfb[3*j+2] = (l4_uint8_t) (fgc >> 16);
	 }
	 else {
	    *(l4_uint16_t*) (&vfb[3*j]) = (l4_uint16_t) (bgc & 0xffff);
	    vfb[3*j+2] = (l4_uint8_t) (bgc >> 16);
	 }
      }
      vfb += bwidth;
      /* length of one line in bmap parsed */
      nobits += offset->endskip_x;
   }
}

static inline void
_bmap32lsb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
   l4_uint32_t nobits=0;
   int i,j, k,kmod;

   nobits += offset->preskip_y
      * (w + offset->preskip_x + offset->endskip_x);
   /* length of one line in bmap (bits!) */

   for (i = 0; i < h; i++) {
      nobits += offset->preskip_x;
      for (j = 0; j < w; j++, nobits++) {
	 l4_uint32_t *dest = (l4_uint32_t*)&vfb[4*j];
	 k = nobits>>3;
	 kmod = (nobits)%8;
	 *dest = (bmap[k] & (0x01 << kmod))
	    ? fgc & 0xffffffff
	    : bgc & 0xffffffff;
      }
      vfb += bwidth;
   }
}

static inline void
_bmap32msb(l4_uint8_t *vfb,
	   l4_uint8_t *bmap,
	   l4_uint32_t fgc,
	   l4_uint32_t bgc,
	   l4_uint32_t w, l4_uint32_t h,
	   struct pslim_offset* offset,
	   l4_uint32_t bwidth)
{
   l4_uint32_t nobits=0;
   int i,j,k,kmod;

   nobits += offset->preskip_y
      * (w + offset->preskip_x + offset->endskip_x);
   /* length of one line in bmap (bits!) */

   for (i = 0; i < h; i++) {
      nobits += offset->preskip_x;
      for (j = 0; j < w; j++, nobits++) {
	 k = nobits>>3;
	 kmod = (nobits)%8;
	 if ( bmap[k] & (0x80 >> kmod) )
	    *(l4_uint32_t*) (&vfb[4*j]) = (l4_uint32_t) (fgc & 0x00ffffff);
	 else
	    *(l4_uint32_t*) (&vfb[4*j]) = (l4_uint32_t) (bgc & 0x00ffffff);
      }
      vfb += bwidth;
      /* length of one line in bmap parsed */
      nobits += offset->endskip_x;
   }
}

static inline void
_set16(l4_uint8_t *vfb,
       l4_uint8_t *pmap,
       l4_uint32_t w, l4_uint32_t h,
       struct pslim_offset* offset,
       l4_uint32_t bwidth,
       l4_uint32_t pwidth)
{
  int i;

#ifdef ARCH_x86
  if (use_fastmemcpy && (w % 4 == 0))
    {
      asm ("emms");
      for (i = 0; i < h; i++)
	{
	  l4_umword_t dummy;
	  pmap += 2 * offset->preskip_x;
	  asm volatile("xorl    %%edx,%%edx              \n\t"
		       "1:                               \n\t"
	               "movq    (%%esi,%%edx,8),%%mm0    \n\t"
	               "movntq  %%mm0,(%%edi,%%edx,8)    \n\t"
		       "add     $1,%%edx                 \n\t"
		       "dec     %%ecx                    \n\t"
		       "jnz     1b                       \n\t"
		       : "=c"(dummy), "=d"(dummy)
		       : "c"(w/4), "S"(pmap), "D"(vfb));
	  vfb  += bwidth;
	  pmap += pwidth;
	}
      asm ("sfence; emms");
    }
  else
#endif
    {
      for (i = 0; i < h; i++)
	{
	  pmap += 2 * offset->preskip_x;
	  memcpy(vfb, pmap, w*2);
	  vfb  += bwidth;
	  pmap += pwidth;
	}
    }
}

static inline void
_set24(l4_uint8_t *vfb,
       l4_uint8_t *pmap,
       l4_uint32_t w, l4_uint32_t h,
       struct pslim_offset* offset,
       l4_uint32_t bwidth,
       l4_uint32_t pwidth)
{
  int i;

  for (i = 0; i < h; i++)
    {
      pmap += 3 * offset->preskip_x;
      memcpy(vfb, pmap, w*3);
      vfb  += bwidth;
      pmap += pwidth;
    }
}

static inline void
_set32(l4_uint8_t *vfb,
       l4_uint8_t *pmap,
       l4_uint32_t w, l4_uint32_t h,
       struct pslim_offset* offset,
       l4_uint32_t bwidth,
       l4_uint32_t pwidth)
{
  int i;

  for (i = 0; i < h; i++)
    {
      pmap += 4 * offset->preskip_x;
      memcpy(vfb, pmap, w*4);
      vfb  += bwidth;
      pmap += pwidth;
   }
}

static inline void
_copy16(l4_uint8_t *vfb,
	l4_int16_t x, l4_int16_t y,
	l4_int16_t dx, l4_int16_t dy,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t bwidth)
{
  int i;
  l4_uint8_t *src = vfb, *dest = vfb;

  if (dy == y && dx == x)
    return;

  if (y >= dy)
    {
      OFFSET( x,  y, src,  2);
      OFFSET(dx, dy, dest, 2);
      for (i = 0; i < h; i++)
	{
	  /* memmove can deal with overlapping regions */
	  memmove(dest, src, 2*w);
	  src += bwidth;
	  dest += bwidth;
	}
    }
  else
    {
      OFFSET( x,  y + h - 1, src,  2);
      OFFSET(dx, dy + h - 1, dest, 2);
      for (i = 0; i < h; i++)
	{
	  /* memmove can deal with overlapping regions */
	  memmove(dest, src, 2*w);
	  src -= bwidth;
	  dest -= bwidth;
	}
    }
}

static inline void
_copy24(l4_uint8_t *vfb,
	l4_int16_t x, l4_int16_t y,
	l4_int16_t dx, l4_int16_t dy,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t bwidth)
{
   int i,j;
   l4_uint8_t *src = vfb, *dest = vfb;

   if (y >= dy) {
      if (y == dy && dx >= x) {	/* tricky */
	 if (x == dx)
	    return;
	 /* my way: start right go left */
	 OFFSET( x,  y, src,  3);
	 OFFSET(dx, dy, dest, 3);
	 for (i = 0; i < h; i++) {
	    for (j = w; j >= 0; --j) {
	        *(l4_uint16_t*) (&dest[3*j]) = *(l4_uint16_t*) (&src[3*j]);
		dest[3*j+2] = src[3*j+2];
	    }
	    src += bwidth;
	    dest += bwidth;
	 }

      }
      else {		/* copy from top to bottom */
	 OFFSET( x,  y, src,  3);
	 OFFSET(dx, dy, dest, 3);
	 for (i = 0; i < h; i++) {
	    for (j = 0; j < w; j++) {
	       *(l4_uint16_t*) (&dest[3*j]) = *(l4_uint16_t*) (&src[3*j]);
	       dest[3*j+2] = src[3*j+2];
	    }
	    src += bwidth;
	    dest += bwidth;
	 }
      }
   }
   else {		/* copy from bottom to top */
      OFFSET( x,  y + h, src,  3);
      OFFSET(dx, dy + h, dest, 3);
      for (i = 0; i < h; i++) {
	 src -= bwidth;
	 dest -= bwidth;
	 for (j = 0; j < w; j++) {
	    *(l4_uint16_t*) (&dest[3*j]) = *(l4_uint16_t*) (&src[3*j]);
	    dest[3*j+2] = src[3*j+2];
	 }
      }
   }
}

static inline void
_copy32(l4_uint8_t *vfb,
	l4_int16_t x, l4_int16_t y,
	l4_int16_t dx, l4_int16_t dy,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t bwidth)
{
   int i,j;
   l4_uint8_t *src = vfb, *dest = vfb;

   if (y >= dy) {
      if (y == dy && dx >= x) {	/* tricky */
	 if (x == dx)
	    return;
	 /* my way: start right go left */
	 OFFSET( x,  y, src,  4);
	 OFFSET(dx, dy, dest, 4);
	 for (i = 0; i < h; i++) {
	    for (j = w; j >= 0; --j)
	       *(l4_uint32_t*) (&dest[4*j]) = *(l4_uint32_t*) (&src[4*j]);
	    src += bwidth;
	    dest += bwidth;
	 }

      }
      else {		/* copy from top to bottom */
	 OFFSET( x,  y, src,  4);
	 OFFSET(dx, dy, dest, 4);
	 for (i = 0; i < h; i++) {
	    for (j = 0; j < w; j++)
	       *(l4_uint32_t*) (&dest[4*j]) = *(l4_uint32_t*) (&src[4*j]);
	    src += bwidth;
	    dest += bwidth;
	 }
      }
   }
   else {		/* copy from bottom to top */
      OFFSET( x,  y + h, src,  4);
      OFFSET(dx, dy + h, dest, 4);
      for (i = 0; i < h; i++) {
	 src -= bwidth;
	 dest -= bwidth;
	 for (j = 0; j < w; j++)
	    *(l4_uint32_t*) (&dest[4*j]) = *(l4_uint32_t*) (&src[4*j]);
      }
   }
}

static inline void
_fill16(l4_uint8_t *vfb,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t color,
	l4_uint32_t bwidth)
{
  int i,j;

  for (i = 0; i < h; i++)
    {
      for (j = 0; j < w; j++)
	*(l4_uint16_t*) (&vfb[2*j]) = (l4_uint16_t)color;
      vfb += bwidth;
    }
}

static inline void
_fill24(l4_uint8_t *vfb,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t color,
	l4_uint32_t bwidth)
{
   int i,j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
	 *(l4_uint16_t*) (&vfb[3*j  ]) = (l4_uint16_t)color;
	                   vfb[3*j+2]  = (l4_uint8_t) (color >> 16);
      }
      vfb += bwidth;
   }
}

static inline void
_fill32(l4_uint8_t *vfb,
	l4_uint32_t w, l4_uint32_t h,
	l4_uint32_t color,
	l4_uint32_t bwidth)
{
   int i,j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++)
	 *(l4_uint32_t*) (&vfb[4*j]) = (l4_uint32_t)color;
      vfb += bwidth;
   }
}

void
sw_fill(struct l4con_vc *vc, int x, int y, int w, int h, unsigned color)
{
  l4_uint8_t *vfb = (l4_uint8_t*) vc->fb;
  l4_uint32_t bwidth = vc->bytes_per_line;

  OFFSET(x, y, vfb, vc->bytes_per_pixel);

  /* wait for any pending acceleration operation */
  vc->do_sync();

  switch(vc->gmode & GRAPH_BPPMASK)
    {
    case GRAPH_BPP_24:
      _fill24(vfb, w, h, color, bwidth);
      break;
    case GRAPH_BPP_32:
      _fill32(vfb, w, h, color, bwidth);
      break;
    case GRAPH_BPP_16:
    default:
      _fill16(vfb, w, h, color, bwidth);
    }

  /* force redraw of changed screen content (needed by VMware) */
  if (vc->do_drty)
    vc->do_drty(x, y, w, h);
}

static inline void
sw_bmap(struct l4con_vc *vc, l4_int16_t x, l4_int16_t y, l4_uint32_t w,
	l4_uint32_t h, l4_uint8_t *bmap, l4_uint32_t fgc, l4_uint32_t bgc,
        struct pslim_offset* offset, l4_uint8_t mode)
{
  l4_uint8_t *vfb = (l4_uint8_t*) vc->fb;
  l4_uint32_t bwidth = vc->bytes_per_line;

  OFFSET(x, y, vfb, vc->bytes_per_pixel);

  /* wait for any pending acceleration operation */
  vc->do_sync();

  switch (mode)
    {
    case pSLIM_BMAP_START_MSB:
      switch(vc->gmode & GRAPH_BPPMASK)
	{
	case GRAPH_BPP_32:
	  _bmap32msb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	  break;
	case GRAPH_BPP_24:
	  _bmap24msb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	  break;
	case GRAPH_BPP_16:
	default:
	  _bmap16msb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	}
      break;
    case pSLIM_BMAP_START_LSB:
    default:	/* `start at least significant' bit is default */
      switch(vc->gmode & GRAPH_BPPMASK)
	{
	case GRAPH_BPP_32:
	  _bmap32lsb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	  break;
        case GRAPH_BPP_24:
	  _bmap24lsb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	  break;
	case GRAPH_BPP_16:
	default:
	  _bmap16lsb(vfb, bmap, fgc, bgc, w, h, offset, bwidth);
	}
    }

  /* force redraw of changed screen content (needed by VMware) */
  if (vc->do_drty)
    vc->do_drty(x, y, w, h);
}

static inline void
sw_set(struct l4con_vc *vc, l4_int16_t x, l4_int16_t y, l4_uint32_t w,
       l4_uint32_t h, l4_uint32_t xoffs, l4_uint32_t yoffs,
       l4_uint8_t *pmap, struct pslim_offset* offset)
{
  l4_uint8_t *vfb = (l4_uint8_t*) vc->fb;
  l4_uint32_t bytepp = vc->bytes_per_pixel;
  l4_uint32_t bwidth = vc->bytes_per_line;
  l4_uint32_t pwidth;

  OFFSET(x+xoffs, y+yoffs, vfb, bytepp);

  if (!pmap)
    {
      /* copy from direct mapped framebuffer of client */
      /* bwidth may be different from xres*bytepp: e.g. VMware */
      pwidth = vc->xres*bytepp;
      pmap   = vc->vfb + y*pwidth + x*bytepp;
    }
  else
    {
      pwidth = bytepp * (w + offset->endskip_x);
      pmap  += bytepp * offset->preskip_y *
		        (w + offset->preskip_x + offset->endskip_x);
    }

  /* wait for any pending acceleration operation */
  vc->do_sync();

  switch(vc->gmode & GRAPH_BPPMASK)
    {
    case GRAPH_BPP_32:
      _set32(vfb, pmap, w, h, offset, bwidth, pwidth);
      break;
    case GRAPH_BPP_24:
      _set24(vfb, pmap, w, h, offset, bwidth, pwidth);
      break;
    case GRAPH_BPP_16:
    default:
      _set16(vfb, pmap, w, h, offset, bwidth, pwidth);
    }

  /* force redraw of changed screen content (needed by VMware) */
  if (vc->do_drty)
    vc->do_drty(x+xoffs, y+yoffs, w, h);
}

void
sw_copy(struct l4con_vc *vc, int x, int y, int w, int h, int dx, int dy)
{
  l4_uint8_t *vfb = (l4_uint8_t*) vc->fb;
  l4_uint32_t bwidth = vc->bytes_per_line;

  /* wait for any pending acceleration operation */
  vc->do_sync();

  switch(vc->gmode & GRAPH_BPPMASK)
    {
    case GRAPH_BPP_32:
      _copy32(vfb, x, y, dx, dy, w, h, bwidth);
      break;
    case GRAPH_BPP_24:
      _copy24(vfb, x, y, dx, dy, w, h, bwidth);
      break;
    case GRAPH_BPP_16:
    default:
      _copy16(vfb, x, y, dx, dy, w, h, bwidth);
    }

  /* force redraw of changed screen content (needed by VMware) */
  if (vc->do_drty)
    vc->do_drty(dx, dy, w, h);
}

static inline void
sw_cscs(struct l4con_vc *vc, l4_int16_t x, l4_int16_t y, l4_uint32_t w,
	l4_uint32_t h, l4_uint8_t *Y, l4_uint8_t *U, l4_uint8_t *V,
	l4_uint32_t scale, struct pslim_offset* offset, l4_uint8_t mode)
{
  /* wait for any pending acceleration operation */
  vc->do_sync();

  /* we exchange U and V here because of video format 420 */
  (*yuv2rgb_render)(vc->fb+y*vc->bytes_per_line+x*vc->bytes_per_pixel,
		    Y, U, V, w, h, vc->bytes_per_line, w, w/2);

  /* force redraw of changed screen content (needed by VMware) */
  if (vc->do_drty)
    vc->do_drty(x, y, w, h);
}

/* SVGAlib calls this: FILLBOX */
void
pslim_fill(struct l4con_vc *vc, int from_user,
	   l4con_pslim_rect_t *rect, l4con_pslim_color_t color)
{
  if (!clip_rect(vc, from_user, rect))
    /* nothing visible */
    return;

  if (!rect->w || !rect->h)
    /* nothing todo */
    return;

  vc->do_fill(vc, rect->x+vc->pan_xofs, rect->y+vc->pan_yofs,
                  rect->w, rect->h, color);
}

/* SVGAlib calls this:	ACCEL_PUTBITMAP (mode 2)
			PBM - images (mode 1)		*/
void
pslim_bmap(struct l4con_vc *vc, int from_user, l4con_pslim_rect_t *rect,
	   l4con_pslim_color_t fgc, l4con_pslim_color_t bgc, void* bmap,
	   l4_uint8_t mode)
{
  struct pslim_offset offset = {0,0,0};

  if (!clip_rect_offset(vc, from_user, rect, &offset))
    /* nothing visible */
    return;

  if (!rect->w || !rect->h)
    /* nothing todo */
    return;

  sw_bmap(vc, rect->x+vc->pan_xofs, rect->y+vc->pan_yofs, rect->w, rect->h,
              bmap, fgc, bgc, &offset, mode);
}

/* SVGAlib calls this:	PUTBOX  (and knows about masked boxes!) */
void
pslim_set(struct l4con_vc *vc, int from_user, l4con_pslim_rect_t *rect,
	  void* pmap)
{
  struct pslim_offset offset = {0,0,0};

  if (!clip_rect_offset(vc, from_user, rect, &offset))
    /* nothing visible */
    return;

  if (!rect->w || !rect->h)
    /* nothing todo */
    return;

  sw_set(vc, rect->x, rect->y, rect->w, rect->h,
             vc->pan_xofs, vc->pan_yofs, (l4_uint8_t*) pmap, &offset);
}

/* SVGAlib calls this:	COPYBOX */
void
pslim_copy(struct l4con_vc *vc, int from_user, l4con_pslim_rect_t *rect,
	   l4_int16_t dx, l4_int16_t dy)
{
  if (!clip_rect_dxy(vc, from_user, rect, &dx, &dy))
    /* nothing visible */
    return;

  if (!rect->w || !rect->h)
    /* nothing todo */
    return;

  vc->do_copy(vc, rect->x+vc->pan_xofs, rect->y+vc->pan_yofs, rect->w, rect->h,
                  dx+vc->pan_xofs, dy+vc->pan_yofs);
}

/* COLOR-SPACE CONVERT and (optional) SCALE
 *
 * mode ... Specifies the resolution of chrominance data
 *
 *	Code     Meaning
 *	----     ------- ----------------------------------------
 *	PLN_420  4:2:0   half resolution in both dimensions (most common format)
 *	PLN_422  4:2:2   half resolution in horizontal direction
 *	PLN_444  4:4:4   full resolution
 */
void
pslim_cscs(struct l4con_vc *vc, int from_user, l4con_pslim_rect_t *rect,
	   void* y, void* u, void *v, l4_uint8_t mode, l4_uint32_t scale)
{
  struct pslim_offset offset = {0,0,0};

  if (!clip_rect_offset(vc, from_user, rect, &offset))
    /* nothing visible */
    return;

  if (!rect->w || !rect->h)
    /* nothing todo */
    return;

  sw_cscs(vc, rect->x+vc->pan_xofs, rect->y+vc->pan_yofs, rect->w, rect->h,
          y, u, v, scale, &offset, mode);
}
