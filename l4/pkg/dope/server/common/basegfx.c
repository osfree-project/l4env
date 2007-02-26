/*
 * \brief	DOpE basegfx module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 * 
 * This  module contains general  drawing functions
 * that should  be used  by widgets to draw  frames    
 * and boxes. There are only a limited set of basic 
 * primitives  (only a  few box  and frame  styles)     
 * The  detailed  look of  the boxes  and frames is     
 * encapsulated  in this  module. By  exchanging or    
 * configuring this module, the look of all widgets 
 * which use its functions will change.             
 */


#include "dope-config.h"
#include "screen.h"
#include "clipping.h"
#include "fontman.h"
#include "basegfx.h"

#define MAX(a,b) a>b?a:b
#define MIN(a,b) a<b?a:b
#define WHITE 0xffff
#define BLACK 0x0000
#define FG_RED 0x
#define DARKGRAY 0x7bef
#define LIGHTGRAY 0xc618

#define COL16(r,g,b) (r<<11) + (g<<6) + b

static struct screen_services *scr;
static struct clipping_services *clip;
static struct fontman_services *fontman;

static u16  scr_w=0;
static u16  scr_h=0;
static u16 *scr_adr=(void *)0;


int init_basegfx(struct dope_services *d);

/*************************/
/*** PRIVATE FUNCTIONS ***/
/*************************/


/*** DRAW A SOLID HORIZONTAL LINE IN 16BIT COLOR MODE ***/
static void solid_hline_16(short *dst,u32 width,u16 col) {
	for (width++;width--;) *(dst++) = col;
}


/*** DRAW A SOLID VERTICAL LINE IN 16BIT COLOR MODE ***/
static void solid_vline_16(u16 *dst,u32 height,u32 scr_w,u16 col) {
	dst-=scr_w;height++;
	for (;height--;) *(dst+=scr_w) = col;
}


/*** DRAW A TRANSPARENT HORIZONTAL LINE IN 16BIT COLOR MODE ***/
static void mix_hline_16(u16 *dst,u32 width,u16 mixcol) {
	mixcol=(mixcol&0xf7de)>>1;
	for (width++;width--;) *(dst++) = (((*dst)&0xf7de)>>1) + mixcol;
}


/*** DRAW A TRANSPARENT VERTICAL LINE IN 16BIT COLOR MODE ***/
static void mix_vline_16(u16 *dst,u32 height,u32 scr_w,u16 mixcol) {
	mixcol=(mixcol&0xf7de)>>1;
	dst-=scr_w;height++;
	for (;height--;) *(dst+=scr_w) = (((*dst)&0xf7de)>>1) + mixcol;
}


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** SCREEN INDEPENDENT DUMMY FUNCTIONS FOR INIT OF SERVICE STRUCTURE ***/
static void draw_box_dummy   (s16 x1,s16 y1,s16 x2,s16 y2,u16 style) {}
static void draw_frame_dummy (s16 x1,s16 y1,s16 x2,s16 y2,u16 style) {}
static void draw_string_dummy(s16 x,s16 y,u32 font_id,u16 style,u8 *str) {}
static void draw_ansi_dummy  (s16 x,s16 y,u32 font_id,u8 *str,u8 *bgfg) {}



/*** DRAW SOLID BOXES IN 16BIT MODE ***/
static void draw_solid_box_16(s16 x1,s16 y1,s16 x2,s16 y2,u16 style) {
	s16 clip_x1=clip->get_x1();
	s16 clip_y1=clip->get_y1();
	s16 clip_x2=clip->get_x2();
	s16 clip_y2=clip->get_y2();
	static s16 x,y;
	static u16 *dst,*dst_line;
	static u16 color;
	
	/* check clipping */
	if (x1<clip_x1) x1=clip_x1;
	if (y1<clip_y1) y1=clip_y1;
	if (x2>clip_x2) x2=clip_x2;
	if (y2>clip_y2) y2=clip_y2;
	if (x1>x2) return;
	if (y1>y2) return;
	
	switch (style) {
		case GFX_BOX_DESKBG:color = COL16(10,12,13);break;
		case GFX_BOX_WINBG:	color = COL16(16,17,18);break;
		case GFX_BOX_ACT: 	color = COL16(15,16,20);break;
		case GFX_BOX_SEL: 	color = COL16(15,15,15);break;
		case GFX_BOX_FOCUS:	color = COL16(17,18,22);break;
		default: return;
	}
	
	/* draw */
	dst_line=scr_adr+scr_w*y1;
	for (y=y1;y<=y2;y++) {
		dst=dst_line + x1;
		for (x=x1;x<=x2;x++) {
			*(dst++)=color;
		}
		dst_line+=scr_w;
	}
}


