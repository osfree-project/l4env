/*
 * \brief	DOpE screen module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component  is the screen  output interface of
 * DOpE. It handles two frame buffers: the physically
 * visible frame buffer and a  background buffer. All
 * graphics  operations are applied to the background
 * buffer and transfered to the physical frame buffer
 * afterwards. This component is also responsible for
 * drawing the mouse cursor.
 * It uses SDL for accessing the screen under linux. 
 */

#include <SDL/SDL.h>
#include <signal.h>
#include "dope-config.h"
#include "screen.h"
#include "clipping.h"

#ifndef SDL_DISABLE
#define SDL_DISABLE 0
#endif
#define SCR_DEBUG 0

static long scr_width,scr_height,scr_depth;
static void *scr_adr;
static long curr_mx=100,curr_my=100;
static SDL_Surface *screen;
static long cursor_state=1;
static struct clipping_services *clip;
static long dax1,day1,dax2,day2;
static long mouse_visible=0;

int init_screen(struct dope_services *d);


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** SET UP SCREEN ***/
static long set_screen(long width, long height, long depth) {
	scr_width=width;
	scr_height=height;
	scr_depth=depth;
	if (SCR_DEBUG) {
		screen = SDL_SetVideoMode(scr_width, scr_height*2 + 100, scr_depth, SDL_SWSURFACE);
	} else {
		screen = SDL_SetVideoMode(scr_width, scr_height + 20, scr_depth, SDL_SWSURFACE);
	}
	if (!screen) return 0;
	scr_adr=screen->pixels;
//	SDL_WM_ToggleFullScreen(screen);
	SDL_ShowCursor(SDL_DISABLE);
	return 1;
}


/*** DEINITIALISATION ***/
static void restore_screen(void) {
	/* nothing to restore - sdl is too good */
}


/*** PROVIDE INFORMATION ABOUT THE SCREEN ***/
static long  get_scr_width	(void)	{return scr_width;}
static long  get_scr_height	(void)	{return scr_height;}
static long  get_scr_depth	(void)	{return scr_depth;}
static void *get_scr_adr	(void)	{return scr_adr;}
static void *get_buf_adr	(void)	{return scr_adr;}


//static void shade_area(long x1,long y1,long x2,long y2) {
//	u16 *dst = scr_adr;
//	u16 i;
//	if (!SCR_DEBUG) return;
//	dst += scr_width*scr_height;
//	dst += y1*scr_width;
//	for (;y1<y2;y1++) {
//		for (i=x1;i<x2;i++) {
//			*(dst+i) += 12;
//		}
//		dst += scr_width;
//	}
//}


/*
static void scr_reset_shade(void) {
	u32 i = scr_width*scr_height;
	u16 *d = scr_adr;
	if (!SCR_DEBUG) return;
	d += scr_width*scr_height;
	while (i--) *(d++)=0;
	SDL_UpdateRect(screen, 0, scr_height, scr_width, scr_height);
}
*/


/*** MAKE CHANGES ON THE SCREEN VISIBLE (BUFFERED OUTPUT) ***/
static void update_area(long x1,long y1,long x2,long y2) {
	long dx;
	long dy;
	long d;
	
	/* apply clipping to specified area */
	if (x1<(d=clip->get_x1())) x1=d;
	if (y1<(d=clip->get_y1())) y1=d;	
	if (x2>(d=clip->get_x2())) x2=d;
	if (y2>(d=clip->get_y2())) y2=d;
	
	dx=x2-x1;
	dy=y2-y1;
	if (dx<0) dx=-dx;
	if (dy<0) dy=-dy;
	SDL_UpdateRect(screen, x1, y1, dx+1, dy+1);
#if (SCR_DEBUG)
	shade_area(x1,y1,x1+dx+1,y1+dy+1);
	SDL_UpdateRect(screen, x1, y1+scr_height, dx+1, dy+1);
#endif
}


static void draw_cursor(short *data,long x,long y) {
	static short i,j;
	short *dst= (short *)scr_adr + y*scr_width + x;
	short *d;
	short *s=data;
	short w=*(data++),h=*(data++);
	short linelen=w;
	if (x>scr_width-16) return;
	if (y>scr_height-16) return;
	if (x>scr_width-16-16) linelen=scr_width-16-x;
	if (y>scr_height-16-16) h=scr_height-16-y;
	for (j=0;j<h;j++) {
		d=dst;s=data;
		for (i=0;i<linelen;i++) {
			if (*s) *d=*s;
			d++;s++;
		}
		dst+=scr_width;
		data+=w;
	}
}

