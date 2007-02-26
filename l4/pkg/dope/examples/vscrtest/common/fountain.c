/*
 * \brief   Fountain particles effect
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
#include "fountain.h"

/*** DECLARATIONS FROM STANDARD MATH LIB ***/
double sin(double x);
double cos(double x);
int    abs(int j);
#define M_PI 3.14159265358979323846

#define SCR_W 320
#define SCR_H 200

#define CENTX SCR_W/2
#define CENTY SCR_H/2

#define NUM_DOTS 200
#define MAX_RADIUS 650

static u16 scr_buf[SCR_H*2][SCR_W];
static u16 *buf_adr1;
static u16 *scr_adr;

static u16 bigball_gfx[32][32];
static u16 ball_gfx[16][16];

static float sintab[1024];
static float costab[1024];

static float dots_r[NUM_DOTS];      /* current radius = distance from centre*/
static float dots_h[NUM_DOTS];      /* current height */
static float dots_cos[NUM_DOTS];    /* cosine of movement direction */
static float dots_sin[NUM_DOTS];    /* sine of movement direction */
static float dots_v[NUM_DOTS];      /* vertical speed */
static float dots_g[NUM_DOTS];      /* gravity */

static float dstx[NUM_DOTS];
static float dsty[NUM_DOTS];
static float dstz[NUM_DOTS];

static int scrx[NUM_DOTS];
static int scry[NUM_DOTS];

static int alph,delt;
static u16 fadetab[1024];
static u16 coltab[1024];

static int px=64,py=64,pflag=0;
static void *fntnvscr_id;
extern long app_id;

/*** PSEUDO RANDOM VALUE GENERATOR ***/
static unsigned int SEED = 93186752;
static int pseudo_rand (void)  {
   static unsigned int a = 1588635695, m = 4294967291U, q = 2, r = 1117695901;
   SEED = a*(SEED % q) - r*(SEED / q);
   return (int)(0x0000ffff*(((double)SEED / (double)m)));
}


static void plot(int x,int y,u16 *dst) {
	u16 *src = &ball_gfx[0][0];
	dst += SCR_W*(y-7) + x - 7;
	for (y=16;y--;) {
		for (x=16;x--;) {
			dst[x] += *(src++);
			if (dst[x]>255) dst[x]=255;
		}
		dst += SCR_W;
	}
}


static void move_dots(void) {
	int i;
	for (i=0;i<NUM_DOTS;i++) {
		dots_r[i] = dots_r[i] + 5.0;
		if (dots_r[i] > MAX_RADIUS) {
			dots_r[i] -= MAX_RADIUS;
			dots_h[i] = 0.0;
			dots_v[i] = 30.0;
		}
		dots_v[i] -= dots_g[i];
		dots_h[i] += dots_v[i];
		if (dots_h[i] < 0.0) {
			dots_h[i] = 0.0;
			dots_v[i] = -dots_v[i] * 0.60;
			if (abs(dots_v[i]) < 1.0) dots_v[i]=0;
		}
		
	}
}


static void convert_to_xyz(void) {
	int i;
	for (i=0;i<NUM_DOTS;i++) {
		dstx[i] = dots_cos[i] * dots_r[i];
		dsty[i] = dots_sin[i] * dots_r[i];
		dstz[i] = dots_h[i] - 100;
	}
}


static void rotate(float *xbuf,float *ybuf,int angle) {
	float sina = sintab[angle&1023];
	float cosa = costab[angle&1023];
	int i;
	float x,y;
	
	for (i=0;i<NUM_DOTS;i++) {
		x = *xbuf * cosa + *ybuf * sina;
		y = *xbuf * sina - *ybuf * cosa;
		*(xbuf++) = x;
		*(ybuf++) = y;
	}
}


