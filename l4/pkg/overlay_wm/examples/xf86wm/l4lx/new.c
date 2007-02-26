
/*** GENERAL INCLUDES ***/
//#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

/*** X11 INCLUDES ***/
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** OVERLAY LIB INCLUDES ***/
#include "ovl_window.h"

/*** LOCAL INCLUDES ***/
#include "config.h"
#include "client.h"
#include "init.h"
#include "xmisc.h"
#include "new.h"

//static char dopecmd[256];
//static long max_dopewin_id = 0;
static void init_position(struct client *);
static void reparent(struct client *);
#ifdef MWM_HINTS
static PropMwmHints *get_mwm_hints(Window);
#endif

/* Set up a client structure for the new (not-yet-mapped) window. The
 * confusing bit is that we have to ignore 2 unmap events if the
 * client was already mapped but has IconicState set (for instance,
 * when we are the second window manager in a session).  That's
 * because there's one for the reparent (which happens on all viewable
 * windows) and then another for the unmapping itself. */

void make_new_client(Window w)
{
    struct client *c;
    XWindowAttributes attr;
    XWMHints *hints;
#ifdef MWM_HINTS
    PropMwmHints *mhints;
#endif
    long dummy;

    c = malloc(sizeof *c);
    c->next = head_client;
    head_client = c;

    XGrabServer(dpy);

    XGetTransientForHint(dpy, w, &c->trans);
    XFetchName(dpy, w, &c->name);
    XGetWindowAttributes(dpy, w, &attr);

//    c->dopewin_id = ++max_dopewin_id;
    c->window = w;
    c->ignore_unmap = 0;
    c->x = attr.x;
    c->y = attr.y;
    c->width = attr.width;
    c->height = attr.height;
    c->cmap = attr.colormap;
    c->size = XAllocSizeHints();
    XGetWMNormalHints(dpy, c->window, c->size, &dummy);
	
    c->ovlwin_id = ovl_window_create();
	ovl_window_set_private(c->ovlwin_id, c);
	ovl_window_place(c->ovlwin_id, c->x, c->y, c->width, c->height);
	ovl_window_open(c->ovlwin_id);

    /* create a corresponding DOpE window */
//    sprintf(dopecmd,"xw%d = new Window()",c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//
//    sprintf(dopecmd,"xw%d.set(-x %d -y %d -w %d -h %d)",c->dopewin_id,c->x,c->y,c->width,c->height);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//
//    sprintf(dopecmd,"xw%d.open()",c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//
//    sprintf(dopecmd,"xw%d.bind(\"top\",\"t win_id=%d\")",c->dopewin_id,c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//
//    sprintf(dopecmd,"xw%d.bind(\"moved\",\"m win_id=%d\")",c->dopewin_id,c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);
//
//    sprintf(dopecmd,"xw%d.bind(\"resized\",\"r win_id=%d\")",c->dopewin_id,c->dopewin_id);
//	dope_manager_exec_cmd(dope_l4id,app_id,dopecmd,&dope_retp,&_ev);

#ifdef SHAPE
    c->has_been_shaped = 0;
#endif

#ifdef XFT
    c->xftdraw = NULL;
#endif

#ifdef MWM_HINTS
    c->has_title  = 1;
    c->has_border = 1;

    if ((mhints = get_mwm_hints(c->window))) {
        if (mhints->flags & MWM_HINTS_DECORATIONS
            && !(mhints->decorations & MWM_DECOR_ALL)) {
            c->has_title  = mhints->decorations & MWM_DECOR_TITLE;
            c->has_border = mhints->decorations & MWM_DECOR_BORDER;
        }
        XFree(mhints);
    }
#endif

#ifdef DEBUG
    dump_title(c, "creating", 'w');
    dump_geom(c, "initial");
#endif

    if (attr.map_state == IsViewable) {
        c->ignore_unmap++;
    } else {
        init_position(c);
        set_wm_state(c, NormalState);
        if ((hints = XGetWMHints(dpy, w))) {
            if (hints->flags & StateHint) set_wm_state(c, hints->initial_state);
            XFree(hints);
        }
    }

#ifdef DEBUG
    dump_geom(c, "set to");
    dump_info(c);
#endif

    gravitate(c, APPLY_GRAVITY);
    reparent(c);

#ifdef XFT
    c->xftdraw = XftDrawCreate(dpy, (Drawable) c->frame,
        DefaultVisual(dpy, DefaultScreen(dpy)),
        DefaultColormap(dpy, DefaultScreen(dpy)));
#endif

    if (attr.map_state == IsViewable) {
        if (get_wm_state(c) == IconicState) {
            c->ignore_unmap++;
            XUnmapWindow(dpy, c->window);
        } else {
            XMapWindow(dpy, c->window);
            XMapRaised(dpy, c->frame);
            set_wm_state(c, NormalState);
        }
    } else {
        if (get_wm_state(c) == NormalState) {
            XMapWindow(dpy, c->window);
            XMapRaised(dpy, c->frame);
        }
    }

    XSync(dpy, False);
    XUngrabServer(dpy);
}