static short bg_buffer[16][16];

static void save_background(long x,long y) {
	short *src=(short *)scr_adr + y*scr_width + x;
	short *dst=(short *)&bg_buffer;
	short *s;
	static int i,j;
	short h=16;
	if (y>scr_height-16) h=scr_height-y;
	for (j=0;j<h;j++) {
		s=src;
		for (i=0;i<16;i++) {
			*(dst++)=*(s++);
		}
		src+=scr_width;
	}
}


static void restore_background(long x,long y) {
	short *src=(short *)&bg_buffer;
	short *dst=(short *)scr_adr + y*scr_width + x;
	short *d;
	static int i,j;
	short h=16;
	if (y>scr_height-16) h=scr_height-y;
	for (j=0;j<h;j++) {
		d=dst;
		for (i=0;i<16;i++) {
			*(d++)=*(src++);
		}
		dst+=scr_width;
	}
}


extern short smallmouse_trp;
extern short bigmouse_trp;


/*** SET MOUSE CURSOR TO THE SPECIFIED POSITION ***/
static void set_mouse_pos(long mx,long my) {
	if (cursor_state==1) {
		if (mouse_visible) {
			restore_background(curr_mx,curr_my);
			mouse_visible=0;
		}
		if ((mx>=dax1) && (my>=day1) && (mx+16<=dax2) && (my+16<=day2)) {
			/* mouse is inside the bad area */
		} else {
			if (!mouse_visible) {
				save_background(mx,my);
				draw_cursor(&bigmouse_trp,mx,my);
				mouse_visible=1;
			}
		}
		SDL_UpdateRect(screen, mx, my, 16, 16);
		SDL_UpdateRect(screen, curr_mx, curr_my, 16, 16);
	}
	curr_mx=mx;
	curr_my=my;
}


/*** SET NEW MOUSE SHAPE ***/
static void set_mouse_shape(void *new_shape) {
	/* not built in yet... */
}


/*** SWITCH MOUSE CURSOR OFF (NEEDED FOR DIRECT DRAWING WITH SOFTWARE CURSOR) ***/
static void mouse_off(void) {
	if (cursor_state==1) {
		if (mouse_visible) {
			restore_background(curr_mx,curr_my);
			SDL_UpdateRect(screen, curr_mx, curr_my, 16, 16);
		}
		mouse_visible=0;
	}
	cursor_state--;
}


/*** SWITCH MOUSE CURSOR ON ***/
static void mouse_on(void) {
	cursor_state++;
	if (cursor_state==1) {
		if (!mouse_visible) {
			save_background(curr_mx,curr_my);
			draw_cursor(&bigmouse_trp,curr_mx,curr_my);
			mouse_visible=1;
			SDL_UpdateRect(screen, curr_mx, curr_my, 16, 16);
		}
	}
}


static void set_draw_area(long x1,long y1,long x2,long y2) {
	dax1=x1;day1=y1;dax2=x2;day2=y2;
	if ((curr_mx+16>=dax1) && (curr_mx<=dax2) && (curr_my+16>=day1) && (curr_my<=day2)) {
		if (mouse_visible) {
			restore_background(curr_mx,curr_my);
			SDL_UpdateRect(screen, curr_mx, curr_my, 16, 16);	
		}
		mouse_visible=0;
	} else {
		if (!mouse_visible) {
			save_background(curr_mx,curr_my);
		}
	}
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct screen_services services = {
	set_screen,
	restore_screen,
	get_scr_width,
	get_scr_height,
	get_scr_depth,
	get_scr_adr,
	get_buf_adr,
	update_area,
	set_mouse_pos,
	set_mouse_shape,
	mouse_off,
	mouse_on,
	set_draw_area
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_screen(struct dope_services *d) {
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) return 0;
	signal(SIGSEGV,SIG_DFL);
	clip = d->get_module("Clipping 1.0");
	d->register_module("Screen 1.0",&services);
	return 1;
}
