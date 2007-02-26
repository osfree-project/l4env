/* aewm - a minimalist X11 window mananager. vim:sw=4:ts=4:et
 * Copyright 1998-2002 Decklin Foster <decklin@red-bean.com>
 * This program is free software; see LICENSE for details. */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

/*** X11 INCLUDES ***/
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** OVERLAY LIB INLCUES ***/
#include "ovl_window.h"

/*** LOCAL INCLUDES ***/
#include "config.h"
#include "init.h"
#include "client.h"
#include "manage.h"
#include "new.h"
#include "events.h"

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>

static void handle_button_event(XButtonEvent *);
static void handle_configure_request(XConfigureRequestEvent *);
static void handle_map_request(XMapRequestEvent *);
static void handle_unmap_event(XUnmapEvent *);
static void handle_destroy_event(XDestroyWindowEvent *);
static void handle_client_message(XClientMessageEvent *);
static void handle_property_change(XPropertyEvent *);
static void handle_enter_event(XCrossingEvent *);
static void handle_colormap_change(XColormapEvent *);
static void handle_expose_event(XExposeEvent *);

/* We may want to put in some sort of check for unknown events at some
 * point. TWM has an interesting and different way of doing this... */

static pthread_mutex_t xcall_mutex;
static pthread_t x_events_th;


#define EVENT_X_BUTTON		1
#define EVENT_X_CONFIGURE	2
#define EVENT_X_MAP			3
#define EVENT_X_UNMAP		4
#define EVENT_X_DESTROYWIN	5
#define EVENT_X_CLIENTMSG	6
#define EVENT_X_COLORMAP	7
#define EVENT_X_PROPERTY	8
#define EVENT_X_ENTERNOTIFY 9
#define EVENT_X_EXPOSE		10

static XEvent xev;

static int ovl_event = 0;
static int ovl_x, ovl_y, ovl_w, ovl_h;
static struct client *ovl_c;

#define OVL_NO_EVENT 0
#define OVL_EVENT_TOP 1
#define OVL_EVENT_PLACE 2

static void wintop_callback(int ovlwin_id) {
	struct client *c = ovl_window_get_private(ovlwin_id);
	if (!c || ovl_event) return;
	ovl_c = c;
	ovl_event = OVL_EVENT_TOP;
}


static void winpos_callback(int ovlwin_id, int x, int y, int w, int h) {
	struct client *c = ovl_window_get_private(ovlwin_id);
	if (!c || ovl_event) return;
	ovl_x = x;
	ovl_y = y - 21;
	ovl_w = w;
	ovl_h = h + 21;
	ovl_c = c;
	ovl_event = OVL_EVENT_PLACE;
}


void do_event_loop(void)
{
	l4_threadid_t mytid = l4_myself();
	printf("eventloop started (%x.%x)\n",(int)mytid.id.task, (int)mytid.id.lthread);

	ovl_wintop_callback(wintop_callback);
	ovl_winpos_callback(winpos_callback);

//	pthread_mutex_init(&xcall_mutex,NULL);

	for (;;) {
//		pthread_mutex_lock(&xcall_mutex);
		XFlush(dpy);
		while (XPending(dpy)) {
			XNextEvent(dpy, &xev);
			switch (xev.type) {
				case ButtonPress:
					handle_button_event(&xev.xbutton); break;
				case ConfigureRequest:
					handle_configure_request(&xev.xconfigurerequest); break;
				case MapRequest:
					handle_map_request(&xev.xmaprequest); break;
				case UnmapNotify:
					handle_unmap_event(&xev.xunmap); break;
				case DestroyNotify:
					handle_destroy_event(&xev.xdestroywindow); break;
				case ClientMessage:
					handle_client_message(&xev.xclient); break;
				case ColormapNotify:
					handle_colormap_change(&xev.xcolormap); break;
				case PropertyNotify:
					handle_property_change(&xev.xproperty); break;
				case EnterNotify:
					handle_enter_event(&xev.xcrossing); break;
				case Expose:
					handle_expose_event(&xev.xexpose); break;
			}
		}
//		pthread_mutex_unlock(&xcall_mutex);
		switch (ovl_event) {
			case OVL_EVENT_TOP:
				XRaiseWindow(dpy, ovl_c->frame);
				break;
			case OVL_EVENT_PLACE:
				XMoveResizeWindow(dpy, ovl_c->frame, ovl_x, ovl_y, ovl_w, ovl_h);
				ovl_c->x = ovl_x;
				ovl_c->y = ovl_y;
				ovl_c->width = ovl_w;
				ovl_c->height = ovl_h;
				break;
		}
		ovl_event = OVL_NO_EVENT;
		usleep(1000*10);
	}
}


