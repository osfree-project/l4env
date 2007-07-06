/*
 * \brief   Bumpmapping effect
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
#include "bump.h"

/*** DECLARATIONS FROM STANDARD MATH LIB ***/
double sin(double x);
double cos(double x);

#define SCR_W 320
#define SCR_H 240

extern long app_id;         		/* DOpE application id */

extern u8 _binary_bumpmap_xga_start;
extern u8 _binary_light_xga_start;

static void *bumpvscr_id;
static u16 *scr_adr = NULL;
static int user_flag = 0;
static int ux=0,uy=0;

static s32 bumpbuf1[SCR_H+5][SCR_W];
static s32 bumpbuf2[SCR_H+5][SCR_W];
static u16 lightbuf[SCR_H*4][SCR_W];

static float alph=0.0,beta=1.0,gamm=2.0,delt=3.0;


static void gen_heightmap(u8 *src,u32 *dst) {
	int cnt = SCR_W*SCR_H;
	u16 pix;
	for (;cnt--;) {
		pix = (*(src)<<8) | *(src+1);
		src+=2;
		*(dst++) = ((((pix>>11)&0x1f) + ((pix>>6)&0x1f) + (pix&0x1f))*3)>>1;
	}
}


static void filter(u32 *src,u32 *dst) {
	int cnt = SCR_W * (SCR_H);
	src += SCR_W;
	
	memcpy(src+SCR_W*SCR_H,src,5*2*SCR_W);
	
	for (;cnt--;) {
		*(dst++) = (*(src-1) + *(src+1) + *(src-SCR_W) + *(src+SCR_W) +
				   *(src-1-SCR_W) + *(src+1-SCR_W) + 
				   *(src-1+SCR_W) + *(src+1+SCR_W))>>3;
		src++;
	}
}


static void gen_offset_map(s32 *src,s32 *dst) {
	int cnt = SCR_W*SCR_H;
	s32 *d;
	int dx,dy,x,y;
	d=dst;
	for (;cnt--;) {
		dx = *src - *(src+3);
		dy = *src - *(src+3*SCR_W);
		*d = dy*SCR_W + dx;
		if (*src < 80) (*d)+= 320*240;
		src++;d++;
	}
	
	d=dst;
	for (y=0;y<SCR_H;y++) {
		dy = (y-(SCR_H/2)) / 4;
		for (x=0;x<SCR_W;x++) {
		
			dx = (x-(SCR_W/2)) / 4;
			
			*(d++) += dy*SCR_W + dx;
			cnt++;
		}
	}
}


static void prepare_bumpmap(void) {

	gen_heightmap(&_binary_bumpmap_xga_start, (u32 *)&bumpbuf1[0][0]);
	filter((u32 *)&bumpbuf1[0][0], (u32 *)&bumpbuf2[0][0]);
	filter((u32 *)&bumpbuf2[0][0], (u32 *)&bumpbuf1[0][0]);
	filter((u32 *)&bumpbuf1[0][0], (u32 *)&bumpbuf2[0][0]);
	gen_offset_map(&bumpbuf2[0][0], &bumpbuf1[0][0]);
}


#define MASK 0x7bcf     // rrrr rggg gggb bbbb

static void prepare_light(void) {
	u8 *src = &_binary_light_xga_start;
	u16 *s,*d = &lightbuf[SCR_H/2][0];
	int cnt = SCR_W*SCR_H;
	for (;cnt--;) {
		*(d++) = ((*src)<<8) + *(src+1);
		src+=2;
	}

	s = &lightbuf[SCR_H/2][0];
	d = &lightbuf[SCR_H + SCR_H/2][0];
	for (cnt = SCR_W*SCR_H;cnt--;) {
		*(d++) = ((*(s++))>>1)&MASK;
	}
	
}


static void bump(s32 *bm, u16 *ls, u16 *dst) {
	int cnt = SCR_W*SCR_H;
	for (;cnt--;) *(dst++) = *(ls + *(bm++));
}


static void enter_callback(dope_event *e,void *arg) {
	user_flag=1;
}


static void leave_callback(dope_event *e,void *arg) {
	user_flag=0;
}


static void motion_callback(dope_event *e,void *arg) {
	if (e->type == EVENT_TYPE_MOTION) {
		ux = e->motion.abs_x;
		uy = e->motion.abs_y;
	}
}


/*** INITIALISATION OF THE EFFECT - MUST BE CALLED DURING THE START UP ***/
int bump_init(void) {

	/* open window with rt-widget */
	dope_cmd(app_id, "bumpwin=new Window()" );
	dope_cmd(app_id, "bumpvscr=new VScreen()" );
	dope_cmd(app_id, "bumpvscr.setmode(320,240,\"RGB16\")" );
	dope_cmd(app_id, "bumpvscr.set(-framerate 25)");
	dope_cmd(app_id, "bumpwin.set(-x 500 -y 460 -w 330 -h 267 -background off -content bumpvscr)" );

	dope_bind(app_id,"bumpvscr","motion", motion_callback, (void *)0x123);
	dope_bind(app_id,"bumpvscr","enter", enter_callback, (void *)0x123);
	dope_bind(app_id,"bumpvscr","leave", leave_callback, (void *)0x123);
	
	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb(app_id, "bumpvscr");
	if (!scr_adr) return -1;
	
	/* get identifier of pSLIM-server */
	bumpvscr_id = vscr_get_server_id(app_id, "bumpvscr");
	
	prepare_bumpmap();
	prepare_light();
	printf("VScrTest(bump_init): done\n");
	return 0;
};


/*** MAIN EFFECT ROUTINE - MUST BE CALLED PERIODICALLY ***/
void bump_exec(int exec_flag) {
	static int spot_x,spot_y;

	vscr_server_waitsync(bumpvscr_id);

	if (!exec_flag || !scr_adr) return;

	if (user_flag) {
		spot_x = 160 - ((ux-SCR_W/2)>>1);
		spot_y = 140 - ((uy-SCR_H/2)>>1);
	} else {
		spot_x = 160 + (int)(50.0*sin(alph));
		spot_y = 140 + (int)(50.0*sin(beta));
	}

	alph = alph + 0.051;
	beta = beta + 0.073;
	gamm = gamm + 0.083;
	delt = delt + 0.021;

	if (scr_adr) {
		bump(&bumpbuf1[0][0], &lightbuf[SCR_H/2+spot_y][spot_x], scr_adr);
	}
}

