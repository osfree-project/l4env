/*
 * \brief	DOpE redraw manager module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 * 
 * This module handles the redraw of non-realtime 
 * widgets.  Redraw-actions are stored in a queue    
 * (by widgets) and executed later (at the end of   
 * a period).										
 */

struct private_widget;
#define WIDGET struct private_widget

#include "dope-config.h"
#include "widget_data.h"
#include "widget.h"
//#include "window.h"
#include "winman.h"
#include "thread.h"
#include "redraw.h"
#include "timer.h"

static struct winman_services 	*winman;
static struct thread_services 	*thread;
static struct timer_services	*timer;

WIDGET {
	struct widget_methods 	*gen;	/* pointer to general methods */
	void					*widget_specific_methods;
	struct widget_data		*wd;	/* pointer to general attributes */
};

#define REDRAW_QUEUE_SIZE 5000

#define ACTION_TYPE_DRAW_AREA	0

struct action_struct {
	u16 type;		/* action type (redraw, move..) */
	WIDGET *wid;		/* associated widget */
	s32 x1,y1,x2,y2; /* area in screen */
};

static struct action_struct action_queue[REDRAW_QUEUE_SIZE];
static s32 first=0;
static s32 last=0;
static float pix_per_usec_avr = 8.0;
static float pix_per_usec_min = 100000.0;

static MUTEX *queue_mutex;

int init_redraw(struct dope_services *d);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

#define MAX(a,b) a>b?a:b
#define MIN(a,b) a<b?a:b


static u32 get_noque(void) {
	return (first+REDRAW_QUEUE_SIZE-last)%REDRAW_QUEUE_SIZE;
}


/*
static void old_add_redraw_action(WIDGET *w,long x1,long y1,long x2,long y2,long cmp_idx) {
	long next_idx;
	long cx1,cy1,cx2,cy2;
	long top_cut,bottom_cut;

//	if (get_noque()>4) {
//		printf("redraw queue length > 20, ignore request\n");
//		return;
//	}
//
	if ((x1>x2) || (y1>y2)) return;

	while ((cmp_idx!=first) && (action_queue[cmp_idx].wid!=w)) {
		cmp_idx = (cmp_idx+1)%REDRAW_QUEUE_SIZE;			
	}

	if (cmp_idx==first) {
//		printf("add redraw action: %lu,%lu,%lu,%lu w=%lu h=%lu\n",x1,y1,x2,y2,x2-x1,y2-y1);
		action_queue[first].type = ACTION_TYPE_DRAW_AREA;
		action_queue[first].wid  = w;
		action_queue[first].x1	 = x1;
		action_queue[first].y1	 = y1;
		action_queue[first].x2	 = x2;
		action_queue[first].y2	 = y2;
		first = (first+1)%REDRAW_QUEUE_SIZE;		
		return;
	}
	
	cx1=action_queue[cmp_idx].x1;
	cy1=action_queue[cmp_idx].y1;
	cx2=action_queue[cmp_idx].x2;
	cy2=action_queue[cmp_idx].y2;
	
	next_idx = (cmp_idx+1)%REDRAW_QUEUE_SIZE;
	
	top_cut=y1;
	bottom_cut=y2;
	
//	if ((MAX(x1,cx1) <= MIN(x2,cx2)) && (MAX(y1,cy1) <= MIN(y2,cy2))) {
	if ((x1>cx2) || (y1>cy2) || (x2<cx1) || (y2<cy1)) {
//	if ((x1>cx2) || (y1>cy2) || (x2<cx1) || (y2<cy1) || ((x2-x1)*(y2-y1)<80*80)) {
		add_redraw_action(w,x1,y1,x2,y2,next_idx);
//		printf("lull!\n");
		return;
	}
	
	if (y1 < cy1) {
//		printf("new area on top\n");
		top_cut = MIN(y2,cy1-1);
		add_redraw_action(w,x1,y1,x2,top_cut,next_idx);
	}

	if (y2 > cy2) {
//		printf("new area on bottom\n");
		bottom_cut = MAX(y1,cy2+1);
		add_redraw_action(w,x1,bottom_cut,x2,y2,next_idx);	
	}

	if (x1 < cx1) {
//		printf("new area on left\n");
		add_redraw_action(w,x1,top_cut,MIN(x2,cx1-1),bottom_cut,next_idx);
	}

	if (x2 > cx2) {
//		printf("new area on right\n");
		add_redraw_action(w,MAX(x1,cx2+1),top_cut,x2,bottom_cut,next_idx);
	}

}
*/