/*** DRAW FRAMES OF DIFFERENT STYLES ***/
static void draw_frame_16(s16 x1,s16 y1,s16 x2,s16 y2,u16 style) {
	s16 clip_x1=clip->get_x1();
	s16 clip_y1=clip->get_y1();
	s16 clip_x2=clip->get_x2();
	s16 clip_y2=clip->get_y2();
	u16 lt_col=BLACK,rb_col=WHITE;
	static s16 cx1,cy1,cx2,cy2;

	switch (style) {
		case GFX_FRAME_RAISED: 
			lt_col=WHITE;
			rb_col=BLACK;
		case GFX_FRAME_SUNKEN:
		
			/* draw outer frame */
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y2<=clip_y2) solid_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,rb_col);
			if (x2<=clip_x2) solid_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,rb_col);
			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);

			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2-1,clip_x2);cy2=MIN(y2-1,clip_y2);

			/* draw inner frame */
			x1++;y1++;x2--;y2--;

			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);

			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);

			if (y2<=clip_y2) mix_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,rb_col);
			if (x2<=clip_x2) mix_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,rb_col);

			x2=x1+1;y2=y1+1;
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;

			if (y1>=clip_y1) solid_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) solid_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);

			return;
		case GFX_FRAME_RIDGE:
			lt_col=WHITE;
			rb_col=BLACK;
		case GFX_FRAME_GROOVE:
		
			/* draw outer frame */
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (y2<=clip_y2) mix_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,rb_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);
			if (x2<=clip_x2) mix_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,rb_col);
	
			/* draw inner frame */
			x1++;y1++;x2--;y2--;

			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,rb_col);
			if (y2<=clip_y2) mix_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,rb_col);
			if (x2<=clip_x2) mix_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,lt_col);
			return;
		case GFX_FRAME_PRESSED:
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) solid_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (y2<=clip_y2) mix_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,rb_col);
			if (x1>=clip_x1) solid_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);
			if (x2<=clip_x2) mix_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,rb_col);
	
			x1++;y1++;x2--;y2--;
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) solid_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) solid_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);

			x1++;y1++;x2--;y2--;
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;

			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);
			return;
		case GFX_FRAME_FOCUS:
			lt_col=WHITE;
			rb_col=BLACK;
		
			/* draw outer frame */
			cx1=MAX(x1,clip_x1);cy1=MAX(y1,clip_y1);cx2=MIN(x2,clip_x2);cy2=MIN(y2,clip_y2);
			if (cx1>cx2) return;
			if (cy1>cy2) return;
	
			if (y1>=clip_y1) mix_hline_16(scr_adr+scr_w*y1+cx1,cx2-cx1,lt_col);
			if (y2<=clip_y2) solid_hline_16(scr_adr+scr_w*y2+cx1,cx2-cx1,rb_col);
			if (x1>=clip_x1) mix_vline_16(scr_adr+scr_w*cy1+x1,cy2-cy1,scr_w,lt_col);
			if (x2<=clip_x2) solid_vline_16(scr_adr+scr_w*cy1+x2,cy2-cy1,scr_w,rb_col);
	
			return;
			
	}
}


