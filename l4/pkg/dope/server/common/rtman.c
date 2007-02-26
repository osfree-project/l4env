/*
 * \brief   DOpE real-time manager module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This real-time manager module is just examplary.
 * It restricts the number of real-time widgets to
 * four.  There are  also no  time-exceeding tests
 * performed.
 * There are four time-slots where real-time opera-
 * tions can be  (interleaved) executed.  Given an
 * execution frequency  of 100Hz - this rt-manager
 * module displays the rt-widgets at 25fps.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct private_widget;
#define WIDGET struct private_widget

#include "dopestd.h"
#include "thread.h"
#include "widget_data.h"
#include "widget.h"
#include "winman.h"
#include "rtman.h"

#define NUM_SLOTS 4

struct timeslot {
	WIDGET *w;
	float   duration;
	MUTEX  *sync_mutex;
};

WIDGET {
	struct widget_methods   *gen;   /* pointer to general methods */
	void                    *widget_specific_methods;
	struct widget_data      *wd;    /* pointer to general attributes */
};

static struct winman_services *winman;
static struct thread_services *thread;

static struct timeslot ts[NUM_SLOTS];
static s32 curr_slot = 0;


int init_rtman(struct dope_services *d);


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** EXECUTE RT-REDRAW OPERATIONS THAT BECOME DUE AT THE CURRENT TIME SLOT ***/
/* This function must be periodically called from within the eventloop. */
static void rt_execute_redraw(void) {
	WIDGET *cw = ts[curr_slot].w;
	long cx1,cy1,cx2,cy2;
	WIDGET *win=NULL;
	MUTEX *sync_mutex = ts[curr_slot].sync_mutex;

	/* cycle trough time slots */
	curr_slot++;
	if (curr_slot >= NUM_SLOTS) curr_slot -= NUM_SLOTS;

	if (!cw) return;

	/* widget area relative to its parent */
	cx1 = cw->wd->x;
	cy1 = cw->wd->y;
	cx2 = cx1 + cw->wd->w - 1;
	cy2 = cy1 + cw->wd->h - 1;

	cw=cw->wd->parent;
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

	if (cw) winman->draw((WINDOW *)win,cx1,cy1,cx2,cy2);
	if (sync_mutex) thread->mutex_up(sync_mutex);
}



/*** REGISTER NEW REAL-TIME WIDGET ***/
static s32 rt_add_widget(WIDGET *w,float duration) {
	s32 free_slot = -1;
	s32 i;

	/* search free slot */
	for (i=0;i<NUM_SLOTS;i++) {
		if (!ts[i].w) free_slot = i;
	}

	if (free_slot == -1) return -1;

	/* settle down at time slot */
	ts[free_slot].w = w;
	ts[free_slot].duration = duration;
	w->gen->inc_ref(w);

	return 0;
}



/*** UNREGISTER A REAL-TIME WIDGET ***/
static void rt_remove_widget(WIDGET *w) {
	s32 i;

	/* search slot of the given widget */
	for (i=0;i<NUM_SLOTS;i++) {

		/* free the widget's time slot */
		if (ts[i].w == w) {
			ts[i].w = NULL;
			ts[i].duration = 0;
			ts[i].sync_mutex = NULL;
			w->gen->dec_ref(w);
			return;
		}
	}
}



/*** SET MUTEX THAT SHOULD BE UNLOCKED AFTER DRAWING OPERATIONS ***/
static void rt_set_sync_mutex(WIDGET *w,MUTEX *m) {
	s32 i;

	/* search slot of the given widget */
	for (i=0;i<NUM_SLOTS;i++) {

		if (ts[i].w == w) {
			ts[i].sync_mutex = m;
			return;
		}
	}
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct rtman_services services = {
	rt_execute_redraw,
	rt_add_widget,
	rt_remove_widget,
	rt_set_sync_mutex,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_rtman(struct dope_services *d) {

	winman = d->get_module("WindowManager 1.0");
	thread = d->get_module("Thread 1.0");

	d->register_module("RTManager 1.0",&services);
	return 1;
}
