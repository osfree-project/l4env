/*
 * \brief   DOpE redraw manager module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module handles the redraw of non-realtime
 * widgets.  Redraw-actions are stored in a queue
 * (by widgets) and executed later (at the end of
 * a period).
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct private_widget;
#define WIDGET struct private_widget

#include "dopestd.h"
#include "widget_data.h"
#include "widget.h"
#include "screen.h"
#include "thread.h"
#include "redraw.h"
#include "timer.h"

static struct thread_services   *thread;
static struct timer_services    *timer;

WIDGET {
	struct widget_methods   *gen;   /* pointer to general methods */
	void                    *widget_specific_methods;
	struct widget_data      *wd;    /* pointer to general attributes */
};

#define REDRAW_QUEUE_SIZE 5000

struct action {
	WIDGET *wid;             /* associated widget */
	s32     x1, y1, x2, y2;  /* area on screen    */
};

static struct action action_queue[REDRAW_QUEUE_SIZE];
static s32 first = 0;
static s32 last  = 0;

extern int config_adapt_redraw;  /* from startup.c */

int init_redraw(struct dope_services *d);


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** RETURN THE NUMBER OF CURRENTRY QUEUED ELEMENTS ***/
static u32 get_noque(void) {
	return (first + REDRAW_QUEUE_SIZE - last) % REDRAW_QUEUE_SIZE;
}


/*** DETERMINE IF SPECIFIED WIDGET IS CURRENTLY QUEUED ***/
static s32 is_queued(WIDGET *w) {
	s32 cmp_idx = last;
	while (cmp_idx!=first) {
		if (action_queue[cmp_idx].wid == w) return 1;
		cmp_idx = (cmp_idx + 1) % REDRAW_QUEUE_SIZE;
	}
	return 0;
}


static void add_redraw_action(WIDGET *w, long x1, long y1, long x2, long y2, long curr_idx) {
	if (x1 > x2 || y1 > y2) return;

	curr_idx++;
	if (curr_idx > first) curr_idx = first;

	/* search for queue entry that affect the same widget */
	while (curr_idx != first && action_queue[curr_idx].wid != w) {
		curr_idx = (curr_idx + 1) % REDRAW_QUEUE_SIZE;
	}

	/* if there are no queue entries left to check - add the action */
	if (curr_idx == first) {
		w->gen->inc_ref(w);
		action_queue[first].wid = w;
		action_queue[first].x1  = x1;
		action_queue[first].y1  = y1;
		action_queue[first].x2  = x2;
		action_queue[first].y2  = y2;
		first = (first + 1) % REDRAW_QUEUE_SIZE;
		return;
	}

	/* merge both redraw requests */
	action_queue[curr_idx].x1 = MIN(action_queue[curr_idx].x1, x1);
	action_queue[curr_idx].y1 = MIN(action_queue[curr_idx].y1, y1);
	action_queue[curr_idx].x2 = MAX(action_queue[curr_idx].x2, x2);
	action_queue[curr_idx].y2 = MAX(action_queue[curr_idx].y2, y2);
}


/*** PUT NEW REDRAW-ACTION INTO QUEUE ***/
static void draw_area(WIDGET *cw, int cx1, int cy1, int cx2, int cy2) {

	/* the parent of a window is a screen, the screen has no parent */
	while (cw && cw->wd->parent && cw->wd->parent->wd->parent) {

		/* increment position by relative widget position */
		cx1 += cw->wd->x;
		cy1 += cw->wd->y;
		cx2 += cw->wd->x;
		cy2 += cw->wd->y;

		cw = cw->wd->parent;

		/* shrink current area to parent view area */
		if (cx1 < 0) cx1 = 0;
		if (cy1 < 0) cy1 = 0;
		if (cx2 > cw->wd->w - 1) cx2 = cw->wd->w - 1;
		if (cy2 > cw->wd->h - 1) cy2 = cw->wd->h - 1;
	}

	if (cw) add_redraw_action(cw, cx1, cy1, cx2, cy2, last);
}


/*** GENERATE ACTION TO REDRAW A SPECIFIED WIDGET ***/
static void draw_widget(WIDGET *cw) {
	draw_area(cw, 0, 0, cw->wd->w - 1, cw->wd->h - 1);
}


/*** GENERATE ACTION TO REDRAW AN AREA OF A SPECIFIED WIDGET ***/
static void draw_widgetarea(WIDGET *cw, int rx1, int ry1, int rx2, int ry2) {
	draw_area(cw, rx1, ry1, rx2, ry2);
}


