#ifndef _XF86IF_H
#define _XF86IF_H

extern int  xf86if_init(void);
extern void xf86if_done(void);
extern int  xf86if_handle_redraw_event(void);
extern int  xf86if_handle_background_event(void);


extern int xf86used;

#endif

