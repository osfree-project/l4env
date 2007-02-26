/*
 * \brief   Feedback effect
 * \date    2002-10-10
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** DOpE SPECIFIC INCLUDES ***/
#include "dopestd.h"
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "feedback.h"

/*** DECLARATIONS FROM STANDARD MATH LIB ***/
double sin(double x);
double cos(double x);

#define BTN_LEFT  0x110
#define BTN_RIGHT 0x111

#define SCR_W 256                   /* size of virtual screen */
#define SCR_H 256
#define CENTX 128                   /* centre position of virtual screen */
#define CENTY 128

extern long app_id;                 /* DOpE application id */

static void *feedvscr_id;

static u16 scr_buf[SCR_H*2][SCR_W];
static u16 *buf_adr;
static u16 *scr_adr = NULL;
static u16 ball_gfx[32][32];
//static u16 map[SCR_H][SCR_W];     /* map for fixed distortion */
static float mx = 42.0, my = 42.0;
static int px = 64, py = 64, pflag = 0;

static float alph=0.0,beta=1.0,gamm=2.0,delt=3.0;


/*** GENERATE SHAPE OF A BALL ***/
static void gen_ball_gfx(void) {
	int x,y;
	for (y=-15;y<16;y++) {
		for (x=-15;x<16;x++) {
			if (x*x + y*y < 15.5*15.5) ball_gfx[y+15][x+15]=0xffff;
			else ball_gfx[y+15][x+15]=0;
		}
	}
}



/*** GENERATE STATIC MAP FOR TUNNEL-DISTORTION ***/
//static void gen_tunn_map(void) {
//  int x,y;
//  for (y=0;y<SCR_H;y++) {
//  	for (x=0;x<SCR_W;x++) {
//  		map[y][x] = y*SCR_W + x;
//  	}
//  }
//}


/* mask for killing the highest bits of the rgb color components */
#define MASK 0x7bcf   /* 16bit color format: rrrr rggg gggb bbbb */

/*** APPLY SMOOTHING ***/
static void smooth(u16 *src,u16 *dst) {
	s16 *s,*d;
	int linoff = 2*SCR_W;
	int cnt;

	s = src;
	d = dst;
	for (cnt=2*SCR_W;cnt--;) *(d++) = *(s++);

	s = src + SCR_W*(SCR_H - 2);
	d = dst + SCR_W*(SCR_H - 2);
	for (cnt=2*SCR_W;cnt--;) *(d++) = *(s++);

	s = src + 2*SCR_W;
	d = dst + 2*SCR_W;
	for (cnt=SCR_W*(SCR_H - 4);cnt--;) {
		*(d++)= ((((((*(s+linoff))>>1)&MASK) + (((*(s+2))>>1)&MASK))>>1)&MASK) +
				((((((*(s-linoff))>>1)&MASK) + (((*(s-2))>>1)&MASK))>>1)&MASK);     
		s++;    
	}
}



/*** APPLY DISTORTION USING A FIXED MAP ***/
//static void distort(u16 *map,u16 *src,u16 *dst) {
//  int cnt = SCR_W*SCR_H;
//  for (;cnt--;) *(dst++) = *(src + *(map++));
//}



/*** APPLY SINE DISTORTION ***/
static void sindist(u16 *src,u16 *dst) {
	static float a,b,af,bf;
	static s16 xmap[SCR_W];
	int x,y;
	u16 *s;

	memcpy(&scr_buf[0],&scr_buf[SCR_H-1],SCR_W*SCR_H);
	memcpy(&scr_buf[SCR_H + (SCR_H>>1)],&scr_buf[SCR_H>>1],SCR_W*SCR_H);
	
	for (x=0;x<SCR_W;x++) xmap[x]=(int)(af*sin(2*b+((float)x)/mx))*SCR_W;

	for (y=0;y<SCR_H;y++) {
		s=src + y*SCR_W + (int)(bf*sin(2*a+((float)y)/my));     // 36.0
		for (x=0;x<SCR_W;x++) *(dst++) = *((s++) + xmap[x]);
//  	for (x=0;x<SCR_W;x++) *(dst++) = (((*(dst))>>1)&MASK) + (((*((s++) + xmap[x]))>>1)&MASK);
	}
	a = a + 0.045*1;
	b = b + 0.033*1;
	af = 40.0 + sin(a)*26.0;
	bf = 40.0 + sin(b)*26.0;
}



/*** PAINT A BALL INTO THE SPECIFIED BUFFER ***/
static void plot(int x,int y,u16 color,u16 *dst) {
	dst += SCR_W*(y-15) + x - 15;
	for (y=31;y--;) {
		for (x=31;x--;) dst[x] |= (ball_gfx[y][x] & color);
		dst += SCR_W;
	}
}