static void add_redraw_action(WIDGET *w,long x1,long y1,long x2,long y2,long curr_idx) {
//	long curr_idx = cmp_idx;

	if ((x1>x2) || (y1>y2)) return;

	curr_idx++;if (curr_idx>first) curr_idx=first;

	/* search for queue entry that affect the same widget */
	while ((curr_idx!=first) && (action_queue[curr_idx].wid!=w)) {
		curr_idx = (curr_idx+1)%REDRAW_QUEUE_SIZE;			
	}

	/* if there are no queue entries left to check - add the action */
	if (curr_idx==first) {
//		printf("add redraw action: %lu,%lu,%lu,%lu w=%lu h=%lu\n",x1,y1,x2,y2,x2-x1,y2-y1);
		action_queue[first].type = ACTION_TYPE_DRAW_AREA;
		action_queue[first].wid  = w;
		action_queue[first].x1	 = x1;
		action_queue[first].y1	 = y1;
		action_queue[first].x2	 = x2;
		action_queue[first].y2	 = y2;
		first = (first+1)%REDRAW_QUEUE_SIZE;		
		return;
	}

	/* merge both redraw requests */	
	action_queue[curr_idx].x1 = MIN(action_queue[curr_idx].x1, x1);
	action_queue[curr_idx].y1 = MIN(action_queue[curr_idx].y1, y1);
	action_queue[curr_idx].x2 = MAX(action_queue[curr_idx].x2, x2);
	action_queue[curr_idx].y2 = MAX(action_queue[curr_idx].y2, y2);	
}




/*** PUT NEW REDRAW-ACTION INTO QUEUE ***/
static void draw_area(WIDGET *w,long x1,long y1,long x2,long y2) {

//	printf("draw_area: %lu,%lu,%lu,%lu w=%lu h=%lu\n",x1,y1,x2,y2,x2-x1,y2-y1);

	thread->mutex_down(queue_mutex);
	add_redraw_action(w,x1,y1,x2,y2,last);
//	printf("redraw-queue: first=%lu last%lu\n",first,last);
//	printf("redraw-queue-size: %lu\n",first-last);
	
	thread->mutex_up(queue_mutex);
	return;
	action_queue[first].type = ACTION_TYPE_DRAW_AREA;
	action_queue[first].wid  = w;
	action_queue[first].x1	 = x1;
	action_queue[first].y1	 = y1;
	action_queue[first].x2	 = x2;
	action_queue[first].y2	 = y2;
	first = (first+1)%REDRAW_QUEUE_SIZE;
	
}


/*** GENERATE ACTION TO REDRAW A SPECIFIED WIDGET ***/
static void draw_widget(WIDGET *cw) {
	long cx1,cy1,cx2,cy2;
	WIDGET *win=NULL;
	
	/* widget area relative to its parent */
	cx1 = cw->wd->x;
	cy1 = cw->wd->y;
	cx2 = cx1 + cw->wd->w - 1;
	cy2 = cy1 + cw->wd->h - 1;
	
	cw=cw->wd->parent;
//	printf("draw widget!\n");
	while ((cw!=NULL) && (cw!=(WIDGET *)'#')) {
	
		/* shink current area to parent view area */
		if (cx1 < 0) cx1 = 0;
		if (cy1 < 0) cy1 = 0;
		if (cx2 > cw->wd->w - 1) cx2=cw->wd->w - 1;
		if (cy2 > cw->wd->h - 1) cy2=cw->wd->h - 1;

		cx1+=cw->wd->x;
		cy1+=cw->wd->y;
		cx2+=cw->wd->x;
		cy2+=cw->wd->y;
	
		win=cw;
		cw=cw->wd->parent;
	}
	
	/* is the parent the windowmanager? */
	if (cw) {
		draw_area(win,cx1,cy1,cx2,cy2);
	}
}



/*** GENERATE ACTION TO REDRAW AN AREA OF A SPECIFIED WIDGET ***/
static void draw_widgetarea(WIDGET *cw,s32 rx1,s32 ry1,s32 rx2,s32 ry2) {
	long cx1,cy1,cx2,cy2;
	WIDGET *win=NULL;

	/* determine 'dirty area' inside the widget */
	cx1 = MAX(cw->wd->x + rx1,cw->wd->x);
	cy1 = MAX(cw->wd->y + ry1,cw->wd->y);
	cx2 = MIN(cw->wd->x + rx2,cw->wd->x + cw->wd->w - 1);
	cy2 = MIN(cw->wd->y + ry2,cw->wd->y + cw->wd->h - 1);

	if ((cx1>cx2) || (cy1>cy2)) return;

	cw=cw->wd->parent;
//	printf("draw widget!\n");
	while ((cw!=NULL) && (cw!=(WIDGET *)'#')) {
	
		/* shink current area to parent view area */
		if (cx1 < 0) cx1 = 0;
		if (cy1 < 0) cy1 = 0;
		if (cx2 > cw->wd->w - 1) cx2=cw->wd->w - 1;
		if (cy2 > cw->wd->h - 1) cy2=cw->wd->h - 1;

		cx1+=cw->wd->x;
		cy1+=cw->wd->y;
		cx2+=cw->wd->x;
		cy2+=cw->wd->y;
	
		win=cw;
		cw=cw->wd->parent;
	}
	
	/* is the parent the windowmanager? */
	if (cw) {
		draw_area(win,cx1,cy1,cx2,cy2);
	}
}



