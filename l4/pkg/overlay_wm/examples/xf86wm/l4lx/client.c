/* aewm - a minimalist X11 window mananager. vim:sw=4:ts=4:et
 * Copyright 1998-2002 Decklin Foster <decklin@red-bean.com>
 * This program is free software; see LICENSE for details. */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** X11 INCLUDES ***/
#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** OVERLAY LIB INCLUDES ***/
#include "ovl_window.h"

/*** LOCAL INCLUDES ***/
#include "config.h"
#include "client.h"
#include "xmisc.h"
#include "init.h"

//static char dopecmd[256];

struct client *find_client(Window w, int mode)
{
    struct client *c;

    if (mode == FRAME) {
        for (c = head_client; c; c = c->next)
            if (c->frame == w) return c;
    } else { /* WINDOW */
        for (c = head_client; c; c = c->next)
            if (c->window == w) return c;
    }

    return NULL;
}

/* For a regular window, c->trans is None (false), and we include
 * enough space to draw the title. For a transient window we just make
 * a tiny strip. */

int theight(struct client *c)
{
    if (!c) return 0;

#ifdef MWM_HINTS
    if (!c->has_title) return 0;
#endif
#ifdef XFT
    return (c->trans ? 0 : xftfont->ascent + xftfont->descent) +
        2*opt_pad + BW(c);
#else
    return (c->trans ? 0 : font->ascent + font->descent) +
        2*opt_pad + BW(c);
#endif
}

/* Attempt to follow the ICCCM by explicity specifying 32 bits for
 * this property. Does this goof up on 64 bit systems? */

void set_wm_state(struct client *c, int state)
{
    CARD32 data[2];

    data[0] = state;
    data[1] = None; /* Icon? We don't need no steenking icon. */

    XChangeProperty(dpy, c->window, wm_state, wm_state,
        32, PropModeReplace, (unsigned char *)data, 2);
}

/* If we can't find a WM_STATE we're going to have to assume
 * Withdrawn. This is not exactly optimal, since we can't really
 * distinguish between the case where no WM has run yet and when the
 * state was explicitly removed (struct clients are allowed to either set the
 * atom to Withdrawn or just remove it... yuck.) */

long get_wm_state(struct client *c)
{
    Atom real_type; int real_format;
    unsigned long items_read, bytes_left;
    long *data, state = WithdrawnState;

    if (XGetWindowProperty(dpy, c->window, wm_state, 0L, 2L, False,
            wm_state, &real_type, &real_format, &items_read, &bytes_left,
            (unsigned char **) &data) == Success && items_read) {
        state = *data;
        XFree(data);
    }
    return state;
}

/* This will need to be called whenever we update our struct client stuff.
 * Yeah, yeah, stop yelling at me about OO. */

void send_config(struct client *c)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.event = c->window;
    ce.window = c->window;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->width;
    ce.height = c->height;
    ce.border_width = 0;
    ce.above = None;
    ce.override_redirect = 0;

    XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);

	ovl_window_place(c->ovlwin_id, c->x, c->y, c->width, c->height);
//    sprintf(dopecmd,"xw%d.set(-x %d -y %d -w %d -h %d)",c->dopewin_id,c->x,c->y,c->width,c->height);
//    dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//    dope_print(dopecmd);
}


/* After pulling my hair out trying to find some way to tell if a
 * window is still valid, I've decided to instead carefully ignore any
 * errors raised by this function. We know that the X calls are, and
 * we know the only reason why they could fail -- a window has removed
 * itself completely before the Unmap and Destroy events get through
 * the queue to us. It's not absolutely perfect, but it works.
 *
 * The 'withdrawing' argument specifes if the client is actually
 * (destroying itself||being destroyed by us) or if we are merely
 * cleaning up its data structures when we exit mid-session. */

static int ignore_xerror(Display *dpy, XErrorEvent *e)
{
    return 0;
}

