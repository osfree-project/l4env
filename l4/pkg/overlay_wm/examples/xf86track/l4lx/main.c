/*
 * \brief   Window Event tracker for the X Window System
 * \date    2004-01-19
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This program sniffs events that are related to the window management.
 * Thanks to the unsecureness of the X11, this job is surprisingly easy.
 */

/*
 * Copyright (C) 2004-2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** GENERIC INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** LINUX INCLUDES ***/
#include <signal.h>

/*** X11 INCLUDES ***/
#include <X11/Xlib.h>

/*** OVERLAY WM INCLUDES ***/
#include "ovl_window.h"


struct window;
struct window {
	int id;
	Window xwin;
	Window above;
	int x, y, w, h, border;
	struct window *next;
};

struct window *first_win;

static int config_force_top;


/*
 * Some window managers do not raise a window that is already on top. This is
 * bad because there may be overlay windows that are not known to the X window
 * system but that cover the topmost X window. Thus, we want always to receive
 * a top event. For this, we create a dedicated invisible window that stays
 * always on top of all others. The topmost real X window is always the second.
 * Therefore, the window manager thinks that this window can be topped and
 * generates the desired event.
 */
static Window topwin;
static struct window *tw;


/*** ERROR HANDLER THAT IS CALLED ON XLIB ERRORS ***/
static int x_error_handler(Display *dpy, XErrorEvent *r) {
	printf("Error: x_error_handler called - damn\n");
	return 0;
}


static int window_left_of_screen(Display *dpy, Window xwin) {
	XWindowAttributes attr;

	XGetWindowAttributes(dpy, xwin, &attr);

	if (attr.x + attr.width <= 0) return 1;
	return 0;
}


/*** CREATE NEW WINDOW ***
 *
 * \param bg_flag   Use window as background if set to 1
 * \param position  -1 .. at top,
 *                  -2 .. background,
 *                  otherwise window id of the neighbor behind the new
 *                  window
 */
static struct window *new_window(Window xwin, Display *dpy, int position) {
	struct window *new = malloc(sizeof(struct window));
	XWindowAttributes attr;

	new->xwin = xwin;

	/* add to window list */
	new->next = first_win;
	first_win = new;

	/* request position and size of new window */
	XGetWindowAttributes(dpy, xwin, &attr);
	new->x      = attr.x;
	new->y      = attr.y;
	new->w      = attr.width;
	new->h      = attr.height;
	new->border = attr.border_width;

	/* create, position and open overlay window */
	new->id = ovl_window_create();

	if (position == -2)
		ovl_set_background(new->id);

	ovl_window_place(new->id, new->x, new->y, new->w + new->border*2,
	                                          new->h + new->border*2);
	ovl_window_open(new->id);

	if (position >= 0)
		ovl_window_stack(new->id, position, 1, 1);

	return new;
}


/*** DESTROY WINDOW ***/
static void destroy_window(struct window *win) {
	struct window *curr = first_win;

	if (!win) return;

	if (win == first_win) first_win = win->next;
	else {

		/* search predecessor of window in list */
		while (curr && curr->next != win) curr = curr->next;

		/* skip window in list */
		if (curr) curr->next = win->next;
	}

	/* deallocate window structure */
	ovl_window_destroy(win->id);
	free(win);
}


/*** CALLED WHEN A WINDOW GETS A NEW SIZE OR POSITION ***/
static void place_window(struct window *win, int x, int y, int w, int h) {

	/* keep track new window position */
	win->x = x; win->y = y; win->w = w; win->h = h;
	ovl_window_place(win->id, win->x, win->y, win->w + win->border*2,
	                                          win->h + win->border*2);
}


/*** UTILITY: FIND WINDOW STRUCTURE FOR A GIVEN X WINDOW ID ***/
static struct window *find_window(Window xwin) {
	struct window *w = first_win;

	/* search for window with matchin xwin id */
	for (w = first_win; w; w = w->next)
		if (w->xwin == xwin) return w;

	return NULL;
}


/*** SCAN ALL CURRENTLY PRESENT WINDOW ***/
static void scan_windows(Display *dpy, Window root) {
	Window dummy_w1, dummy_w2, *win_list;
	XWindowAttributes attr;
	unsigned int num_wins;
	int i;

	XQueryTree(dpy, root, &dummy_w1, &dummy_w2, &win_list, &num_wins);
	for (i = 0; i < num_wins; i++) {

		XGetWindowAttributes(dpy, win_list[i], &attr);
		if (attr.map_state == IsViewable)
			new_window(win_list[i], dpy, -1);
	}
	XFree(win_list);

	/* listen at the root window */
	XSelectInput(dpy, root, SubstructureNotifyMask | DestroyNotify);
}


static void dump_window_stack(Display *dpy, Window root) {
	Window dummy_w1, dummy_w2, *win_list;
	unsigned int num_wins;
	int i;

	XQueryTree(dpy, root, &dummy_w1, &dummy_w2, &win_list, &num_wins);
	printf("window stack: ");
	for (i = 0; i < num_wins; i++) {
		printf("%d, ", (int)win_list[i]);
	}
	printf("\n");
}


static struct window *find_window_behind(Display *dpy, Window root, Window win) {
	Window dummy_w1, dummy_w2, *win_list;
	struct window *prev = NULL, *curr;
	unsigned int num_wins;
	int i;

