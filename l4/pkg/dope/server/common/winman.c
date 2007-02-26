/*
 * \brief	DOpE window manager module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * The window manager defines how windows are arranged 
 * on the screen. It manages their stacking order and
 * and positions. Since it is the only component that
 * knows about all windows and its visible areas it is 
 * responsible for drawing them.
 */

#include "dope-config.h"
#include "clipping.h"
#include "screen.h"
#include "widget.h"
#include "window.h"
#include "redraw.h"
#include "winman.h"
#include "button.h" /*!!! temporary !!!*/
#include "appman.h" /*!!! temporary !!!*/

#define MAX(a,b) a>b?a:b
#define MIN(a,b) a<b?a:b

static WIDGET *first_win=NULL;
static WIDGET *me= (WIDGET *)'#';
static WINDOW *active_win=NULL;

static struct clipping_services *clip;
static struct screen_services 	*scr;
static struct redraw_services	*redraw;
static struct appman_services	*appman;

static struct dope_services *dope;

extern BUTTON *menubutton;	/*!!! temporary !!!*/

int init_winman(struct dope_services *d);
void print_wininfo(void);


static void draw_rec(WIDGET *cw,long cx1,long cy1,long cx2,long cy2) {
	long	sx1,sy1,sx2,sy2;
	static long d;
	WIDGET *next;
	
	if (!cw) return;	/* fool me? */
	
	/* calc intersection between dirty area and current window */
	sx1=MAX(cx1,(d = cw->gen->get_x(cw)));
	sx2=MIN(cx2,d + cw->gen->get_w(cw) - 1);
	sy1=MAX(cy1,(d = cw->gen->get_y(cw)));
	sy2=MIN(cy2,d + cw->gen->get_h(cw) - 1);

	/* if there is an intersection - subdivide area */	
	if ((sx1<=sx2) && (sy1<=sy2)) {
		
		/* draw current window */
		clip->push(sx1,sy1,sx2,sy2);
		scr->set_draw_area(sx1,sy1,sx2,sy2);
		cw->gen->draw(cw,0,0);
		clip->pop();

		/* take care about the rest */
		if ((next=cw->gen->get_next(cw))==NULL) return;
		if (sx1>cx1) draw_rec(next,cx1,MAX(cy1,sy1),sx1-1,MIN(cy2,sy2));
		if (sy1>cy1) draw_rec(next,cx1,cy1,cx2,sy1-1);
		if (sx2<cx2) draw_rec(next,sx2+1,MAX(cy1,sy1),cx2,MIN(cy2,sy2));
		if (sy2<cy2) draw_rec(next,cx1,sy2+1,cx2,cy2);
	} else {
		draw_rec(cw->gen->get_next(cw),cx1,cy1,cx2,cy2);
	}
}



static void draw_win_rec(WIDGET *cw,WINDOW *win,long cx1,long cy1,long cx2,long cy2) {
	long	sx1,sy1,sx2,sy2;
	static long d;
	WIDGET *next;
	
	if (!cw) return;	/* fool me? */
	if (!win) return;
	
	/* calc intersection between dirty area and current window */
	sx1=MAX(cx1,(d = cw->gen->get_x(cw)));
	sx2=MIN(cx2,d + cw->gen->get_w(cw) - 1);
	sy1=MAX(cy1,(d = cw->gen->get_y(cw)));
	sy2=MIN(cy2,d + cw->gen->get_h(cw) - 1);

	/* if there is an intersection - subdivide area */	
	if ((sx1<=sx2) && (sy1<=sy2)) {
		
		/* draw current window */
		clip->push(sx1,sy1,sx2,sy2);
		if (cw==(WIDGET *)win) {
			scr->set_draw_area(sx1,sy1,sx2,sy2);
			cw->gen->draw(cw,0,0);
			scr->set_draw_area(-1,-1,-1,-1);
			scr->update_area(sx1,sy1,sx2,sy2);
		}
		clip->pop();

		/* take care about the rest */
		if ((next=cw->gen->get_next(cw))==NULL) return;
		if (sx1>cx1) draw_win_rec(next,win,cx1,MAX(cy1,sy1),sx1-1,MIN(cy2,sy2));
		if (sy1>cy1) draw_win_rec(next,win,cx1,cy1,cx2,sy1-1);
		if (sx2<cx2) draw_win_rec(next,win,sx2+1,MAX(cy1,sy1),cx2,MIN(cy2,sy2));
		if (sy2<cy2) draw_win_rec(next,win,cx1,sy2+1,cx2,cy2);
	} else {
		draw_win_rec(cw->gen->get_next(cw),win,cx1,cy1,cx2,cy2);
	}
}



