/*
 * \brief   DOpE user state module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component manages the different states of,
 * the user interface. These states depend on the
 * users action.
 *
 * idle       - user drinks coffee
 * drag       - user drags a widget with the mouse
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
#include "event.h"
#include "widget.h"
#include "input.h"
#include "scrdrv.h"
#include "winman.h"
#include "userstate.h"
#include "redraw.h"
#include "messenger.h"

static struct input_services        *input;
static struct winman_services       *winman;
static struct scrdrv_services       *scrdrv;
static struct redraw_services       *redraw;
static struct messenger_services    *msg;

static s32     omx,omy;               /* original mouse postion */
static s32     curr_mx=0,curr_my=0;   /* current mouse position */
static s32     curr_mb=0;             /* current mouse button state */
static s32     curr_state=0;
static WIDGET *curr_selected=NULL;    /* currently selected widget */
static WIDGET *curr_focus=NULL;       /* currently focused widget */
static void  (*curr_motion_callback)  (WIDGET *,int,int);
static void  (*curr_release_callback) (WIDGET *,int,int);
static void  (*curr_tick_callback)    (WIDGET *,int,int);
static EVENT   event;


int init_userstate(struct dope_services *d);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** RELEASE CURRENT USERSTATE ***/
static void leave_current(void) {

	curr_mx = input->get_mx();
	curr_my = input->get_my();

	switch (curr_state) {
	case USERSTATE_DRAG:
		if (curr_release_callback) {
			curr_release_callback(curr_selected, curr_mx - omx, curr_my - omy);
		}
		break;
	case USERSTATE_TOUCH:
		if (curr_release_callback) {
			curr_release_callback(curr_selected, curr_mx - omx, curr_my - omy);
		}
		curr_selected->gen->set_state(curr_selected, 0);
		if (curr_focus != curr_selected) {
			curr_selected->gen->set_focus(curr_selected,0);
		}
		curr_selected->gen->update(curr_selected, WID_UPDATE_REDRAW);
		break;
	case USERSTATE_GRAB:
		input->set_pos(omx,omy);
		scrdrv->set_mouse_pos(omx,omy);
		break;
	default:
		break;
	}
}


/*** SET NEW USER STATE ***/
static void idle(void) {
	leave_current();
	curr_state = USERSTATE_IDLE;
}


/*** ENTER TOUCH-USERSTATE ***/
static void touch(WIDGET *w, void (*tick_callback)   (WIDGET *,int dx, int dy),
                             void (*release_callback)(WIDGET *,int dx, int dy)) {
	leave_current();
	curr_selected = w;
	omx = input->get_mx();
	omy = input->get_my();
	curr_mb = input->get_mb();
	curr_tick_callback    = tick_callback;
	curr_release_callback = release_callback;
	curr_state = USERSTATE_TOUCH;
	curr_selected->gen->set_state(curr_selected, 1);
	curr_selected->gen->update(curr_selected, WID_UPDATE_REDRAW);
}


/*** ENTER DRAG-USERSTATE ***/
static void drag(WIDGET *w, void (*motion_callback) (WIDGET *,int dx, int dy),
                            void (*tick_callback)   (WIDGET *,int dx, int dy),
                            void (*release_callback)(WIDGET *,int dx, int dy)) {
	leave_current();
	curr_selected = w;
	omx = input->get_mx();
	omy = input->get_my();
	curr_mb = input->get_mb();
	curr_motion_callback  = motion_callback;
	curr_tick_callback    = tick_callback;
	curr_release_callback = release_callback;
	curr_state = USERSTATE_DRAG;
}


/*** ENTER MOUSE GRAB USERSTATE ***/
static void grab(WIDGET *w, void (*tick_callback) (WIDGET *, int, int)) {
	leave_current();
	omx = input->get_mx();
	omy = input->get_my();
	scrdrv->set_mouse_pos(5000,5000);
	curr_tick_callback = tick_callback;
	curr_selected = w;
	curr_state = USERSTATE_GRAB;
}


static long get(void) {
	return curr_state;
}