/*** DRAW A STRING IN 16BIT COLORDEPTH ***/
static void draw_string_16(s16 x,s16 y,u32 font_id,u16 style,u8 *str) {
	struct font_struct *font = fontman->get_by_id(font_id);
	s16 clip_x1=clip->get_x1();
	s16 clip_y1=clip->get_y1();
	s16 clip_x2=clip->get_x2();
	s16 clip_y2=clip->get_y2();
	s32 *wtab=font->width_table;
	s32 *otab=font->offset_table;
	s32 img_w=font->img_w;
	s32 img_h=font->img_h;
	u16 *dst = scr_adr + y*scr_w + x;
	u8  *src = font->image;
	u8  *s;
	u16 *d;
	s16 i,j;
	s32 w;
	s32 h = font->img_h;
	u16 color=-1;
	if (style==GFX_STRING_BLACK) color=0;
	
	if (!str) return;
	
	/* check top clipping */
	if (y<clip_y1) {
		src+= (clip_y1-y)*img_w;	/* skip upper lines in font image */
		h-= (clip_y1-y);			/* decrement number of lines to draw */
		dst+= (clip_y1-y)*scr_w;
	}
	
	/* check bottom clipping */
	if (y+img_h-1>clip_y2) {
		h-= (y+img_h-1-clip_y2);	/* decrement number of lines to draw */
	}
	
	if (h<1) return;
	
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
			d=d+scr_w;
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
			d=d+scr_w;
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
		if (x<clip_x1) {	/* check if character is also left-cutted */
			w-=(clip_x1-x);
			s+=(clip_x1-x);
			d+=(clip_x1-x);
		}		
		for (j=0;j<h;j++) {
			for (i=0;i<w;i++) {
				if (*(s+i)) *(d+i)=color;
			}
			s=s+img_w;
			d=d+scr_w;
		}
	}	
}


static u16 coltab_16[16] = {
	COL16( 0, 0, 0),COL16(15, 0, 0),COL16( 0,15, 0),COL16(15,15, 0),
	COL16( 0, 0,15),COL16(15, 0,15),COL16( 0,15,15),COL16(15,15,15),
	COL16( 0, 0, 0),COL16(31, 0, 0),COL16( 0,31, 0),COL16(31,31, 0),
	COL16( 0, 0,31),COL16(31, 0,31),COL16( 0,31,31),COL16(31,31,31),
};


/*** DRAW AN ANSI STRING IN 16BIT COLORDEPTH ***/
static void draw_ansi_16(s16 x,s16 y,u32 font_id,u8 *str,u8 *bgfg) {
	struct font_struct *font = fontman->get_by_id(font_id);
	s16 clip_x1=clip->get_x1();
	s16 clip_y1=clip->get_y1();
	s16 clip_x2=clip->get_x2();
	s16 clip_y2=clip->get_y2();
	s32 *wtab=font->width_table;
	s32 *otab=font->offset_table;
	s32 img_w=font->img_w;
	s32 img_h=font->img_h;
	u16 *dst = scr_adr + y*scr_w + x;
	u8  *src = font->image;
	u8  *s;
	u16 *d;
	s16 i,j;
	s32 w;
	s32 h = font->img_h;
	u16 fg_col=-1;
	u16 bg_col=-1;
	
	if (!str) return;
	
	/* check top clipping */
	if (y<clip_y1) {
		src+= (clip_y1-y)*img_w;	/* skip upper lines in font image */
		h-= (clip_y1-y);			/* decrement number of lines to draw */
		dst+= (clip_y1-y)*scr_w;
	}
	
	/* check bottom clipping */
	if (y+img_h-1>clip_y2) {
		h-= (y+img_h-1-clip_y2);	/* decrement number of lines to draw */
	}
	
	if (h<1) return;
	
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
			d=d+scr_w;
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
			d=d+scr_w;
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
		if (x<clip_x1) {	/* check if character is also left-cutted */
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
			d=d+scr_w;
		}
	}	
}





/* PROTOTYPE OF SERVICE STRUCTURE */
static struct basegfx_services services;


/*** UPDATE THE PROPERTIES OF THE BASEGFX MODULE ***/
static void update_properties(void) {

	/* register drawing functions dependent on curren color depth */
	switch (scr->get_scr_depth()) {
		case 16:
			services.draw_box    = draw_solid_box_16;
			services.draw_frame  = draw_frame_16;
			services.draw_string = draw_string_16;
			services.draw_ansi   = draw_ansi_16;
			break;
	}
	
	scr_adr	= scr->get_buf_adr();
	scr_w	= scr->get_scr_width();
	scr_h	= scr->get_scr_height();
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct basegfx_services services = {
	draw_box_dummy,
	draw_frame_dummy,
	draw_string_dummy,
	draw_ansi_dummy,
	update_properties
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_basegfx(struct dope_services *d) {
	
	scr		= d->get_module("Screen 1.0");
	clip	= d->get_module("Clipping 1.0");
	fontman	= d->get_module("FontManager 1.0");
	
	d->register_module("Basegfx 1.0",&services);
	return 1;
}