/*** DETERMINE THE LAST 'STAYTOP'-WINDOW OF THE WINDOW STACK ***/
static WINDOW *get_last_staytop_win(void) {
	WINDOW *ltw=(WINDOW *)first_win, *cw;
	while (ltw) {
		cw=(WINDOW *)ltw->gen->get_next(ltw);
		if (!cw) break;
		if (!cw->win->get_staytop(cw)) break;
		if (!ltw->win->get_staytop(ltw)) break;
		ltw=cw;
	}
	if (ltw) {
		if (!ltw->win->get_staytop(ltw)) ltw=NULL;
	}
	return ltw;
}



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** DRAW WINDOWS AT THE SPECIFIED AREA OF THE SCREEN ***/
static void	winman_draw(WINDOW *win,long x1,long y1,long x2,long y2) {
	clip->reset();
	if (win) draw_win_rec(first_win,win,x1,y1,x2,y2);
	else {
		draw_rec(first_win,x1,y1,x2,y2);
		scr->set_draw_area(-1,-1,-1,-1);
		scr->update_area(x1,y1,x2,y2);
	}
	scr->set_draw_area(-1,-1,-1,-1);
}


/*** DRAW WINDOWS BEHIND A WINDOW AT THE SPECIFIED AREA OF THE SCREEN ***/
static void	winman_draw_behind(WINDOW *win,long x1,long y1,long x2,long y2) {

}


/*** SET ACTIVE WINDOW ***/
static void winman_activate(WINDOW *win) {
	s32 app_id;
	if (win!=active_win) {
		if (win) {
			if (win->win->get_staytop(win)) return;
			win->win->set_state(win,1);
		}
		if (active_win) active_win->win->set_state(active_win,0);
		active_win=win;
		app_id = win->gen->get_app_id(win);
		menubutton->but->set_text(menubutton,appman->get_app_name(app_id));
		menubutton->gen->update((WIDGET *)menubutton,1);
	}
}


/*** ADD WINDOW TO THE WINDOW DISPLAY LIST ***/
static void	winman_add(WINDOW *win) {
	WIDGET *cw,*ltw;
	
	DOPEDEBUG(printf("winman_add()\n"));
	
	/* bad child */
	if (!win) return;
	
	/* already adopted this window... dont make this mistake again */
	if (win->gen->get_parent(win)==me) return;
		
	/* determine window stack position for the new window */
	ltw=(WIDGET *)get_last_staytop_win();
	if (ltw) {
	
		/* put window beyond the 'staytop' windows */
		cw = ltw->gen->get_next(ltw);
		ltw->gen->set_next(ltw,(WINDOW *)win);
	} else {
	
		/* put new window at the beginning of the list */
		cw = first_win;
		first_win = (WIDGET *)win;
	}
	win->gen->set_next(win,cw);
	win->gen->set_parent(win,me);
	
	/* redraw the new window... */
	redraw->draw_area(NULL,win->gen->get_x(win),win->gen->get_y(win),
				win->gen->get_x(win)+win->gen->get_w(win)-1,
				win->gen->get_y(win)+win->gen->get_h(win)-1);
}


void print_wininfo(void) {
	WIDGET *cw = first_win;
	while (cw!=NULL) {
		DOPEDEBUG(printf("window (xywh):%lu,%lu,%lu,%lu\n",
					cw->gen->get_x(cw),cw->gen->get_y(cw),
					cw->gen->get_w(cw),cw->gen->get_h(cw)));
		cw=cw->gen->get_next(cw);
	}
}


/*** FIND WIDGET AT A SPECIFIED ABSOLUTE SCREEN POSITION ***/
static WIDGET *winman_find(long x,long y) {
	WIDGET *win=first_win;
	WIDGET *result;
	while (win!=NULL) {
		if ((result=win->gen->find(win,x,y))) return result;
		win=win->gen->get_next(win);
	}
	return NULL;
}