/* Someone clicked a button. If it was on the root, we get the click
 * be default. If it's on a window frame, we get it as well. If it's
 * on a client window, it may still fall through to us if the client
 * doesn't select for mouse-click events. The upshot of this is that
 * you should be able to click on the blank part of a GTK window with
 * Button2 to move it.
 *
 * If you have a hankering to change the button bindings, they're
 * right here. However(!), I highly reccomend that 2-button mouse
 * users try xmodmap -e 'pointer = 1 3 2' first, as mentioned in the
 * README. */

static void fork_exec(char *cmd)
{
    pid_t pid = fork();

    switch (pid) {
        case 0:
            setsid();
            execlp("/bin/sh", "sh", "-c", cmd, NULL);
            printf("exec failed, cleaning up child");
            exit(1);
        case -1:
            printf("can't fork");
    }
}

static void handle_button_event(XButtonEvent *e)
{
    struct client *c = find_client(e->window, FRAME);
    int in_box;

    if (e->window == root) {
#ifdef DEBUG
        dump_clients();
#endif
        switch (e->button) {
            case Button1: fork_exec(opt_new1); break;
            case Button2: fork_exec(opt_new2); break;
            case Button3: fork_exec(opt_new3); break;
        }
    } else if (c) {
        in_box = (e->x >= c->width - theight(c)) && (e->y <= theight(c));
        switch (e->button) {
            case Button1:
                in_box ? hide(c) : raise_win(c); break;
            case Button2:
                in_box ? resize(c) : move(c); break;
            case Button3:
                in_box ? send_wm_delete(c) : lower_win(c); break;
        }
    }
}

/* Because we are redirecting the root window, we get ConfigureRequest
 * events from both clients we're handling and ones that we aren't.
 * For clients we manage, we need to fiddle with the frame and the
 * client window, and for unmanaged windows we have to pass along
 * everything unchanged. Thankfully, we can reuse (a) the
 * XWindowChanges struct and (c) the code to configure the client
 * window in both cases.
 *
 * Most of the assignments here are going to be garbage, but only the
 * ones that are masked in by e->value_mask will be looked at by the X
 * server. */

static void handle_configure_request(XConfigureRequestEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);
    XWindowChanges wc;

    if (c) {
        gravitate(c, REMOVE_GRAVITY);
        if (e->value_mask & CWX) c->x = e->x;
        if (e->value_mask & CWY) c->y = e->y;
        if (e->value_mask & CWWidth) c->width = e->width;
        if (e->value_mask & CWHeight) c->height = e->height;
        gravitate(c, APPLY_GRAVITY);
#ifdef DEBUG
        dump_geom(c, "moving to");
#endif
        /* configure the frame */
        wc.x = c->x;
        wc.y = c->y - theight(c);
        wc.width = c->width;
        wc.height = c->height + theight(c);
        wc.border_width = BW(c);
        wc.sibling = e->above;
        wc.stack_mode = e->detail;
        XConfigureWindow(dpy, c->frame, e->value_mask, &wc);
#ifdef SHAPE
        if (e->value_mask & (CWWidth|CWHeight)) set_shape(c);
#endif
        send_config(c);

        /* start setting up the next call */
        wc.x = 0;
        wc.y = theight(c);
    } else {
        wc.x = e->x;
        wc.y = e->y;
    }

    wc.width = e->width;
    wc.height = e->height;
    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    XConfigureWindow(dpy, e->window, e->value_mask, &wc);


}

/* Two possiblilies if a client is asking to be mapped. One is that
 * it's a new window, so we handle that if it isn't in our clients
 * list anywhere. The other is that it already exists and wants to
 * de-iconify, which is simple to take care of. */