/*** ADAPT AVERAGE PIXEL PER USEC RATIO ***
 *
 * \param value   old average ratio to adopt
 * \param pixels  processed pixels
 * \param usec    time needed to process the pixels
 * \return        new average ratio
 */
static inline float adapt_pix_per_usec(float value, int pixels, float usec) {
	float pix_usec_ratio = (float)pixels / usec;
	return 0.95 * value + 0.05 * pix_usec_ratio;
}


/*** PROCESS THE REDRAW OF THE SPECIFIED AMOUNT OF PIXELS ***
 *
 * \param max_pixels   max amount of pixels to process
 * \return             number of actually processed pixels
 *
 * This function takes redraw requests from the queue and executes them.
 * If a request is bigger than max_pixels, only a fraction of the
 * request is executed and the remaining part stays at the queue.
 */
static inline s32 process_pixels(s32 max_pixels) {
	WIDGET *cw;
	int x, y, w, h, cut_h;
	int processed_pixels = 0;

	while (last != first && max_pixels > 0) {
		
		/* get pending redraw request */
		cw = action_queue[last].wid;
		x  = action_queue[last].x1;
		y  = action_queue[last].y1;
		w  = action_queue[last].x2 - x + 1;
		h  = action_queue[last].y2 - y + 1;
		
		/* calc fraction of request to be processed */
		cut_h = max_pixels / w;
		cut_h = (cut_h < h) ? cut_h : h;

		if (cut_h == 0) break;

		/* process redraw */
		if (cw && w > 0 && cut_h > 0) {
			cw->gen->lock(cw);
			cw->gen->drawarea(cw, cw, x, y, w, cut_h);
			cw->gen->unlock(cw);
			max_pixels       -= w * cut_h;
			processed_pixels += w * cut_h;
		}

		/* shrink request by the processed area */
		action_queue[last].y1 += cut_h;

		/* kick request out of the queue if it is completed */
		if (cut_h >= h) {
			WIDGET *w;
			if ((w = action_queue[last].wid))
				w->gen->dec_ref(w);
			last = (last + 1) % REDRAW_QUEUE_SIZE;
		}
	}
	return processed_pixels;
}


/*** EXECUTE REDRAW REQUEST QUEUE ***
 *
 * \param avail_time   time available for executing redraw requests
 */
static s32 exec_redraw(s32 avail_time) {
	static float pix_per_usec = 8.0;   /* average number of pixels per usec    */
	int used_time = 0;                 /* time used for the iteration          */
	int pix_cnt   = 0;                 /* number of overall processed pixels   */
	int min_pix   = 1000;              /* minimal number of pixels to process  */
	int num_pix   = 0;                 /* number of currently processed pixels */
	u32 start_time;                    /* start time of drawing                */

	start_time = timer->get_time();

	/* process pixels as long as there are due redraw requests and there is time left */
	while (last != first && used_time < avail_time) {

		/* determine number of pixels that can be processed in the available time */
		num_pix = (avail_time - used_time) * pix_per_usec;

		/* sanity check */
		if (num_pix < 0) break;

		/* process num_pix pixels */
		pix_cnt += process_pixels(num_pix);

		used_time = timer->get_diff(start_time, timer->get_time());
	}

	/* adapt the pix_per_usec ratio */
	if (config_adapt_redraw && num_pix > 10000)
		pix_per_usec = adapt_pix_per_usec(pix_per_usec, pix_cnt, used_time);

	/*
	 * Clamp the pix/usec ratio to a lower border. In some situation
	 * (for example if someone switches off interrupts for some time),
	 * the measurement of drawing duration times may be messed up.
	 * This could cause DOpE to adapt to these timing constrains in a
	 * way that only a few pixels are drawn for each period, which
	 * raises the overhead of traversing widget structures and clipping
	 * an never allows DOpE to recover from that situation. Thus,
	 * limiting the adaption to a lower border is needed to preserve
	 * robustness even in such bad situations.
	 */
	if (pix_per_usec < 8.0) pix_per_usec = 8.0;

	/*
	 * If there was not enough time to process min_pix pixels
	 * draw min_pix pixels to keep the user interface alive.
	 */
	if (pix_cnt < min_pix) process_pixels(min_pix);

	return 1;
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct redraw_services services = {
	draw_area,
	draw_widget,
	draw_widgetarea,
	exec_redraw,
	process_pixels,
	get_noque,
	is_queued,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_redraw(struct dope_services *d) {

	thread  =   d->get_module("Thread 1.0");
	timer   =   d->get_module("Timer 1.0");

	d->register_module("RedrawManager 1.0", &services);
	return 1;
}
