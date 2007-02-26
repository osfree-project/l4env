/*
 * \brief  DOpE main event loop
 * \date   2002-11-13
 * \author Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "dopestd.h"
#include "scrdrv.h"
#include "input.h"
#include "event.h"
#include "widget.h"
#include "button.h"
#include "container.h"
#include "background.h"
#include "frame.h"
#include "window.h"
#include "winman.h"
#include "userstate.h"
#include "scrollbar.h"
#include "grid.h"
#include "redraw.h"
#include "hashtab.h"
#include "server.h"
#include "timer.h"
#include "rtman.h"
#include "pslim.h"


static struct grid_services *grid;
static struct input_services *input;
static struct frame_services *frame;
static struct timer_services *timer;
static struct rtman_services *rtman;
static struct pslim_services *pslim;
static struct scrdrv_services *scrdrv;
static struct window_services *win;
static struct button_services *but;
static struct server_services *server;
static struct redraw_services *redraw;
static struct winman_services *winman;
static struct hashtab_services *hashtab;
static struct container_services *cont;
static struct scrollbar_services *scroll;
static struct userstate_services *userstate;
static struct background_services *bg;

void eventloop(struct dope_services *dope);


static void dummyclick(WIDGET *w) {
	w=w;
}

static WINDOW *menuwindow=NULL;
BUTTON *menubutton=NULL;

static void menu_click(BUTTON *m) {
	menuwindow->gen->set_x(menuwindow,scrdrv->get_scr_width()-18);
	menuwindow->gen->set_y(menuwindow,-2);
	menuwindow->gen->update(menuwindow,1);
}


static void smallmenu_click(BUTTON *m) {
	menuwindow->gen->set_x(menuwindow,-2-18);
	menuwindow->gen->set_y(menuwindow,-2);
	menuwindow->gen->update(menuwindow,1);

}



char *texts[] = {
	"Button1","Button2","Button3","Button4","Button5","Button6","Button7",
	"Button8"
};


extern long pt;     /* !!! temporary debugging stuff */
extern long pn;
extern long bf;
extern long af;

