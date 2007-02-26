/* aewm - a minimalist X11 window mananager. vim:sw=4:ts=4:et
 * Copyright 1998-2002 Decklin Foster <decklin@red-bean.com>
 * This program is free software; see LICENSE for details. */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

/*** X11 INCLUDES ***/
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** LOCAL INCLUDES ***/
#include "init.h"
#include "misc.h"
#include "events.h"
#include "new.h"
#include "client.h"

/*** OVERLAY SERVER INCLUDES ***/
#include "config.h"
#include "ovl_window.h"

Display *dpy;
Window root;
int screen;
XFontStruct *font;
GC invert_gc;
GC string_gc;
GC border_gc;
XColor fg;
XColor bg;
XColor bd;
Cursor move_curs;
Cursor resize_curs;
Atom wm_protos;
Atom wm_delete;
Atom wm_state;
Atom wm_change_state;
struct client *head_client;
static char *opt_font = DEF_FONT;
static char *opt_fg = DEF_FG;
static char *opt_bg = DEF_BG;
static char *opt_bd = DEF_BD;
char *opt_new1 = DEF_NEW1;
char *opt_new2 = DEF_NEW2;
char *opt_new3 = DEF_NEW3;
int opt_bw = DEF_BW;
int opt_pad = DEF_PAD;
#ifdef SHAPE
Bool shape;
int shape_event;
#endif

#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)
#define ButtonMask (ButtonPressMask|ButtonReleaseMask)

//long app_id;
//sm_exc_t _ev;
//l4_threadid_t dope_l4id;
//char dope_retbuf[256];
//char *dope_retp = dope_retbuf;


static void scan_wins(void);
static void setup_display(void);
//static void read_config_helper(FILE *);
//static void read_config(char *);
//static int init_dopeapp(void);


static void quit_nicely(void)
{
    unsigned int nwins, i;
    Window dummyw1, dummyw2, *wins;
    struct client *c;

    XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
    for (i = 0; i < nwins; i++) {
        c = find_client(wins[i], FRAME);
        if (c) remove_client(c, REMAP);
    }
    XFree(wins);

    XFreeFont(dpy, font);
    XFreeCursor(dpy, move_curs);
    XFreeCursor(dpy, resize_curs);
    XFreeGC(dpy, invert_gc);
    XFreeGC(dpy, border_gc);
    XFreeGC(dpy, string_gc);

    XInstallColormap(dpy, DefaultColormap(dpy, screen));
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);

    XCloseDisplay(dpy);
    exit(0);
}

void sig_handler(int signal)
{
    switch (signal) {
        case SIGINT:
        case SIGTERM:
        case SIGHUP:
            quit_nicely(); break;
        case SIGCHLD:
            wait(NULL); break;
    }
}

int main(int argc, char **argv)
{
//    int i;
    struct sigaction act;

    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGCHLD, &act, NULL);

    printf("main()\n");

//    init_dopeapp();
	ovl_window_init(NULL);

    setup_display();
    scan_wins();
    do_event_loop();
    return 1; /* just another brick in the -Wall */
}

static void scan_wins(void)
{
    unsigned int nwins, i;
    Window dummyw1, dummyw2, *wins;
    XWindowAttributes attr;

    XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
    for (i = 0; i < nwins; i++) {
        XGetWindowAttributes(dpy, wins[i], &attr);
        if (!attr.override_redirect && attr.map_state == IsViewable)
            make_new_client(wins[i]);
    }
    XFree(wins);
}

static void setup_display(void)
{
    XColor dummyc;
    XGCValues gv;
    XSetWindowAttributes sattr;
#ifdef SHAPE
    int dummy;
#endif

    dpy = XOpenDisplay(NULL);

    if (!dpy) {
        printf("can't open display '%s' (is $DISPLAY set properly?)",
            getenv("DISPLAY"));
        exit(1);
    }

    XSetErrorHandler(handle_xerror);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_state = XInternAtom(dpy, "WM_STATE", False);
    wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);

    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummyc);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummyc);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bd, &bd, &dummyc);

    font = XLoadQueryFont(dpy, opt_font);
    if (!font) { printf("font '%s' not found", opt_font); exit(1); }

#ifdef XFT
    xft_fg.color.red = fg.red;
    xft_fg.color.green = fg.green;
    xft_fg.color.blue = fg.blue;
    xft_fg.color.alpha = 0xffff;
    xft_fg.pixel = fg.pixel;

    xftfont = XftFontOpenXlfd(dpy, DefaultScreen(dpy), opt_font);
    if (!xftfont) { printf("font '%s' not found", opt_font); exit(1); }
#endif

#ifdef SHAPE
    shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
#endif

    move_curs = XCreateFontCursor(dpy, XC_fleur);
    resize_curs = XCreateFontCursor(dpy, XC_plus);

    gv.function = GXcopy;
    gv.foreground = fg.pixel;
    gv.font = font->fid;
    string_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCFont, &gv);

    gv.foreground = bd.pixel;
    gv.line_width = opt_bw;
    border_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCLineWidth, &gv);

    gv.function = GXinvert;
    gv.subwindow_mode = IncludeInferiors;
    invert_gc = XCreateGC(dpy, root, GCFunction|GCSubwindowMode|GCLineWidth|GCFont, &gv);

    sattr.event_mask = ChildMask|ColormapChangeMask|ButtonMask;
    XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);
}


//static int dope_inited = 0;
//  
//static int init_dopeapp(void) {
//    char *listener_ident;
//
//    printf("init_dopeapp: get thread id of DOpE from Names\n");
//	if (!names_waitfor_name("DOpE", &dope_l4id, 5000)) {
//		printf("Error: can not connect to DOpE\n");
//		return -1;
//	}
//
//    listener_ident = "Hallo";
//    printf("init_dopeapp: start dope listener\n");
//    listener_ident = start_dope_listener();
//    printf("init_dopeapp: dope_listener identifier: %s\n",listener_ident);
//	app_id = dope_manager_init_app(dope_l4id,"DOpEwm","",&_ev);
//
//    printf("init_dopeapp: create console window\n");    
//	dope_manager_exec_cmd(dope_l4id,app_id,"tw=new Window()",&dope_retp,&_ev);
//	dope_manager_exec_cmd(dope_l4id,app_id,"term=new Terminal()",&dope_retp,&_ev);
//	dope_manager_exec_cmd(dope_l4id,app_id,"tw.set(-x 20 -y 20 -w 300 -h 300 -content term -scrollx yes -scrolly yes",&dope_retp,&_ev);
//	dope_manager_exec_cmd(dope_l4id,app_id,"tw.open()",&dope_retp,&_ev);
//
//	dope_inited = 1;
//	return 0;
//}



//void dope_print(char *s) {
//	static char strbuf[256];
//    static int i;
//	sprintf(&strbuf[0],"term.print(\"%s\")",s);
//	if (dope_inited) dope_manager_exec_cmd(dope_l4id,app_id,&strbuf[0],&dope_retp,&_ev);
//}