	XQueryTree(dpy, root, &dummy_w1, &dummy_w2, &win_list, &num_wins);
	for (i = 0; i < num_wins; i++) {

		if (win_list[i] == win)
			return prev;

		if ((curr = find_window(win_list[i])))
			prev = curr;
	}
	return NULL;
}


static struct window *find_window_above(Display *dpy, Window root, Window win) {
	Window dummy_w1, dummy_w2, *win_list;
	struct window *curr;
	unsigned int num_wins;
	int i;

	XQueryTree(dpy, root, &dummy_w1, &dummy_w2, &win_list, &num_wins);

	/* find window in X window stack */
	for (i = 0; i < num_wins; i++)
		if (win_list[i] == win)
			break;

	/* skip current window */
	i++;

	/* find and return next overlay window */
	for (; i < num_wins; i++)
		if ((curr = find_window(win_list[i])))
			return curr;

	return NULL;
}


/*** CREATE MAGIC WINDOW THAT STAYS ALWAYS ON TOP ***/
static void create_magic_topwin(Display *dpy, Window root) {
	XWindowChanges wincfg;

	/* create magic window that stays always on top */
	topwin = XCreateWindow(dpy, root,
	                       -200, -200,       /* position */
	                       100, 100,         /* size     */
	                       0,                /* border   */
	                       CopyFromParent,   /* depth    */
	                       InputOutput,      /* class    */
	                       CopyFromParent,   /* visual   */
	                       0,0);

	wincfg.x = -200;
	wincfg.y = -200;
	XConfigureWindow(dpy, topwin,  CWX | CWY , &wincfg);
	XMapWindow(dpy, topwin);
	wincfg.x = -200;
	wincfg.y = -200;
	XConfigureWindow(dpy, topwin,  CWX | CWY , &wincfg);
	tw = new_window(topwin, dpy, root);
}


/*** DESTROY MAGIC TOP WINDOW ***/
static void destroy_magic_topwin(Display *dpy) {
	if (topwin)
		XDestroyWindow(dpy, topwin);
	topwin = 0;
}


/*** BRING MAGIC WINDOW IN FRONT OF ALL OTHERS ***/
static void raise_magic_window(Display *dpy) {
	XRaiseWindow(dpy, topwin);
}


/*** HANDLE X WINDOW SYSTEM EVENTS ***/
static void eventloop(Display *dpy, Window root) {
	XEvent ev;
	while (1) {
		struct window *win;
		XNextEvent(dpy, &ev);

		switch (ev.type) {

		case ConfigureNotify:
			if ((win = find_window(ev.xconfigure.window))) {

				int x = ev.xconfigure.x;
				int y = ev.xconfigure.y;
				int w = ev.xconfigure.width;
				int h = ev.xconfigure.height;

				/*
				 * If window position and size keeps the same,
				 * we assume, the window has been topped.
				 */
				if (x == win->x && y == win->y && w == win->w && h == win->h) {

					struct window *behind = find_window_above(dpy, root, ev.xconfigure.window);

					ovl_window_stack(win->id, behind ? behind->id : -1, 1, 1);

					if (!window_left_of_screen(dpy, ev.xconfigure.window) && config_force_top)
						raise_magic_window(dpy);

				} else
					place_window(win, x, y, w, h);
			}
			break;

		case Expose:
			if ((win = find_window(ev.xconfigure.window)))

				/* top overlay window */
				ovl_window_stack(win->id, -1, 1, 1);

			break;

		case UnmapNotify:
			if ((win = find_window(ev.xconfigure.window)))
				destroy_window(win);
			break;

		case MapNotify:
			if ((win = find_window(ev.xconfigure.window)))
				printf("MapRequest: window already present - Open Window %d\n", win->id);
			else {

				struct window *behind = find_window_above(dpy, root, ev.xconfigure.window);
				/*
				 * Idea: Call XQueryTree to obtain the position where the new
				 *       window is located in the window stack.
				 *
				 */

				new_window(ev.xconfigure.window, dpy, behind ? behind->id : -1);

				if (!window_left_of_screen(dpy, ev.xconfigure.window) && config_force_top)
					raise_magic_window(dpy);
			}
			break;
		}
	}
}


/*** SIGNAL HANDLER FOR GRACEFUL EXIT ON CTRL-C ***/
static void signal_handler(int signal) {
	switch (signal) {
	case SIGINT:
	case SIGTERM:
	case SIGHUP:
		while (first_win) destroy_window(first_win);
		exit(0);
		break;
	}
}


/*** MAIN PROGRAM ***/
int main(int argc, char **argv) {
	struct sigaction action;
	Display *dpy;
	Window root;
	int screen;
	int i;

	ovl_window_init(NULL);

	/* install signal handler for quitting nicely on ctrl-c */
	action.sa_handler = signal_handler;
	action.sa_flags   = 0;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT,  &action, NULL);
	sigaction(SIGHUP,  &action, NULL);

	/* init X stuffs */
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		printf("Error - cannot open display\n");
		return 1;
	}

	XSetErrorHandler(x_error_handler);

	screen = DefaultScreen(dpy);
	root   = RootWindow(dpy, screen);

	/* check command line arguments */
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--force-top")) config_force_top = 1;
	}

	if (config_force_top)
		create_magic_topwin(dpy, root);

	/* create background overlay window */
	new_window(root, dpy, -2);

	/* retrieve information about currently present windows */
	scan_windows(dpy, root);

	/* start sniffing */
	eventloop(dpy, root);
	return 0;
}
