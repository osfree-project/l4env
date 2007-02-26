/* aewm - a minimalist X11 window mananager. vim:sw=4:ts=4:et
 * Copyright 1998-2002 Decklin Foster <decklin@red-bean.com>
 * This program is free software; see LICENSE for details. */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** X11 INCLUDES ***/
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** OVERLAY LIB INCLUDES ***/
#include "ovl_window.h"

/*** LOCAL INCLUDES ***/
#include "config.h"
#include "init.h"
#include "client.h"
#include "xmisc.h"
#include "manage.h"

/* Shorthand for wordy function calls */

#define setmouse(w, x, y) XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y)
#define ungrab() XUngrabPointer(dpy, CurrentTime)
#define grab(w, mask, curs) (XGrabPointer(dpy, w, False, mask, \
    GrabModeAsync, GrabModeAsync, None, curs, CurrentTime) == GrabSuccess)

/* Modes to call get_incsize with */

#define PIXELS 0
#define INCREMENTS 1

//static char dopecmd[256];
static void drag(struct client *);
static void sweep(struct client *);
static void recalc_sweep(struct client *, int, int, int, int);
static void draw_outline(struct client *);
static int get_incsize(struct client *, int *, int *, int);

static int send_xmessage(Window w, Atom a, long x)
{
    XEvent e;

    e.type = ClientMessage;
    e.xclient.window = w;
    e.xclient.message_type = a;
    e.xclient.format = 32;
    e.xclient.data.l[0] = x;
    e.xclient.data.l[1] = CurrentTime;

    return XSendEvent(dpy, w, False, NoEventMask, &e);
}

void move(struct client *c)
{
    drag(c);
    XMoveWindow(dpy, c->frame, c->x, c->y - theight(c));
    send_config(c);
}

void resize(struct client *c)
{
    sweep(c);
    XMoveResizeWindow(dpy, c->frame,
        c->x, c->y - theight(c), c->width, c->height + theight(c));
    XMoveResizeWindow(dpy, c->window,
        0, theight(c), c->width, c->height);
    send_config(c);
}

void hide(struct client *c)
{
    if (!c->ignore_unmap) c->ignore_unmap++;
    XUnmapWindow(dpy, c->frame);
    XUnmapWindow(dpy, c->window);
    set_wm_state(c, IconicState);
//    sprintf(dopecmd,"xw%d.close()",c->dopewin_id);
//    dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
}


void lower_win(struct client *c) {
//    XLowerWindow(dpy, (c)->frame);
}

void raise_win(struct client *c) {
    XRaiseWindow(dpy, (c)->frame);
	ovl_window_top(c->ovlwin_id);
//    sprintf(dopecmd,"xw%d.top()",c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
}

/* The name of this function is a bit misleading: if the client
 * doesn't listen to WM_DELETE then we just terminate it with extreme
 * prejudice. */

void send_wm_delete(struct client *c)
{
    int i, n, found = 0;
    Atom *protocols;

    if (XGetWMProtocols(dpy, c->window, &protocols, &n)) {
        for (i=0; i<n; i++) if (protocols[i] == wm_delete) found++;
        XFree(protocols);
    }
    if (found) send_xmessage(c->window, wm_protos, wm_delete);
    else XKillClient(dpy, c->window);

//    sprintf(dopecmd,"xw%d.close()",c->dopewin_id);
//    dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
}

#define MouseMask (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)

static void drag(struct client *c)
{
    XEvent ev;
    int x1, y1;
    int old_cx = c->x;
    int old_cy = c->y;

    if (!grab(root, MouseMask, move_curs)) return;
    XGrabServer(dpy);
    get_mouse_position(&x1, &y1);

    draw_outline(c);
    for (;;) {
        XMaskEvent(dpy, MouseMask, &ev);
        switch (ev.type) {
            case MotionNotify:
                draw_outline(c); /* clear */
                c->x = old_cx + (ev.xmotion.x - x1);
                c->y = old_cy + (ev.xmotion.y - y1);
                draw_outline(c);
                break;
            case ButtonRelease:
                draw_outline(c); /* clear */
                XUngrabServer(dpy);
                ungrab();
                return;
        }
    }
}