/*** PAINT A (BLACK) BALL INTO THE SPECIFIED BUFFER ***/
static void clear(int x,int y,u16 color,u16 *dst) {
	dst += SCR_W*(y-15) + x - 15;
	for (y=31;y--;) {
		for (x=31;x--;) dst[x] &= (ball_gfx[y][x] ^ color);
		dst += SCR_W;
	}
}



static void enter_callback(dope_event *e,void *arg) {
	pflag=1;
}

static void leave_callback(dope_event *e,void *arg) {
	pflag=0;
}

static void motion_callback(dope_event *e,void *arg) {
	static int x,y;

	if (e->type == EVENT_TYPE_MOTION) {
		x = e->motion.abs_x;
		y = e->motion.abs_y;
	}
	
	if (dope_get_keystate(app_id, BTN_LEFT)) {
		mx = (float)x/3;
		my = (float)y/3;
	}
	px=x;py=y;
}


/*** INITIALISATION OF THE EFFECT - MUST BE CALLED DURING THE START UP ***/
int feedback_init(void) {

	/* open window with rt-widget */
	dope_cmd(app_id, "feedwin=new Window()" );
	dope_cmd(app_id, "feedvscr=new VScreen()" );
	dope_cmd(app_id, "feedvscr.setmode(256,256,\"RGB16\")" );
	dope_cmd(app_id, "feedvscr.set(-framerate 25 -grabmouse yes)");
	dope_cmd(app_id, "feedwin.set(-x 100 -y 450 -w 266 -h 283 -fitx yes -fity yes -background off -content feedvscr)" );
	
	dope_bind(app_id,"feedvscr","motion", motion_callback, (void *)0x123);
	dope_bind(app_id,"feedvscr","press", motion_callback, (void *)0x123);
	dope_bind(app_id,"feedvscr","enter", enter_callback, (void *)0x123);
	dope_bind(app_id,"feedvscr","leave", leave_callback, (void *)0x123);

	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb(app_id, "feedvscr");
	if (!scr_adr) return -1;
	
	/* get identifier of pSLIM-server */
	feedvscr_id = vscr_get_server_id(app_id, "feedvscr");
	
	buf_adr = &scr_buf[SCR_H/2][0];

	gen_ball_gfx();
//  gen_tunn_map();
//  gen_sin1_map();

	printf("VScrTest(feedback_init): done\n");
	return 0;
};



/*** MAIN EFFECT ROUTINE - MUST BE CALLED PERIODICALLY ***/
void feedback_exec(int exec_flag) {

	vscr_server_waitsync(feedvscr_id);
	
	if (!exec_flag || !scr_adr) return;
	
	smooth(scr_adr,buf_adr);

	/* draw feedback source */
	if (!pflag) {   
		clear(CENTX-1*64 + 5*sin(delt),CENTY + 15*cos(alph),0xffff,buf_adr);
		plot (CENTX-1*64 + 15*sin(alph),CENTY + 5*cos(beta),0xf800,buf_adr);
		plot (CENTX-1*64 + 5*sin(beta),CENTY + 15*cos(gamm),0x07e0,buf_adr);
		plot (CENTX-1*64 + 5*sin(gamm),CENTY + 15*cos(delt),0x001f,buf_adr);

		// 10 5
//  	clear(CENTX+0*64 + 80*sin(delt),CENTY + 80*cos(alph),0xffff,buf_adr);
//  	plot (CENTX+0*64 + 80*sin(gamm),CENTY + 80*cos(beta),0xf800,buf_adr);
//  	plot (CENTX+0*64 + 80*sin(alph),CENTY + 80*cos(gamm),0x07e0,buf_adr);
//  	plot (CENTX+0*64 + 80*sin(delt),CENTY + 80*cos(beta),0x001f,buf_adr);

		clear(CENTX+1*64 + 10*sin(delt),CENTY + 15*cos(alph),0xffff,buf_adr);
		plot (CENTX+1*64 + 10*sin(gamm),CENTY + 5*cos(beta),0xf800,buf_adr);
		plot (CENTX+1*64 + 10*sin(alph),CENTY + 15*cos(gamm),0x07e0,buf_adr);
		plot (CENTX+1*64 + 10*sin(delt),CENTY + 5*cos(beta),0x001f,buf_adr);
	}

	alph = alph + 0.05;
	beta = beta + 0.073;
	gamm = gamm + 0.083;
	delt = delt + 0.021;

	sindist(buf_adr,scr_adr);

	if (pflag) {
		if (py<40) py = 40;
		if (py>SCR_H-40) py = SCR_H-40;
		clear(px + 10*sin(delt),py + 10*cos(alph),0xffff,scr_adr);
		plot (px + 10*sin(gamm),py + 10*cos(beta),0xf800,scr_adr);
		plot (px + 10*sin(alph),py + 10*cos(gamm),0x07e0,scr_adr);
		plot (px + 10*sin(delt),py + 10*cos(beta),0x001f,scr_adr);
	}

//  	distort(map,scr_adr,buf_adr);

}