/* This one does -not- free the data coming back from Xlib; it just
 * sends back the pointer to what was allocated. */

#ifdef MWM_HINTS
static PropMwmHints *get_mwm_hints(Window w)
{
    Atom real_type; int real_format;
    unsigned long items_read, bytes_left;
    PropMwmHints *data;

    if (XGetWindowProperty(dpy, w, mwm_hints, 0L, 20L, False,
            mwm_hints, &real_type, &real_format, &items_read, &bytes_left,
            (unsigned char **) &data) == Success
        && items_read >= PROP_MOTIF_WM_HINTS_ELEMENTS) {
        return data;
    } else {
        return NULL;
    }
}
#endif

/* Figure out where to map the window. c->x, c->y, c->width, and
 * c->height actually start out with values in them (whatever the
 * client passed to XCreateWindow).  Program-specified hints will
 * override these, but anything set by the program will be
 * sanity-checked before it is used. PSize is ignored completely,
 * because GTK sets it to 200x200 for almost everything. User-
 * specified hints will of course override anything the program says.
 *
 * If we can't find a reasonable position hint, we make up a position
 * using the mouse co-ordinates and window size. If the mouse is in
 * the center, we center the window; if it's at an edge, the window
 * goes on the edge. To account for window gravity while doing this,
 * we add theight into the calculation and then degravitate. Don't
 * think about it too hard, or your head will explode.
 *
 * If we are using the gnome_pda hint, the entire process is done over
 * a smaller "pretend" root window, and then at the very end we shift
 * the window into the right place based using the left/top offsets. */

static void init_position(struct client *c)
{
    int xmax = DisplayWidth(dpy, screen);
    int ymax = DisplayHeight(dpy, screen);

#ifdef GNOME_PDA
    deskarea_t da;
    get_gnome_pda(&da);
    xmax -= da.left + da.right;
    ymax -= da.top + da.bottom;
#endif

    if (c->size->flags & (USSize)) {
        if (c->size->width) c->width = c->size->width;
        if (c->size->height) c->height = c->size->height;
    } else {
        /* make sure it's big enough to click at */
        if (c->width < 2 * theight(c)) c->width = 2 * theight(c);
        if (c->height < theight(c)) c->height = theight(c);
    }

    if (c->size->flags & USPosition) {
        c->x = c->size->x;
        c->y = c->size->y;
    } else {
        if (c->size->flags & PPosition) {
            c->x = c->size->x;
            c->y = c->size->y;
        }
        if (c->x < 0) c->x = 0;
        if (c->y < 0) c->y = 0;
        if (c->x > xmax) c->x = xmax - theight(c);
        if (c->y > ymax) c->y = ymax - theight(c);

        if (c->x == 0 && c->y == 0) {
            int mouse_x, mouse_y;
            get_mouse_position(&mouse_x, &mouse_y);
            if (c->width < xmax)
                c->x = (mouse_x < xmax ? (mouse_x / (float)xmax) : 1)
                    * (xmax - c->width - 2*BW(c));
            if (c->height + theight(c) < ymax)
                c->y = (mouse_y < ymax ? (mouse_y / (float)ymax) : 1)
                    * (ymax - c->height - theight(c) - 2*BW(c));
            c->y += theight(c);
            gravitate(c, REMOVE_GRAVITY);
        }
    }

}

/* Stick the client's window into our frame. */

#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)

static void reparent(struct client *c)
{
    XSetWindowAttributes pattr;

    pattr.override_redirect = True;
    pattr.background_pixel = bg.pixel;
    pattr.border_pixel = bd.pixel;
    pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
    c->frame = XCreateWindow(dpy, root,
        c->x, c->y - theight(c), c->width, c->height + theight(c), BW(c),
        DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);

#ifdef SHAPE
    if (shape) {
        XShapeSelectInput(dpy, c->window, ShapeNotifyMask);
        set_shape(c);
    }
#endif

    XAddToSaveSet(dpy, c->window);
    XSelectInput(dpy, c->window, ColormapChangeMask|PropertyChangeMask);
    XSetWindowBorderWidth(dpy, c->window, 0);
    XResizeWindow(dpy, c->window, c->width, c->height);
    XReparentWindow(dpy, c->window, c->frame, 0, theight(c));

    send_config(c);
}
