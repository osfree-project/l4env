/*
 * \brief	DOpE 8bit indexed image module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Component, that provides functions to handle
 * 8bit indexed images.
 */


#include "dope-config.h"
#include "memory.h"
#include "cache.h"
#include "screen.h"
#include "img16data.h"
#include "img8idata.h"

static struct memory_services *mem;
static struct screen_services *scr;
static struct cache_services *cache;
static struct image16_services *img16;

static s32 scr_depth=0;
static s32 ident_cnt=42;
static CACHE *imgcache=NULL;

int init_image8idata(struct dope_services *d);



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** INITIALISE IMAGE CACHE WITH NEW CACHE PARAMETERS ***/
static void set_cache(s32 max_elem,s32 max_size) {
	if (imgcache) cache->destroy(imgcache);
	imgcache=cache->create(max_elem,max_size);
}


/*** DUMMY FUNCTION (COMPATIBLE WITH ANY COLOR DEPTH ,-) ***/
static void paint_dummy(s32 x,s32 y,struct image8i *img) {
	DOPEDEBUG(printf("Image8iData(paint_dummy)\n");)
}


/*** CONVERT AN INDEXED 8-BIT IMAGE TO A 16BIT HICOLOR IMAGE ***/
static u16 coltab[256];
static struct image16 *convert_8i_to_16(struct image8i *img8i) {
	struct image16 *new;
	u32 col24;
	s32 *src_col;
	s32 i;
	u16 *dst_pix;
	u8  *src_idx;

	DOPEDEBUG(printf("Image8iData(convert_8i_to_16)\n");)
	new = img16->create(img8i->w,img8i->h);
	
	/* convert color table from 24bit to 16bit */
	src_col = img8i->palette;
	for (i=0;i<256;i++) {
		col24=*(src_col++);
		coltab[i] = ((col24&0xf8000000)>>(16))  |
					((col24&0x00fc0000)>>(8+5)) |
					((col24&0x0000f800)>>(8+3));
	}
	
	/* convert pixels using color table */
	src_idx = img8i->pixels;
	dst_pix = new->pixels;
	for (i=img8i->w*img8i->h; i--;) {
		*(dst_pix++) = coltab[*(src_idx++)];
	}
	return new;
}


static void imgcache_destroy_dummy(void *elem) {
	DOPEDEBUG(printf("Image8iData(cache_destroy_dummy): How did you get in here???\n");)
}


/*** POINTER TO CACHE ELEMENT DESTROY FUNCTION ***/
/* This function depends on the used screen mode - so we use this */
/* additional indirection to be flexible */
static void (*imgcache_destroy) (void *elem) = imgcache_destroy_dummy;


/*** PAINT AN 8-BIT INDEXED IMAGE IN 16BIT COLORDEPTH ***/
static void paint_16(s32 x,s32 y,struct image8i *img) {
	struct image16 *i16;

	if (!img) return;

	/* check if cache contains converted image */	
	i16 = cache->get_elem(imgcache,img->cache_index,img->cache_ident);
	if (!i16) {
		i16 = convert_8i_to_16(img);
		img->cache_index = cache->add_elem(imgcache,i16,img16->size_of(i16),
										   img->cache_ident,imgcache_destroy);
	}
	
	/* i16 is the 16bit image to display now... */
	img16->paint(x,y,i16);	
}



/*** DUMMY FUNCTION (COMPATIBLE WITH ANY COLOR DEPTH ,-) ***/
static void paint_scaled_dummy(s32 x,s32 y,s32 w,s32 h,struct image8i *img) {
	DOPEDEBUG(printf("Image8iData(paint_scaled_dummy)\n");)
}


/*** PAINT A SCALED 8-BIT INDEXED IMAGE IN 16BIT COLORDEPTH ***/
static void paint_scaled_16(s32 x,s32 y,s32 w,s32 h,struct image8i *img) {
	struct image16 *i16;
	if (!img) return;

	/* check if cache contains converted image */	
	i16 = cache->get_elem(imgcache,img->cache_index,img->cache_ident);
	if (!i16) {
		i16 = convert_8i_to_16(img);
		img->cache_index = cache->add_elem(imgcache,i16,img16->size_of(i16),
										   img->cache_ident,imgcache_destroy);
	}
	
	/* i16 is the 16bit image to display now... */	
	img16->paint_scaled(x,y,w,h,i16);	
}


/*** SYNC CACHE WITH IMAGE CHANGES ***/
static void refresh(struct image8i *img) {

	/* we just kill the corresponding cache element, hehe */
	cache->remove_elem(imgcache,img->cache_index);
}


/*** UPDATE MODULE PROPERTIES (FOR SCREEN MODE CHANGES ETC.) ***/
static struct image8i_services services ;
static void update_properties(void) {
	scr_depth=scr->get_scr_depth();
	switch (scr_depth) {
	case 16:
		services.paint=paint_16;
		services.paint_scaled=paint_scaled_16;
		imgcache_destroy=(void (*)(void *))(img16->destroy);
		break;
	}
} 


/*** CREATE AN 8-BIT IMAGE WITH THE SPECIFIED SIZE ***/
static struct image8i *create(s32 width,s32 height) {
	struct image8i *new;

	new = (struct image8i *)mem->alloc(width*height + sizeof(struct image8i) + 4*256);	
	if (!new) {
		DOPEDEBUG(printf("Image8iData(create): out of memory\n");)
		return NULL;
	}

	new->w=width;
	new->h=height;
	new->palette= (u32 *)((long)new + sizeof(struct image8i));
	new->pixels = (u8  *)((long)new + sizeof(struct image8i) + 4*256);
	new->cache_index=999999;
	
	/* give new image an identity */
	new->cache_ident=ident_cnt++;

	return new;
}


/*** DESTROY 8-BIT IMAGE ***/
static void destroy(struct image8i *img) {
	if (!img) return;
	
	/* cleanup image cache */
	cache->remove_elem(imgcache,img->cache_index);
	
	/* free image memory */
	mem->free(img);
}


/*** DETERMINE SIZE OF IMAGE IN BYTES ***/
static s32 size_of_img8i(struct image8i *img) {
	if (!img) return 0;
	return img->w*img->h + sizeof(struct image8i) + 4*256;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct image8i_services services = {
	set_cache,
	update_properties,
	paint_dummy,
	paint_scaled_dummy,
	refresh,
	create,
	destroy,
	size_of_img8i
}; 



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_image8idata(struct dope_services *d) {
	
	scr		=	d->get_module("Screen 1.0");
	cache	=	d->get_module("Cache 1.0");
	mem		= 	d->get_module("Memory 1.0");
	img16	=	d->get_module("Image16Data 1.0");
	
	imgcache = cache->create(100,200*1000);
	d->register_module("Image8iData 1.0",&services);
	return 1;
}