/*** REMOVE WINDOW FROM THE WINDOW DISPLAY LIST ***/
static void	winman_remove(WINDOW *win) {
	WIDGET *cw=first_win;
	
	if (!win) return;
	if (win->gen->get_parent(win)!=me) return;
	
	/* is win the first win? */
	if ((WIDGET *)win==first_win) {
		first_win=win->gen->get_next(win);
	} else {
	
		/* search in the window list for the window */
		while (cw!=NULL) {
			/* is the next of the current list entry the window? */
			if (cw->gen->get_next(cw)==(WIDGET *)win) {
				/* skip win in list */
				cw->gen->set_next(cw,win->gen->get_next(win));
				break;
			}
			cw=cw->gen->get_next(cw);
		}
	}
	
	/* win is not longer an element of the list, so lets */
	/* destroy its information about its next element */
	win->gen->set_next(win,NULL);
	win->gen->set_parent(win,NULL);
	
	/* redraw area where the window was before we kicked it out... */
	redraw->draw_area(NULL,win->gen->get_x(win),win->gen->get_y(win),
				win->gen->get_x(win)+win->gen->get_w(win)-1,
				win->gen->get_y(win)+win->gen->get_h(win)-1);
}


/*** PULL WINDOW TO THE TOP WHILE RESPECTING 'STAYTOP'-WINDOWS ***/
static void	winman_top(WINDOW *win) {
	WINDOW *cw;
	WINDOW *ltw;			/* last 'staytop' window */

	if (!win) return;
	if (win->gen->get_parent(win)!=me) return;

	/* find last 'stay on top'-window */
	ltw = get_last_staytop_win();
	
	/* search in the window list for the window */
	cw=(WINDOW *)first_win;
	while (cw) {
	
		/* is the next of the current list entry the window? */
		if (cw->gen->get_next(cw)==(WIDGET *)win) break;	
		cw=(WINDOW *)cw->gen->get_next(cw);
	}
	
	if (!cw) return;
	
	/* if the previous window is also a 'stay-top' window -> return */
	if (cw->win->get_staytop(cw)) return;
			
	cw->gen->set_next(cw,win->gen->get_next(win));
			
	if (ltw) {
		win->gen->set_next(win,ltw->gen->get_next(ltw));
		ltw->gen->set_next(ltw,win);
	} else {
		win->gen->set_next(win,first_win);
		first_win=(WIDGET *)win;
	}

	/* redraw window area */
	redraw->draw_area(NULL,win->gen->get_x(win),win->gen->get_y(win),
				win->gen->get_x(win)+win->gen->get_w(win)-1,
				win->gen->get_y(win)+win->gen->get_h(win)-1);
}


/*** PULL ALL 'STAYTOP'-WINDOWS TO THE BEGIN OF THE WINDOW STACK ***/
static void winman_reorder(void) {
	WINDOW *cw=(WINDOW *)first_win, *stw;
	
	/* find first 'normal' window */
	while (cw) {		
		if (!cw->win->get_staytop(cw)) {
				
			/* is there a 'staytop' window after it? */
			stw=(WINDOW *)cw->gen->get_next(cw);
			while (stw) {
				if (stw->win->get_staytop(stw)) winman_top(stw);
				stw = (WINDOW *)stw->gen->get_next(stw);
			}		
		}
		cw = (WINDOW *)cw->gen->get_next(cw);
	}
}


static void	winman_move(WIDGET *cw,long ox1,long oy1,long nx1,long ny1) {
}


static void winman_update_properties(void) {
	redraw = dope->get_module("RedrawManager 1.0");
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct winman_services services = {
	winman_add,
	winman_remove,
	winman_draw,
	winman_draw_behind,
	winman_activate,
	winman_top,
	winman_move,
	winman_find,
	winman_reorder,
	winman_update_properties,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_winman(struct dope_services *d) {
	dope=d;
	
	clip=d->get_module("Clipping 1.0");
	scr=d->get_module("Screen 1.0");
	appman=d->get_module("ApplicationManager 1.0");
	
	d->register_module("WindowManager 1.0",&services);
	return 1;
}