static void sweep(struct client *c)
{
    XEvent ev;
    int old_cx = c->x;
    int old_cy = c->y;

    if (!grab(root, MouseMask, resize_curs)) return;
    XGrabServer(dpy);

    draw_outline(c);
    setmouse(c->window, c->width, c->height);
    for (;;) {
        XMaskEvent(dpy, MouseMask, &ev);
        switch (ev.type) {
            case MotionNotify:
                draw_outline(c); /* clear */
                recalc_sweep(c, old_cx, old_cy, ev.xmotion.x, ev.xmotion.y);
                draw_outline(c);
                break;
            case ButtonRelease:
                draw_outline(c); /* clear */
                XUngrabServer(dpy);
                ungrab();
                return;
        }
    }
}

static void recalc_sweep(struct client *c, int x1, int y1, int x2, int y2)
{
    c->width = abs(x1 - x2) - BW(c);
    c->height = abs(y1 - y2) - BW(c);

    get_incsize(c, &c->width, &c->height, PIXELS);

    if (c->size->flags & PMinSize) {
        if (c->width < c->size->min_width) c->width = c->size->min_width;
        if (c->height < c->size->min_height) c->height = c->size->min_height;
    }

    if (c->size->flags & PMaxSize) {
        if (c->width > c->size->max_width) c->width = c->size->max_width;
        if (c->height > c->size->max_height) c->height = c->size->max_height;
    }

    c->x = (x1 <= x2) ? x1 : x1 - c->width;
    c->y = (y1 <= y2) ? y1 : y1 - c->height;
}

static void draw_outline(struct client *c)
{
    char buf[32];
    int width, height;

    XDrawRectangle(dpy, root, invert_gc,
        c->x + BW(c)/2, c->y - theight(c) + BW(c)/2,
        c->width + BW(c), c->height + theight(c) + BW(c));
    XDrawLine(dpy, root, invert_gc, c->x + BW(c), c->y + BW(c)/2,
        c->x + c->width + BW(c), c->y + BW(c)/2);

    if (!get_incsize(c, &width, &height, INCREMENTS)) {
        width = c->width;
        height = c->height;
    }

    gravitate(c, REMOVE_GRAVITY);
    snprintf(buf, sizeof buf, "%dx%d+%d+%d", width, height, c->x, c->y);
    gravitate(c, APPLY_GRAVITY);
    XDrawString(dpy, root, invert_gc,
        c->x + c->width - XTextWidth(font, buf, strlen(buf)) - opt_pad,
        c->y + c->height - opt_pad,
        buf, strlen(buf));
}

/* If the window in question has a ResizeInc hint, then it wants to be
 * resized in multiples of some (x,y). If we are calculating a new
 * window size, we set mode == PIXELS and get the correct width and
 * height back. If we are drawing a friendly label on the screen for
 * the user, we set mode == INCREMENTS so that they see the geometry
 * in human-readable form (80x25 for xterm, etc). */

static int get_incsize(struct client *c, int *x_ret, int *y_ret, int mode)
{
    int width_inc, height_inc;
    int base_width, base_height;

    if (c->size->flags & PResizeInc) {
        width_inc = c->size->width_inc ? c->size->width_inc : 1;
        height_inc = c->size->height_inc ? c->size->height_inc : 1;
        base_width = (c->size->flags & PBaseSize) ? c->size->base_width :
            (c->size->flags & PMinSize) ? c->size->min_width : 0;
        base_height = (c->size->flags & PBaseSize) ? c->size->base_height :
            (c->size->flags & PMinSize) ? c->size->min_height : 0;

        if (mode == PIXELS) {
            *x_ret = c->width - ((c->width - base_width) % width_inc);
            *y_ret = c->height - ((c->height - base_height) % height_inc);
        } else /* INCREMENTS */ {
            *x_ret = (c->width - base_width) / width_inc;
            *y_ret = (c->height - base_height) / height_inc;
        }
        return 1;
    }

    return 0;
}
