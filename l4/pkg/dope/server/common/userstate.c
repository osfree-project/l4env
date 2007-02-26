/*
 * \brief	DOpE user state module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This component manages the different states of,
 * the user interface. These states depend on the
 * users action.  For each situation there exists
 * a corresponding  user state.  The  implemented
 * states are:
 * 
 * idle       - user drinks coffee
 * scrollstep - user presses  a scrollbar's arrow. 
 *              It  causes an  accellerated  move-
 *              ment of the slider
 * winmove    - user is currently moving a window
 * winsize    - user resizes a window
 * scrolldrag - user drags the slider of a scroll-
 *              bar
 */


#include "dope-config.h"
#include "event.h"
#include "widget.h"
#include "input.h"
#include "screen.h"
#include "winman.h"
#include "scrollbar.h"
#include "userstate.h"
#include "redraw.h"
#include "messenger.h"

static struct input_services        *input;
static struct winman_services       *winman;
static struct screen_services       *scr;
static struct redraw_services       *redraw;
static struct messenger_services    *msg;

static long          owx1,owy1,owx2,owy2;   /* original window area */
static long          nwx1,nwy1,nwx2,nwy2;   /* new window area */
static long          omx,omy;               /* original mouse postion */
static long          osx,osy;               /* original slider position */
static long          curr_mx=0,curr_my=0;   /* current mouse position */
static long          curr_mb=0;             /* current mouse button state */
static float         curr_scroll_speed;     /* current scroll speed */
static float         max_scroll_speed=20;   /* maximal scroll speed */
static float         scroll_accel=0.4;      /* scroll acceleration */
static long          curr_state=0;
static WIDGET       *curr_selected=NULL;    /* currently selected widget */
static WIDGET       *curr_focus=NULL;       /* currently focused widget */
static WINDOW       *curr_window=NULL;      /* currently modified window */
static SCROLLBAR    *curr_scrollbar=NULL;   /* currently modified scrollbar */
static EVENT         event;
static long          moved_flag;


int init_userstate(struct dope_services *d);

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** SET NEW USER STATE ***/
static void set(long new_state,WIDGET *cw) {

	switch (curr_state) {
	case USERSTATE_IDLE:
	case USERSTATE_SCROLLSTEP:
			break;
	case USERSTATE_WINMOVE:
		if (curr_window && moved_flag) {
			u8 *m  = curr_window->gen->get_bind_msg(curr_window,"moved");
			s32 id = curr_window->gen->get_app_id(curr_window);
			if (m) msg->send_action_event(id,"moved",m);
		}
		break;
	case USERSTATE_WINSIZE:
		if (curr_window && moved_flag) {
			u8  *m = curr_window->gen->get_bind_msg(curr_window,"resized");
			s32 id = curr_window->gen->get_app_id(curr_window);
			if (m) msg->send_action_event(id,"resized",m);
			if ((owx1!=nwx1) || (owy1!=nwy1)) {
				m = curr_window->gen->get_bind_msg(curr_window,"moved");
				if (m) msg->send_action_event(id,"moved",m);
			}
		}
		break;
	case USERSTATE_SCROLLDRAG:
		break;
	}


	switch (new_state) {
	case USERSTATE_IDLE:
		DOPEDEBUG(printf("UserState(set): entering IDLE userstate\n"));
		/* deselect current widget */
		if (curr_selected) {
			curr_selected->gen->set_state(curr_selected,0);
			curr_selected->gen->update(curr_selected,WID_UPDATE_REDRAW);
		}
		break;
	
	case USERSTATE_WINMOVE:
		DOPEDEBUG(printf("UserState(set): entering WINMOVE userstate\n"));
	
		/* select current widget */
		cw->gen->set_state(cw,1);
		cw->gen->update(cw,WID_UPDATE_REDRAW);
		moved_flag=0;
		
		/* determine the associated window */
		curr_window=(WINDOW *)cw->gen->get_window(cw);
		curr_selected=cw;

		nwx1 = owx1 = curr_window->gen->get_x(curr_window);
		nwy1 = owy1 = curr_window->gen->get_y(curr_window);

		break;
		
	case USERSTATE_WINSIZE:
		DOPEDEBUG(printf("UserState(set): entering WINSIZE userstate\n"));
		/* select current widget */
		cw->gen->set_state(cw,1);
		cw->gen->update(cw,WID_UPDATE_REDRAW);
		moved_flag=0;

		/* determine the associated window */
		curr_window=(WINDOW *)cw->gen->get_window(cw);
		curr_selected=cw;
		
		nwx1 = owx1 = curr_window->gen->get_x(curr_window);
		nwy1 = owy1 = curr_window->gen->get_y(curr_window);
		nwx2 = owx2 = owx1 + curr_window->gen->get_w(curr_window) - 1;
		nwy2 = owy2 = owy1 + curr_window->gen->get_h(curr_window) - 1;
		
		omx = input->get_mx();
		omy = input->get_my();
		
		break;

	case USERSTATE_SCROLLDRAG:
		DOPEDEBUG(printf("UserState(set): entering SCROLLDRAG userstate\n"));
	
		/* determine the associated window */
		curr_scrollbar=(SCROLLBAR *)cw;

		omx = input->get_mx();
		omy = input->get_my();
		
		osx = curr_scrollbar->scroll->get_slider_x(curr_scrollbar);
		osy = curr_scrollbar->scroll->get_slider_y(curr_scrollbar);
		
		break;

	case USERSTATE_SCROLLSTEP:
		DOPEDEBUG(printf("UserState(set): entering SCROLLSTEP userstate\n"));
	
		/* select current widget */
		cw->gen->set_state(cw,1);
		cw->gen->update(cw,WID_UPDATE_REDRAW);
		
		/* determine the associated window */
		curr_selected=cw;
		curr_scrollbar=(SCROLLBAR *)cw->gen->get_parent(cw);

		curr_scroll_speed=0.6;
		break;
	}
	curr_state=new_state;
}