void eventloop(struct dope_services *dope) {
	static long quit;

	static WINDOW *w1,*desk,*slotwin;
	static CONTAINER *c;
	static FRAME *f;
	static BACKGROUND *bg1;
	static BUTTON *b1;
	static PSLIM *p;
	static s32 i,j;
	static u32 start_time, rt_end_time, end_time, usr_end_time;
	static s32 left_time;
	static u32 curr_length;
	s32 period_clock  = 10000;
	s32 period_length = 8000;

	static s32 slot_usr_time[4];
	static s32 slot_rt_time[4];
	static s32 slot_nrt_time[4];
	static s32 curr_slot=0;
	static s32 period_exceeded = 0;

	scrdrv    = dope->get_module("ScreenDriver 1.0");
	input     = dope->get_module("Input 1.0");
	win       = dope->get_module("Window 1.0");
	cont      = dope->get_module("Container 1.0");
	bg        = dope->get_module("Background 1.0");
	but       = dope->get_module("Button 1.0");
	userstate = dope->get_module("UserState 1.0");
	scroll    = dope->get_module("Scrollbar 1.0");
	frame     = dope->get_module("Frame 1.0");
	grid      = dope->get_module("Grid 1.0");
	redraw    = dope->get_module("RedrawManager 1.0");
	hashtab   = dope->get_module("HashTable 1.0");
	server    = dope->get_module("Server 1.0");
	winman    = dope->get_module("WindowManager 1.0");
	timer     = dope->get_module("Timer 1.0");
	rtman     = dope->get_module("RTManager 1.0");
	pslim     = dope->get_module("PSLIM 1.0");

	input->update_properties();
	winman->update_properties();


	INFO(printf("create window\n"));
	desk = win->create();
	desk->gen->set_x(desk,-50);
	desk->gen->set_y(desk,-50);
	desk->gen->set_w(desk,scrdrv->get_scr_width() + 100);
	desk->gen->set_h(desk,scrdrv->get_scr_height()+ 100);
	desk->gen->update(desk,WID_UPDATE_REDRAW);

	bg1=bg->create();
	bg1->gen->set_w(bg1,scrdrv->get_scr_width() + 100);
	bg1->gen->set_h(bg1,scrdrv->get_scr_height()+ 100);
	bg1->bg->set_style(bg1,BG_STYLE_DESK);
	bg1->bg->set_click(bg1,dummyclick);
//  f=frame->create();
//  desk->win->content(desk);
	desk->win->set_content(desk,(WIDGET *)bg1);

	desk->win->open(desk);

	/*** create menubar ***/

	menuwindow = w1=win->create();
	w1->win->set_elem_mask(w1,0);
	w1->gen->set_x(w1,-2-18);
	w1->gen->set_y(w1,-2);
	w1->gen->set_w(w1,scrdrv->get_scr_width()+4+18);
	w1->gen->set_h(w1,22);
	w1->win->set_staytop(w1,1);
	w1->gen->update(w1,0);

	f=frame->create();
//  f=w1->win->get_workarea(w1);
	w1->win->set_content(w1,f);

	c=cont->create();
	c->gen->set_w(c,scrdrv->get_scr_width()+4+18);
	c->gen->set_h(c,22);
	c->gen->update(c,0);

	menubutton = b1 = but->create();
	b1->gen->set_x(b1,18);
	b1->gen->set_y(b1,0);
	b1->gen->set_w(b1,scrdrv->get_scr_width()+4);
	b1->gen->set_h(b1,22);
	b1->but->set_click(b1,menu_click);
	b1->gen->update(b1,0);
	c->cont->add(c,b1);

	b1 = but->create();
	b1->gen->set_x(b1,0);
	b1->gen->set_y(b1,2);
	b1->gen->set_w(b1,18);
	b1->gen->set_h(b1,20);
	b1->but->set_text(b1,"!");
	b1->but->set_click(b1,smallmenu_click);
	b1->gen->update(b1,0);
	c->cont->add(c,b1);

//  f->frame->set_content(f,c);
//  f->gen->update(f,0);

	w1->win->set_content(w1,c);
	w1->gen->update(w1,0);

	w1->win->open(w1);

	userstate->idle();


	/*** create slot display window ***/
	slotwin = w1 = win->create();

	p = pslim->create();
	p->pslim->set_mode(p,32,64,16);
	w1->win->set_content(w1,p);
	w1->gen->set_x(w1,10);
	w1->gen->set_y(w1,110);
	w1->gen->set_w(w1,74);
	w1->gen->set_h(w1,250);
	w1->gen->update(w1,0);

	w1->win->open(w1);

	/*** STARTING DOpE SERVER ***/

	INFO(printf("starting server\n"));
	server->start();

	redraw->exec_redraw(1000*1000*1000);

	/*** ENTERING MAINLOOP ***/

	INFO(printf("starting eventloop\n"));

	j=0;
	while (!quit) {
		start_time = timer->get_time();

		input->update(userstate->get_curr_focus());
		userstate->handle();

		usr_end_time = timer->get_time();

		rtman->execute();

		rt_end_time = timer->get_time();

		left_time = (s32)period_length - (s32)timer->get_diff(start_time,timer->get_time());

		if (left_time<1000) left_time = 1000;

		redraw->exec_redraw(left_time);

		end_time = timer->get_time();

		curr_length = timer->get_diff(start_time,end_time);

		if (curr_length>period_length) {
//          ERROR(printf("*** period exceeded! %lu ***\n",j);)
/*          ERROR(printf("* rt_end_time  = %lu\n",(long)rt_end_time - start_time);)
			ERROR(printf("* end_time     = %lu\n",(long)end_time - start_time);)
			ERROR(printf("* left_time    = %lu\n",(long)left_time);)
			ERROR(printf("* pt           = %lu\n",(long)pt);)
			ERROR(printf("* pn           = %lu\n",(long)pn);)
			ERROR(printf("* bf           = %lu\n",(long)bf);)
			ERROR(printf("* af           = %lu\n",(long)af);)*/
			j++;
			period_exceeded = 1;
		} else {
			timer->usleep(period_clock - curr_length);
		}
/* !!!!! */
		timer->usleep(1000*10);
		
		slot_usr_time[curr_slot] += usr_end_time - start_time;
		slot_rt_time[curr_slot]  += rt_end_time - usr_end_time;
		slot_nrt_time[curr_slot] += end_time - rt_end_time;
		curr_slot = (curr_slot + 1)%4;

		i--;
		if (i<=0) {
//      if (0) {
			pslim_rect_t r;
//          printf("rt_time=%lu\n",timer->get_diff(start_time,rt_end_time));
//          INFO(printf("avr_ppt=%lu, ",(u32)(1000000*redraw->get_avr_ppt()));)
//          INFO(printf("min_ppt=%lu\n",(u32)(1000000*redraw->get_min_ppt()));)
			i+=20;

			r.x=0; r.y=0; r.w=32; r.h=64;
			if (period_exceeded) p->pslim->fill(p,&r,0x7800 + 0x03e0 + 0x000f);
			else p->pslim->fill(p,&r,0);

			period_exceeded = 0;
			for (j=0;j<4;j++) {

				r.x=j*8+1; r.y=0; r.w=5; r.h=slot_usr_time[j]/((period_length*5)/64);
				p->pslim->fill(p,&r,0x07e0);

				r.x=j*8+1; r.y+=r.h; r.w=5; r.h=slot_rt_time[j]/((period_length*5)/64);
				p->pslim->fill(p,&r,0xf800);
				r.x=j*8+2; r.w=1;
				p->pslim->fill(p,&r,0xf800 + 0x03e0 + 0x000f);

				r.x=j*8+1; r.y+=r.h; r.w=5; r.h=slot_nrt_time[j]/((period_length*5)/64);
				p->pslim->fill(p,&r,0x1f);
				r.x=j*8+2; r.w=1;
				p->pslim->fill(p,&r,0x3800 + 0x01e0 + 0x001f);

				slot_rt_time[j] = 0;
				slot_nrt_time[j] = 0;
				slot_usr_time[j] = 0;
			}
		}


	}

	scrdrv->restore_screen();
}
