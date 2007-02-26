/*
 * \brief	DOpE 16bit image data module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

#include "dope-config.h"
#include "memory.h"
#include "cache.h"
#include "screen.h"
#include "clipping.h"
#include "img16data.h"

static struct memory_services *mem;
static struct clipping_services *clip;
static struct screen_services *scr;

static s32 scr_width,scr_height,scr_depth;
static u16 *scr_adr;
static s32 ident_cnt;

int init_image16data(struct dope_services *d);



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** DUMMY PAINT FUNCTION (COMPATIBLE WITH ALL SCREEN MODES) ***/
static void paint_dummy(s32 x,s32 y,struct image16 *img) {}


/*** DRAW CLIPPED 16BIT IMAGE ON 16BIT SCREEN ***/
static void paint_16(s32 x,s32 y,struct image16 *img) {
	long clip_x1=clip->get_x1();
	long clip_y1=clip->get_y1();
	long clip_x2=clip->get_x2();
	long clip_y2=clip->get_y2();
	static s32 i,j,w,h;
	static u16 *src,*dst,*s,*d;
	
	if (!img) return;
	w	= img->w;
	h	= img->h;
	src	= img->pixels;
	dst	= scr_adr + y*scr_width + x;
	
	/* left clipping */
	if (x < clip_x1) {
		w 	-= (clip_x1-x);
		src += (clip_x1-x);
		dst += (clip_x1-x);
		x=clip_x1;
	}
	
	/* right clipping */
	if (x+w-1 > clip_x2) {
		w	-= (x+w-1 - clip_x2);
	}
	
	/* top clipping */
	if (y < clip_y1) {
		h 	-= (clip_y1-y);
		src += (clip_y1-y)*img->w;
		dst += (clip_y1-y)*scr_width;
		y=clip_y1;	
	}

	/* bottom clipping */
	if (y+h-1 > clip_y2) {
		h	-= (y+h-1 - clip_y2);
	}
		
	/* anything left? */
	if ((w<0) || (h<0)) return;
	
	/* paint... */
	for (j=h;j--;) {

		/* copy line from image to screen */
		for (i=w,s=src,d=dst;i--;*(d++)=*(s++));
		src+=img->w;
		dst+=scr_width;
	}
}


/*** DUMMY FOR DRAWING SCALED AND CLIPPED 16BIT IMAGE ***/
static void paint_scaled_dummy(s32 x,s32 y,s32 w,s32 h,struct image16 *img) {
}


static s32 scale_xbuf[2000];

/*** DRAW SCALED AND CLIPPED 16BIT IMAGE ***/
static void paint_scaled_16(s32 x,s32 y,s32 sw,s32 sh,struct image16 *img) {
	long clip_x1=clip->get_x1();
	long clip_y1=clip->get_y1();
	long clip_x2=clip->get_x2();
	long clip_y2=clip->get_y2();
	static float mx,my;
	static long i,j,w,h;
	static float sx,sy;
	static u16 *src,*dst,*s,*d;
	
	if (!img) return;
	
	if (sw) mx = (float)img->w / (float)sw;
	else mx=0.0;
	
	if (sh) my = (float)img->h / (float)sh;
	else my=0.0;

	src	= img->pixels;
	dst	= scr_adr + y*scr_width + x;
	sy=0.0;
	sx=0.0;
	w=sw;
	h=sh;

	/* left clipping */
	if (x < clip_x1) {
		w 	-= (clip_x1-x);
		sx  += (float)(clip_x1-x) * mx;
		dst += (clip_x1-x);
		x=clip_x1;
	}
	
	/* right clipping */
	if (x+w-1 > clip_x2) {
		w	-= (x+w-1 - clip_x2);
	}
	
	/* top clipping */
	if (y < clip_y1) {
		h 	-= (clip_y1-y);
		sy  += (float)(clip_y1-y) * my;
		dst += (clip_y1-y)*scr_width;
		y=clip_y1;	
	}

	/* bottom clipping */
	if (y+h-1 > clip_y2) {
		h	-= (y+h-1 - clip_y2);
	}
		
	/* anything left? */
	if ((w<0) || (h<0)) return;
	
	/* calculate x offsets */
	for (i=w;i--;) {
		scale_xbuf[i]=(long)sx;
		sx += mx;
	}
	
	/* draw scaled image */
	for (j=h;j--;) {
		
		s=src + ((long)sy*img->w);
		d=dst;
		for (i=w;i--;) *(d++) = *(s + scale_xbuf[i]);
		sy += my;
		dst+= scr_width;	
	}
}


/*** CREATE AN 16-BIT IMAGE WITH THE SPECIFIED SIZE ***/
static struct image16 *create(s32 width,s32 height) {
	struct image16 *new;

	new = (struct image16 *)mem->alloc(2*width*height + sizeof(struct image16));
	if (!new) {
		DOPEDEBUG(printf("Image16Data(create): out of memory\n");)
		return NULL;		
	}
	
	new->w=width;
	new->h=height;
	new->pixels = (u16 *)((long)new + sizeof(struct image16));
	new->cache_index=999999;
	
	/* give new image an identity */
	new->cache_ident=ident_cnt++;

	return new;
}


/*** DESTROY 16-BIT IMAGE ***/
static void destroy(struct image16 *img) {
	if (!img) return;
	mem->free(img);
}


/*** UPDATE MODULE PROPERTIES (FOR SCREEN MODE CHANGES ETC.) ***/
static struct image16_services services;
static void update_properties(void) {
	scr_width=scr->get_scr_width();
	scr_height=scr->get_scr_height();
	scr_depth=scr->get_scr_depth();
	scr_adr=scr->get_buf_adr();
		
	/* set screen mode dependent methods */
	switch (scr_depth) {
	case 16:
		services.paint=paint_16;
		services.paint_scaled=paint_scaled_16;
		break;
	}
}


/*** DETERMINE SIZE OF IMAGE IN BYTES ***/
static s32 size_of_img16(struct image16 *img) {
	if (!img) return 0;
	return img->w*img->h*2 + sizeof(struct image16);
}


/*** CLEAR IMAGE ***/
static void clear(struct image16 *img) {
	long i;
	if (!img) return;
	for (i=img->w*img->h;i--;) *(img->pixels+i)=0;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct image16_services services = {
	update_properties,
	create,
	destroy,
	paint_dummy,
	paint_scaled_dummy,
	size_of_img16,
	clear,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_image16data(struct dope_services *d) {
	
	mem =	d->get_module("Memory 1.0");
	scr	=	d->get_module("Screen 1.0");
	clip=	d->get_module("Clipping 1.0");
		
	d->register_module("Image16Data 1.0",&services);
	return 1;
}