static long get(void) {
	return curr_state;
}

static void handle(void) {
	static long new_mx,new_my,new_mb;
	static WIDGET *new_focus;
	static long size_flags;
	static long update_needed=0;
	
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
				event.motion.abs_x=new_mx - new_focus->gen->get_abs_x(new_focus);
				event.motion.abs_y=new_my - new_focus->gen->get_abs_y(new_focus);
				event.motion.rel_x=new_mx - curr_mx;
				event.motion.rel_y=new_my - curr_my;
				new_focus->gen->handle_event(new_focus,&event);	
			}
		}
		
		/* if mouse state changed -> deliver up/down event */
/*		if (new_mb!=curr_mb) {
			if (new_mb) {
				event.type=EVENT_PRESS;
				event.press.code=BTN_LEFT;
				if (new_focus) new_focus->gen->handle_event(new_focus,&event);
			} else {
				event.type=EVENT_RELEASE;
				event.press.code=BTN_LEFT;
				if (new_focus) new_focus->gen->handle_event(new_focus,&event);
			}
		}*/
		break;
		
	case USERSTATE_WINMOVE:
		
		/* if mouse moved, update position of current window */
		if ((new_mx!=curr_mx) || (new_my!=curr_my) || update_needed) {
			if (curr_window) {
				nwx1 += new_mx - curr_mx;
				nwy1 += new_my - curr_my;
				update_needed=1;
				if (redraw->get_noque()==0) {
					curr_window->gen->set_x(curr_window,nwx1);
					curr_window->gen->set_y(curr_window,nwy1);
					curr_window->gen->update(curr_window,WID_UPDATE_REDRAW);
					update_needed=0;
				} 
			}
			moved_flag=1;
		}
		
		/* is there a change of the mouse buttons */
		if (new_mb!=curr_mb) {
			if (!new_mb) {
				set(USERSTATE_IDLE,curr_selected);
				if (!moved_flag) {
					if (curr_window) curr_window->win->top(curr_window);
				} else {
					if (curr_window) {
						curr_window->gen->update(curr_window,WID_UPDATE_REDRAW);
					}
				}
				
			}
		}
		break;
	
	case USERSTATE_WINSIZE:
	
		/* is there a change of the mouse buttons */
		if (new_mb!=curr_mb) {
			if (!new_mb) {
				set(USERSTATE_IDLE,curr_selected);
				if (!moved_flag) {
					if (curr_window) curr_window->win->top(curr_window);
				} else {
					if (curr_window) {
						curr_window->gen->update(curr_window,WID_UPDATE_REDRAW);
					}
				}
			}
		}
		
		if (!curr_window) break;
	
		size_flags=(long)curr_selected->gen->get_context(curr_selected);
		
		/* if mouse horizontaly moved, update position of current window */
		if (new_mx!=curr_mx) {
			long min_w=curr_window->gen->get_min_w(curr_window);
			long max_w=curr_window->gen->get_max_w(curr_window);
			/* flag for left resizing */
			if (size_flags & 1) {
				nwx1 = owx1 + new_mx - omx;
				if (nwx1 + min_w - 1 > owx2) nwx2 = nwx1 + min_w - 1;
				else if (nwx1 + max_w - 1 < owx2) nwx2 = nwx1 + max_w - 1;
				else nwx2 = owx2;
			}
			/* flag for right resizing */
			if (size_flags & 4) {
				nwx2 = owx2 + new_mx - omx;
				if (owx1 + min_w - 1 > nwx2) nwx1 = nwx2 - min_w + 1;
				else if (owx1 + max_w - 1 < nwx2) nwx1 = nwx2 - max_w + 1;
				else nwx1 = owx1;
			}
		}
		
		/* if mouse verticaly moved, update position of current window */
		if (new_my!=curr_my) {
			long min_h=curr_window->gen->get_min_h(curr_window);
			long max_h=curr_window->gen->get_max_h(curr_window);
			/* flag for top resizing */
			if (size_flags & 2) {
				nwy1 = owy1 + new_my - omy;
				if (nwy1 + min_h - 1 > owy2) nwy2 = nwy1 + min_h - 1;
				else if (nwy1 + max_h - 1 < owy2) nwy2 = nwy1 + max_h - 1;
				else nwy2 = owy2;				
			}
			/* flag for bottom resizing */
			if (size_flags & 8) {
				nwy2 = owy2 + new_my - omy;
				if (owy1 + min_h - 1 > nwy2) nwy1 = nwy2 - min_h + 1;
				else if (owy1 + max_h - 1 <nwy2) nwy1 = nwy2 - max_h + 1;
				else nwy1 = owy1;
			}
		}
		
		/* update window if its size changed */
		if ((new_mx!=curr_mx) || (new_my!=curr_my) || update_needed) {
			update_needed=1;
			if (redraw->get_noque()==0) {
				curr_window->gen->set_x(curr_window,nwx1);
				curr_window->gen->set_y(curr_window,nwy1);
				curr_window->gen->set_w(curr_window,nwx2-nwx1+1);
				curr_window->gen->set_h(curr_window,nwy2-nwy1+1);
				curr_window->gen->update(curr_window,WID_UPDATE_REDRAW);
				update_needed=0;
			}
			moved_flag=1;
		}
		
		break;
		
	case USERSTATE_SCROLLDRAG:

		/* mousebutton leaved? */
		if (new_mb!=curr_mb) {
			if (!new_mb) {
				set(USERSTATE_IDLE,(WIDGET *)curr_scrollbar);
			}
		}

		/* mouse moved horizontally */
		if (new_mx!=curr_mx) {
			curr_scrollbar->scroll->set_slider_x(curr_scrollbar,osx + new_mx - omx);
		}
		
		/* mouse moved vertically */
		if (new_my!=curr_my) {
			curr_scrollbar->scroll->set_slider_y(curr_scrollbar,osy + new_my - omy);
		}

		/* update scrollbar if mouse moved */
		if ((new_mx!=curr_mx) || (new_my!=curr_my)) {
			curr_scrollbar->gen->update(curr_scrollbar,WID_UPDATE_REDRAW);
		}
	
		break;

	case USERSTATE_SCROLLSTEP:

		/* mousebutton leaved? */
		if (new_mb!=curr_mb) {
			if (!new_mb) {
				curr_selected->gen->set_state(curr_selected,0);
				curr_selected->gen->update(curr_selected,WID_UPDATE_REDRAW);
				set(USERSTATE_IDLE,(WIDGET *)curr_scrollbar);
			}
		}

		if (curr_selected->gen->get_context(curr_selected)) {
			s32 offset = curr_scrollbar->scroll->get_view_offset(curr_scrollbar);
			curr_scrollbar->scroll->set_view_offset(curr_scrollbar,offset - (s32)curr_scroll_speed);
		} else {
			s32 offset = curr_scrollbar->scroll->get_view_offset(curr_scrollbar);
			curr_scrollbar->scroll->set_view_offset(curr_scrollbar,offset + (s32)curr_scroll_speed);		
		}
		curr_scrollbar->gen->update(curr_scrollbar,WID_UPDATE_REDRAW);
		
		curr_scroll_speed += scroll_accel;
		if (curr_scroll_speed>max_scroll_speed) curr_scroll_speed=max_scroll_speed;

		break;
	}
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
	set,
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
	scr     = d->get_module("Screen 1.0");
	redraw  = d->get_module("RedrawManager 1.0");
	msg     = d->get_module("Messenger 1.0");
	
	d->register_module("UserState 1.0",&services);
	return 1;
}