void remove_client(struct client *c, int mode)
{
    struct client *p;

    XGrabServer(dpy);
    XSetErrorHandler(ignore_xerror);

#ifdef DEBUG
    dump_title(c, "removing", 'r');
    dump_removal(c, mode);
#endif

    if (mode == WITHDRAW) set_wm_state(c, WithdrawnState);
    else /* REMAP */ XMapWindow(dpy, c->window);

    gravitate(c, REMOVE_GRAVITY);
    XReparentWindow(dpy, c->window, root, c->x, c->y);
#ifdef MWM_HINTS
    if (c->has_border) XSetWindowBorderWidth(dpy, c->window, 1);
#else
    XSetWindowBorderWidth(dpy, c->window, 1);
#endif
#ifdef XFT
    if (c->xftdraw) XftDrawDestroy(c->xftdraw);
#endif
    XRemoveFromSaveSet(dpy, c->window);
    XDestroyWindow(dpy, c->frame);

	ovl_window_close(c->ovlwin_id);
//    sprintf(dopecmd,"xw%d.close()",c->dopewin_id);
//    dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);

    if (head_client == c) head_client = c->next;
    else for (p = head_client; p && p->next; p = p->next)
        if (p->next == c) p->next = c->next;

    if (c->name) XFree(c->name);
    if (c->size) XFree(c->size);
    free(c);

    XSync(dpy, False);
    XSetErrorHandler(handle_xerror);
    XUngrabServer(dpy);
}

/* I've changed this to just clear the window every time. The amount
 * of 'flicker' is basically imperceptable. Also, we might be drawing
 * an anti-aliased font with Xft, in which case we always have to
 * clear to draw the text properly. This allows us to simplify
 * handle_property_change as well. */

void redraw(struct client *c)
{
#ifdef MWM_HINTS
	if (!c->has_title) return;
#endif

	XClearWindow(dpy, c->frame);
	XDrawLine(dpy, c->frame, border_gc,
		0, theight(c) - BW(c) + BW(c)/2,
		c->width, theight(c) - BW(c) + BW(c)/2);
	XDrawLine(dpy, c->frame, border_gc,
		c->width - theight(c)+ BW(c)/2, 0,
		c->width - theight(c)+ BW(c)/2, theight(c));
	if (!c->trans && c->name) {
#ifdef XFT
		XftDrawString8(c->xftdraw, &xft_fg,
			xftfont, opt_pad, opt_pad + xftfont->ascent,
			c->name, strlen(c->name));
#else
		XDrawString(dpy, c->frame, string_gc,
			opt_pad, opt_pad + font->ascent,
			c->name, strlen(c->name));
#endif
	}
}

/* Window gravity is a mess to explain, but we don't need to do much
 * about it since we're using X borders. For NorthWest et al, the top
 * left corner of the window when there is no WM needs to match up
 * with the top left of our fram once we manage it, and likewise with
 * SouthWest and the bottom right (these are the only values I ever
 * use, but the others should be obvious.) Our titlebar is on the top
 * so we only have to adjust in the first case. */

void gravitate(struct client *c, int multiplier)
{
    int dy = 0;
    int gravity = (c->size->flags & PWinGravity) ?
        c->size->win_gravity : NorthWestGravity;

    switch (gravity) {
        case NorthWestGravity:
        case NorthEastGravity:
        case NorthGravity: dy = theight(c); break;
        case CenterGravity: dy = theight(c)/2; break;
    }

    c->y += multiplier * dy;
}

/* Well, the man pages for the shape extension say nothing, but I was
 * able to find a shape.PS.Z on the x.org FTP site. What we want to do
 * here is make the window shape be a boolean OR (or union, if you
 * prefer) of the client's shape and our titlebar. The titlebar
 * requires both a bound and a clip because it has a border -- the X
 * server will paint the border in the region between the two. (I knew
 * that using X borders would get me eventually... ;-)) */

#ifdef SHAPE
void set_shape(struct client *c)
{
    int n, order;
    XRectangle temp, *dummy;

    dummy = XShapeGetRectangles(dpy, c->window, ShapeBounding, &n, &order);
    if (n > 1) {
        XShapeCombineShape(dpy, c->frame, ShapeBounding,
            0, theight(c), c->window, ShapeBounding, ShapeSet);
        temp.x = -BW(c);
        temp.y = -BW(c);
        temp.width = c->width + 2*BW(c);
        temp.height = theight(c) + BW(c);
        XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
            0, 0, &temp, 1, ShapeUnion, YXBanded);
        temp.x = 0;
        temp.y = 0;
        temp.width = c->width;
        temp.height = theight(c) - BW(c);
        XShapeCombineRectangles(dpy, c->frame, ShapeClip,
            0, theight(c), &temp, 1, ShapeUnion, YXBanded);
        c->has_been_shaped = 1;
    } else if (c->has_been_shaped) {
        /* I can't find a 'remove all shaping' function... */
        temp.x = -BW(c);
        temp.y = -BW(c);
        temp.width = c->width + 2*BW(c);
        temp.height = c->height + theight(c) + 2*BW(c);
        XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
            0, 0, &temp, 1, ShapeSet, YXBanded);
    }
    XFree(dummy);
}
#endif
