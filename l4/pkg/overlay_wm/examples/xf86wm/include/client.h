
/* Multipliers for calling gravitate */

#define APPLY_GRAVITY 1
#define REMOVE_GRAVITY -1

/* Modes for find_client */

#define WINDOW 0
#define FRAME 1


/* And finally modes for remove_client. */

#define WITHDRAW 0
#define REMAP 1

/* This structure keeps track of top-level windows (hereinafter
 * 'clients'). The clients we know about (i.e. all that don't set
 * override-redirect) are kept track of in linked list starting at the
 * global pointer called, appropriately, 'clients'. 
 *
 * window and parent refer to the actual client window and the larget
 * frame into which we will reparent it respectively. trans is set to
 * None for regular windows, and the window's 'owner' for a transient
 * window. Currently, we don't actually do anything with the owner for
 * transients; it's just used as a boolean.
 *
 * ignore_unmap is for our own purposes and doesn't reflect anything
 * from X. Whenever we unmap a window intentionally, we increment
 * ignore_unmap. This way our unmap event handler can tell when it
 * isn't supposed to do anything. */

struct client {
    struct client   *next;
    char       *name;
    XSizeHints *size;
    int        ovlwin_id;
    Window     window, frame, trans;
    Colormap   cmap;
    int        x, y, width, height;
    int        ignore_unmap;
};

extern struct client *find_client(Window, int);
extern int theight(struct client *);
extern void set_wm_state(struct client *, int);
extern long get_wm_state(struct client *);
extern void send_config(struct client *);
extern void remove_client(struct client *, int);
extern void redraw(struct client *);
extern void gravitate(struct client *, int);

