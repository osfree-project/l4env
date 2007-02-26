
/*** GENERAL INCLUDES ***/
#include <stdio.h>
//#include <stdarg.h>
//#include <string.h>
//#include <unistd.h>
//#include <signal.h>

/*** X11 INCLUDES ***/
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*** LOCAL INCLUDES ***/
#include "init.h"
#include "xmisc.h"

int handle_xerror(Display *dpy, XErrorEvent *e)
{
    /* FIXME: should this be here or not? */
    /* struct client *c = find_client(e->resourceid, WINDOW); */

    if (e->error_code == BadAccess && e->resourceid == root) {
        printf("root window unavailible (maybe another wm is running?)");
        exit(1);
    } else {
        char msg[255];
        XGetErrorText(dpy, e->error_code, msg, sizeof msg);
        printf("X error (%#lx): %s", e->resourceid, msg);
    }

    /* if (c) remove_client(c, WITHDRAW); */
    return 0;
}

void get_mouse_position(int *x, int *y)
{
    Window mouse_root, mouse_win;
    int win_x, win_y;
    unsigned int mask;

    XQueryPointer(dpy, root, &mouse_root, &mouse_win,
        x, y, &win_x, &win_y, &mask);
}