long pt;	/* !!! temporary debugging stuff */
long pn;
long bf;
long af;

/*** EXECUTE ACTION QUEUE ***/
static s32 exec_redraw(s32 max_time) {
	s32 w,h,cut_h;
	s32 x1,y1,x2,y2;
	s32 pix_cnt=0;
	s32 cnt=0;
	float pix_usec_ratio;
	s32 num_pixels;
	u32 start_time,end_time,used_time;

	thread->mutex_down(queue_mutex);
	
//	if (last!=first) printf("exec redraw %lu pixels\n",max_pixels);
//	if (first!=last) printf("first: %lu, last: %lu, first-last: %lu\n",first,last,first-last);

	start_time = timer->get_time();
	max_time -= 500;
	used_time = 0;
	bf=0;af=0;
	while ((last != first) && (used_time < max_time)) {
		cnt++;
		
		pt=used_time;
		
		x1=action_queue[last].x1;
		y1=action_queue[last].y1;
		x2=action_queue[last].x2;
		y2=action_queue[last].y2;

		w=x2-x1;
		h=y2-y1;
				
		num_pixels = ((s32)max_time - (s32)used_time) * pix_per_usec_min;

		pn=num_pixels;

		if (num_pixels < 100) break;
		
		if (w*h > num_pixels) {
		
			/* subdivide last action */
			cut_h = num_pixels / w;
			
			if (cut_h>1) {
			
			/* reduce size of current queue element */
			action_queue[last].y1 += cut_h;

//			printf("exec_redraw: max_pixels=%lu w=%lu cut_h=%lu\n",max_pixels,w,cut_h);
			
			/* draw the cutted part */
			
			bf = timer->get_time();
			winman->draw((WINDOW *)action_queue[last].wid,x1,y1,x2,y1+cut_h-1);
//			winman->draw(NULL,x1,y1,x2,y1+cut_h-1);
			bf = timer->get_diff(bf,timer->get_time());
			af = cut_h;
			}
		} else {
			af=0;bf=0;
			/* the last queue element can be processed completely */
			bf = timer->get_time();
			winman->draw((WINDOW *)action_queue[last].wid,x1,y1,x2,y2);		
//			winman->draw(NULL,x1,y1,x2,y2);		
			bf = timer->get_diff(bf,timer->get_time());
			last = (last+1)%REDRAW_QUEUE_SIZE;
		}
		pix_cnt += w*h;		
		used_time = timer->get_diff(start_time,timer->get_time());
	}
	
	end_time = timer->get_time();
	
	if (pix_cnt>5000) {
		used_time = timer->get_diff(start_time,end_time);
		
		if (used_time> 100) {
			pix_usec_ratio = (float)pix_cnt / (float)used_time;
			pix_per_usec_avr = 0.95*pix_per_usec_avr + 
							   0.05*pix_usec_ratio;
			if (pix_usec_ratio < pix_per_usec_min) 
				pix_per_usec_min = pix_usec_ratio;
			
		}
	}
	if (last==first) {
		thread->mutex_up(queue_mutex);
		return 0;
	}
	thread->mutex_up(queue_mutex);
	return 1;
}


static float get_avr_ppt(void) {
	return pix_per_usec_avr;
}

static float get_min_ppt(void) {
	return pix_per_usec_min;
}

/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct redraw_services services = {
	draw_area,
	draw_widget,
	draw_widgetarea,
	exec_redraw,
	get_noque,
	get_avr_ppt,
	get_min_ppt,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_redraw(struct dope_services *d) {
	
	winman	=	d->get_module("WindowManager 1.0");	
	thread	=	d->get_module("Thread 1.0");
	timer	=	d->get_module("Timer 1.0");

	queue_mutex = thread->create_mutex(0);
	
	d->register_module("RedrawManager 1.0",&services);
	return 1;
}
