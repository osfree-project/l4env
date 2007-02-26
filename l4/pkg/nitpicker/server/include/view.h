/*
 * \brief   Nitpicker view interface
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_VIEW_H_
#define _NITPICKER_VIEW_H_

#include "nitpicker-server.h"

struct view;
struct buffer;

#define VIEW_FLAGS_STAYTOP     0x01   /* keep view always on top       */
#define VIEW_FLAGS_BORDER      0x02   /* draw border                   */
#define VIEW_FLAGS_TRANSPARENT 0x04   /* take rear views as background */
#define VIEW_FLAGS_LABEL       0x08   /* display label                 */
#define VIEW_FLAGS_FRONT       0x10   /* do not dim the view           */
#define VIEW_FLAGS_BACKGROUND  0x20   /* view is a background view     */

#define BORDER     3                  /* size of view border       */
#define MAX_LABEL 32                  /* max.characters of a label */

typedef struct view {
	int state;                    /* state of view                          */
	int flags;                    /* properties of the view                 */
	int x, y, w, h;               /* view position and size on screen       */
	unsigned long token;          /* private token of the application       */
	char label[MAX_LABEL];        /* label string (client-defined label)    */
	char *client_name;            /* name of client (trusted label)         */
	int lx, ly, lw, lh;           /* position and size of label             */
	struct buffer *buf;           /* buffer that is visible within the view */
	int buf_x, buf_y;             /* offset to the visible buffer area      */
	struct view *share_next;      /* next view of the same buffer           */
	struct view *next;            /* next view in ordered stacking list     */
	CORBA_Object_base listener;   /* receiver of events for this view       */
	CORBA_Object_base owner;      /* associated client id                   */
} view;

extern CORBA_Object_base curr_evrec;    /* current event receiver   */
extern unsigned long     curr_token;    /* token of focused view    */
extern view             *curr_view;     /* currently focused view   */
extern view             *first_view;    /* first view of view stack */


/*** LOOK UP VIEW STRUCTURE ***/
extern view *lookup_view(CORBA_Object tid, int view_id);


/*** REFRESH AREA WITHIN A VIEW ***
 *
 * \param v    View that should be updated on screen
 * \param dst  NULL if all views in the specified area should be
 *             refreshed or 'v' if the refresh should be limited to
 *             the specified view.
 */
extern void refresh_view(view *v, view *dst, int x, int y, int w, int h);


/*** FIND THE VIEW AT THE SPECIFIED SCREEN POSITION ***/
extern view *find_view(int x, int y);


/*** SET NEW ACTIVE VIEW ***/
extern void activate_view(view *v);


/*** DRAW VIEWS IN SPECIFIED AREA ***#
 *
 * \param cv   current view in view stack
 * \param dst  desired view to draw or NULL if all views should be drawn
 * \param exc  buffer not to redraw
 */
extern void draw_rec(view *cv, view *dst, buffer *exc,
                     int cx1, int cy1, int cx2, int cy2);

#endif /* _NITPICKER_VIEW_H_ */