static void handle(void) {
	static long new_mx,new_my,new_mb;
	static WIDGET *new_focus = NULL;
	static long update_needed = 0;

	new_mx=input->get_mx();
	new_my=input->get_my();
	new_mb=input->get_mb();

	switch (curr_state) {
	case USERSTATE_IDLE:
		new_focus=winman->find(new_mx,new_my);
		if (new_focus!=curr_focus) {
			if (curr_focus) {
				curr_focus->gen->set_focus(curr_focus,0);
				curr_focus->gen->update(curr_focus,WID_UPDATE_REDRAW);
				event.type=EVENT_MOUSE_LEAVE;
				curr_focus->gen->handle_event(curr_focus,&event);
			}
			if (new_focus) {
				new_focus->gen->set_focus(new_focus,1);
				new_focus->gen->update(new_focus,WID_UPDATE_REDRAW);
				event.type=EVENT_MOUSE_ENTER;
				new_focus->gen->handle_event(new_focus,&event);
			}
			curr_focus=new_focus;
		}

		/* if mouse position changed -> deliver motion event */
		if ((new_mx!=curr_mx) || (new_my!=curr_my)) {
			if (new_focus) {
				event.type=EVENT_MOTION;
				event.abs_x=new_mx - new_focus->gen->get_abs_x(new_focus);
				event.abs_y=new_my - new_focus->gen->get_abs_y(new_focus);
				event.rel_x=new_mx - curr_mx;
				event.rel_y=new_my - curr_my;
				new_focus->gen->handle_event(new_focus,&event);
			}
		}
		break;
		
	case USERSTATE_TOUCH:
		
		if (curr_tick_callback) {
			curr_tick_callback(curr_selected, new_mx - omx, new_my - omy);
		}
		
		if (new_mb != curr_mb) idle();
		
		new_focus=winman->find(new_mx,new_my);
		if (new_focus!=curr_focus) {
			if (new_focus == curr_selected) {
				curr_selected->gen->set_state(curr_selected,1);
				curr_selected->gen->update(curr_selected,WID_UPDATE_REDRAW);
			} else {
				curr_selected->gen->set_state(curr_selected,0);
				curr_selected->gen->update(curr_selected,WID_UPDATE_REDRAW);
			}
			curr_focus=new_focus;
		}
		break;

	case USERSTATE_DRAG:

		if (new_mb != curr_mb) idle();

		if (curr_tick_callback) {
			curr_tick_callback(curr_selected, new_mx - omx, new_my - omy);
		}
		if ((new_mx!=curr_mx) || (new_my!=curr_my) || update_needed) {
			update_needed = 1;
			if (curr_motion_callback && (!redraw->is_queued(NULL))) {
				update_needed = 0;
				curr_motion_callback(curr_selected, new_mx - omx, new_my - omy);
			}
		}
		break;

	case USERSTATE_GRAB:

		/* if mouse position changed -> deliver motion event */
		if ((new_mx!=curr_mx) || (new_my!=curr_my)) {
			s32 min_x = curr_selected->gen->get_abs_x(curr_selected);
			s32 min_y = curr_selected->gen->get_abs_y(curr_selected);
			s32 max_x = min_x + curr_selected->gen->get_w(curr_selected) - 1;
			s32 max_y = min_y + curr_selected->gen->get_h(curr_selected) - 1;

			if ((new_mx < min_x) || (new_my < min_y) ||
			    (new_mx > max_x) || (new_my > max_y)) {
				if (new_mx < min_x) new_mx = min_x;
				if (new_my < min_y) new_my = min_y;
				if (new_mx > max_x) new_mx = max_x;
				if (new_my > max_y) new_my = max_y;
				input->set_pos(new_mx,new_my);
			}

			event.type=EVENT_MOTION;
			event.abs_x=new_mx - min_x;
			event.abs_y=new_my - min_y;
			event.rel_x=new_mx - curr_mx;
			event.rel_y=new_my - curr_my;
			curr_selected->gen->handle_event(curr_selected,&event);
		}
		if (curr_tick_callback) {
			curr_tick_callback(curr_selected, new_mx - omx, new_my - omy);
		}
		break;
	}
	scrdrv->set_mouse_pos(new_mx,new_my);
	curr_mx=new_mx;
	curr_my=new_my;
	curr_mb=new_mb;
}


static WIDGET *get_curr_focus(void) {
	return curr_focus;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct userstate_services services = {
	idle,
	touch,
	drag,
	grab,
	get,
	handle,
	get_curr_focus,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_userstate(struct dope_services *d) {

	input   = d->get_module("Input 1.0");
	winman  = d->get_module("WindowManager 1.0");
	scrdrv  = d->get_module("ScreenDriver 1.0");
	redraw  = d->get_module("RedrawManager 1.0");
	msg     = d->get_module("Messenger 1.0");

	d->register_module("UserState 1.0",&services);
	return 1;
}