static void handle_map_request(XMapRequestEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (!c) {
        make_new_client(e->window);
    } else {
        XMapWindow(dpy, c->window);
        XMapRaised(dpy, c->frame);
        set_wm_state(c, NormalState);
    }
}

/* See aewm.h for the intro to this one. If this is a window we
 * unmapped ourselves, decrement c->ignore_unmap and casually go on as
 * if nothing had happened. If the window unmapped itself from under
 * our feet, however, get rid of it.
 *
 * If you spend a lot of time with -DDEBUG on, you'll realize that
 * because most clients unmap and destroy themselves at once, they're
 * gone before we even get the Unmap event, never mind the Destroy
 * one. This will necessitate some extra caution in remove_client.
 *
 * Personally, I think that if Map events are intercepted, Unmap
 * events should be intercepted too. No use arguing with a standard
 * that's almost as old as I am though. :-( */

static void handle_unmap_event(XUnmapEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (!c) return;

    if (c->ignore_unmap) c->ignore_unmap--;
    else remove_client(c, WITHDRAW);
}

/* This happens when a window is iconified and destroys itself. An
 * Unmap event wouldn't happen in that case because the window is
 * already unmapped. */

static void handle_destroy_event(XDestroyWindowEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (!c) return;
    remove_client(c, WITHDRAW);
}

/* If a client wants to iconify itself (boo! hiss!) it must send a
 * special kind of ClientMessage. We might set up other handlers here
 * but there's nothing else required by the ICCCM. */

static void handle_client_message(XClientMessageEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (c && e->message_type == wm_change_state &&
        e->format == 32 && e->data.l[0] == IconicState) hide(c);
}

/* All that we have cached is the name and the size hints, so we only
 * have to check for those here. A change in the name means we have to
 * immediately wipe out the old name and redraw size hints only get
 * used when we need them. Note that the actual redraw call both
 * clears and draws; before Xft, the XClearWindow call was only made
 * in this function. */

static void handle_property_change(XPropertyEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);
    long dummy;

    if (!c) return;

    switch (e->atom) {
        case XA_WM_NAME:
            if (c->name) XFree(c->name);
            XFetchName(dpy, c->window, &c->name);
            redraw(c);
            break;
        case XA_WM_NORMAL_HINTS:
            XGetWMNormalHints(dpy, c->window, c->size, &dummy);
    }
}

/* X's default focus policy is follows-mouse, but we have to set it
 * anyway because some sloppily written clients assume that (a) they
 * can set the focus whenever they want or (b) that they don't have
 * the focus unless the keyboard is grabbed to them. OTOH it does
 * allow us to keep the previous focus when pointing at the root,
 * which is nice.
 *
 * We also implement a colormap-follows-mouse policy here. That, on
 * the third hand, is *not* X's default. */

static void handle_enter_event(XCrossingEvent *e)
{
    struct client *c = find_client(e->window, FRAME);

    if (!c) return;
    XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
    XInstallColormap(dpy, c->cmap);
}

/* Here's part 2 of our colormap policy: when a client installs a new
 * colormap on itself, set the display's colormap to that. Arguably,
 * this is bad, because we should only set the colormap if that client
 * has the focus. However, clients don't usually set colormaps at
 * random when you're not interacting with them, so I think we're
 * safe. If you have an 8-bit display and this doesn't work for you,
 * by all means yell at me, but very few people have 8-bit displays
 * these days. */

static void handle_colormap_change(XColormapEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (c && e->new) {
        c->cmap = e->colormap;
        XInstallColormap(dpy, c->cmap);
    }
}

/* If we were covered by multiple windows, we will usually get
 * multiple expose events, so ignore them unless e->count (the number
 * of outstanding exposes) is zero. */

static void handle_expose_event(XExposeEvent *e)
{
    struct client *c = find_client(e->window, FRAME);

    if (c && e->count == 0) redraw(c);
}

#ifdef SHAPE
static void handle_shape_change(XShapeEvent *e)
{
    struct client *c = find_client(e->window, WINDOW);

    if (c) set_shape(c);
}
#endif