static void projection(void) {
	int i;
	int z;
	int distance = 300;
	float *xbuf=dstx, *ybuf=dsty, *zbuf=dstz;
	int *scrxbuf = scrx,*scrybuf = scry;
	
	for (i=0;i<NUM_DOTS;i++) {
		z = *(zbuf++) + 1300;
		if (z == 0) z+=1;
		*(scrxbuf++) = (distance*(*(xbuf++))) / z;
		*(scrybuf++) = (distance*(*(ybuf++))) / z;
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
	px=x;py=y;
}


/*** INITIALISATION OF THE EFFECT - MUST BE CALLED DURING THE START UP ***/
int fountain_init(void) {
	int i;

	/* open window with rt-widget */
	dope_cmd(app_id, "fntnwin=new Window()" );
	dope_cmd(app_id, "fntnvscr=new VScreen()" );
	dope_cmd(app_id, "fntnvscr.setmode(320,200,\"RGB16\")" );
	dope_cmd(app_id, "fntnvscr.set(-framerate 25)" );
	dope_cmd(app_id, "fntnwin.set(-x 90 -y 150 -w 330 -h 227 -background off -content fntnvscr)" );

	dope_bind(app_id,"fntnvscr","motion", motion_callback, (void *)0x123);
	dope_bind(app_id,"fntnvscr","enter", enter_callback, (void *)0x123);
	dope_bind(app_id,"fntnvscr","leave", leave_callback, (void *)0x123);
	
	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb(app_id, "fntnvscr");
	if (!scr_adr) return -1;
	
	/* get identifier of pSLIM-server */
	fntnvscr_id = vscr_get_server_id(app_id, "fntnvscr");
	buf_adr1 = &scr_buf[SCR_H/2][0];
		
	/* generate sine and cosine table */
	{
		float step = M_PI/512.0;
		float ang = 0.0;
		for (i=0;i<1024;i++) {
			sintab[i]=sin(ang);
			costab[i]=cos(ang);
			ang = ang + step;
		}
	}
	
	/* generate fading table */
	{
		fadetab[0]=0;
		for (i=1;i<1024;i++) {
			fadetab[i]=(i*13)>>4;
			if (fadetab[i] == i) fadetab[i]--;
		}
	}
	
	/* generate ball gfx */
	{
		int x,y;
		float r;
		for (y=-15;y<16;y++) {
			for (x=-15;x<16;x++) {
				r = x*x + y*y;
				bigball_gfx[y+15][x+15]=0;
				if (r<15*15) bigball_gfx[y+15][x+15]=9;  //5  + 20;
				if (r<10*10) bigball_gfx[y+15][x+15]=15;  //10 + 30;
				if (r<5*5)   bigball_gfx[y+15][x+15]=40;  //25 + 50;
				if (r<2*2)   bigball_gfx[y+15][x+15]=200;  //160;
			}
		}
		for (y=0;y<15;y++) {
			for (x=0;x<15;x++) {
				ball_gfx[y][x] = (bigball_gfx[y*2][x*2] + bigball_gfx[y*2+1][x*2+1] +
				                  bigball_gfx[y*2][x*2+1] + bigball_gfx[y*2+1][x*2]) >> 2;
			}
		}
	}

	/* generate color table */
	{
		float r=0.0,g=0.0,b=0.0;
		for (i=0;i<150;i++) {
			coltab[i] = (((int)b)&0x1f) + (((int)g<<6)&0x07e0) + (((int)r<<11)&0xf800);
			r += 0.3/2; g += 0.3/2; b += 0.5/2;
			if (r>31) r=31;
			if (g>31) g=31;
			if (b>31) b=31;
		}
		for (;i<256;i++) {
			coltab[i] = (((int)b)&0x1f) + (((int)g<<6)&0x07e0) + (((int)r<<11)&0xf800);
			r += 0.25*1; g += 0.18*1; b += 0.15*1;
			if (r>31) r=31;
			if (g>31) g=31;
			if (b>31) b=31;
		}
		for (;i<1024;i++) coltab[i] = 0xffff;
	}

	/* init dots */
	{
		int i;
		for (i=0;i<NUM_DOTS;i++) {
			dots_r[i]   = ((pseudo_rand()%MAX_RADIUS) & 0xff0f) + (pseudo_rand()%130);
			dots_cos[i] = cos(i);
			dots_sin[i] = sin(i);
			dots_v[i]   = pseudo_rand()%30;
			dots_g[i]   = 0.99;
		}
	}

	printf("VScrTest(fntn_init): done\n");
	return 0;
}




/*** MAIN EFFECT ROUTINE - MUST BE CALLED PERIODICALLY ***/
void fountain_exec(int exec_flag) {
	u16 *src,*dst;
	int i;

	vscr_server_waitsync(fntnvscr_id);
	
	if (!exec_flag || !scr_adr) return;
	
	/* change view angles */
	alph = alph + 13;
	delt = delt + 17;
	if (alph > 2*M_PI) alph -= 2*M_PI;
	if (delt > 2*M_PI) delt -= 2*M_PI;

	move_dots();
	convert_to_xyz();
	
	if (pflag) {
//  	rotate(dstx,dstz,552);
		rotate(dstz,dstx,px + 768 + 100);
		rotate(dsty,dstz,py*3 + 768 - 50);
	} else {
		rotate(dstx,dstz,552);
		rotate(dstx,dsty,alph>>3);
		rotate(dsty,dstz,850 + 40*sintab[(delt>>3)&1023]);
	}
		
	projection();

	/* fade down */
	dst = buf_adr1;
	for (i=SCR_W*SCR_H;i--;dst++) *dst = fadetab[*dst];

	/* draw dots */
	{
		int x,y;
		for (i=0;i<NUM_DOTS;i++) {
			x = scrx[i] + CENTX;
			y = scry[i] + CENTY;
			if ((x>8) && (x<SCR_W-9) && (y>8) && (y<SCR_H-9)) plot(x,y,buf_adr1);
		}
	}

	/* convert to 16bit rgb */
	src = buf_adr1; 
	dst = scr_adr;
	for (i=SCR_W*SCR_H;i--;) *(dst++) = coltab[*(src++)];

}
