/*
 * \brief   Nitpicker view functions
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

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


view *first_view;  /* topmost view of view stack */
view *curr_view;   /* currently activated view   */


/**********************************
 *** FUNCTIONS FOR INTERNAL USE ***
 **********************************/

/*** UTILITY: DRAW RECTANGLE ***/
static inline void draw_rect(void *dst, int x, int y, int w, int h, u32 color) {
	gfx->draw_box(dst, x, y, w, 1, color);
	gfx->draw_box(dst, x, y, 1, h, color);
	gfx->draw_box(dst, x + w - 1, y, 1, h, color);
	gfx->draw_box(dst, x, y + h - 1, w, 1, color);
}


/*** UTILITY: DRAW FRAME AROUND A VIEW ***/
static inline void draw_frame(view *cv) {
	u32 color = (cv == curr_view) ? GFX_RGB(255,200,127) : GFX_RGB(255,255,255);

	/* draw frame around the view */
	if (cv->flags & VIEW_FLAGS_BORDER) {
		draw_rect(scr_adr, cv->x - 3, cv->y - 3, cv->w + 6, cv->h + 6, GFX_RGB(0,0,0));
		draw_rect(scr_adr, cv->x - 2, cv->y - 2, cv->w + 4, cv->h + 4, color);
		draw_rect(scr_adr, cv->x - 1, cv->y - 1, cv->w + 2, cv->h + 2, GFX_RGB(0,0,0));
	}
}


/*** UTILITY: DRAW VIEW TO PHYSICAL SCREEN ***/
static inline void draw_view(view *cv) {

	/* dimming in x-ray mode but not in kill mode */
	int op = (mode == MODE_SECURE) ? GFX_OP_DARKEN : GFX_OP_SOLID;

	if (!(mode & MODE_KILL)) {
//		if (curr_view && (curr_view->buf == cv->buf)) op = GFX_OP_SOLID;
		if (curr_view && (curr_view == cv)) op = GFX_OP_SOLID;
		if (cv->flags & VIEW_FLAGS_FRONT)   op = GFX_OP_SOLID;
	}

	push_clipping(cv->x, cv->y, cv->w, cv->h);

	/* draw views behind the current view */
	if (cv->flags & VIEW_FLAGS_TRANSPARENT) {
		draw_rec(cv->next, 0, 0, MAX(clip_x1, cv->x), MAX(clip_y1, cv->y),
		                         MIN(clip_x2, cv->x + cv->w - 1),
		                         MIN(clip_y2, cv->y + cv->h - 1));

		/* if we have a transparent background, draw the foreground masked */
		op = GFX_OP_ALPHA;
	}

	/* draw buffer */
	if (cv->buf && cv->buf->data)
		gfx->draw_img(scr_adr, cv->x - cv->buf_x, cv->y - cv->buf_y,
		              cv->buf->w, cv->buf->h, cv->buf->data, op);
	else
		gfx->draw_box(scr_adr, cv->x, cv->y, cv->w, cv->h, GFX_RGB(50,60,70));

	if ((mode & MODE_KILL) && !(cv->flags & VIEW_FLAGS_TRANSPARENT))
		gfx->draw_box(scr_adr, cv->x, cv->y, cv->w, cv->h, GFX_RGBA(255,0,0,127));

	pop_clipping();

	if (mode) draw_frame(cv);

	/* draw label */
	if (mode && (cv->flags & VIEW_FLAGS_LABEL) && cv->label) {
		u32 fgcol = (cv == curr_view) ? GFX_RGB(255,255,127) : GFX_RGB(216,216,216);
		push_clipping(cv->x, cv->y, cv->w, cv->h);
		DRAW_LABEL(scr_adr, cv->lx+2, cv->ly+2, GFX_RGB(0, 0, 0), fgcol, cv->client_name);
		DRAW_LABEL(scr_adr, cv->lx + font_string_width(default_font, cv->client_name) + 7,
		           cv->ly+2, GFX_RGB(0,0,0), fgcol - GFX_RGBA(24,24,0,0), cv->label);
		pop_clipping();
	}
}


/*** UTILITY: DETERMINE INTERSECTION OF A VIEW WITH SPECIFIED AREA ***
 *
 * \param v         view to check area against
 * \param x1, y1    left top point of area
 * \param x2, y2    right bottom point of area
 * \param dx1, dy1  resulting left top point of intersection area
 * \param dx2, dy2  resulting right bottom point of intersection area
 *
 * \return          1 if view area intersects with specified area
 *                  0 if there is no intersection
 */
static int intersect(view *v, int x1, int y1, int x2, int y2,
                     int *dx1, int *dy1, int *dx2, int *dy2) {

	int d, border = (mode && (v->flags & VIEW_FLAGS_BORDER))
	              ? BORDER : 0;

	*dx1 = MAX(x1, (d = v->x - border));
	*dx2 = MIN(x2,  d + v->w + border*2 - 1);
	*dy1 = MAX(y1, (d = v->y - border));
	*dy2 = MIN(y2,  d + v->h + border*2 - 1);

	return ((*dx1 <= *dx2) && (*dy1 <= *dy2));
}


/*** UTILITY: POSITION THE LABEL SUCH THAT IT IS VISIBLE ***
 *
 * \param cv         current view in the view stack
 * \param lv         view of the label to position
 * \param cx1, cy1   left top point of visible area at current depth
 * \param cx2, cy2   right bottom point of visible area
 */
static void place_label_rec(view *cv, view *lv,
                            int cx1, int cy1, int cx2, int cy2) {

	int sx1, sy1, sx2, sy2;  /* intersection area                          */
	static int cut_w;        /* widest visible area of the view            */
	static int placed;       /* flag that indicates a successful placement */

	/* sanity check */
	if (!cv || !lv) return;

	/* init for the first iteration */
	if (cv == first_view->next) cut_w = placed = 0;

	/* do not replace a label that has already a cosy position */
	if (placed) return;

	/* if there is an intersection */
	if (intersect(cv, cx1, cy1, cx2, cy2, &sx1, &sy1, &sx2, &sy2)) {

		/* if we hit the desired view */
		if (cv == lv) {

			/* if there is enough space, place label */
			if ((sx2 - sx1 + 1 > cv->lw) && (sy2 - sy1 + 1 > cv->lh)) {
				cv->lx = sx1 + (sx2 - sx1 + 1 - cv->lw)/2;
				cv->ly = sy1;
				placed = 1;   /* mark label as placed */
				return;

			} else if (sy2 - sy1 + 1 > cv->lh) {

				/*
				 * If the vertical space is big enough to hold the label but
				 * the available width is not enough, we place the label there
				 * and remeber the visible width. If we find a wider area
				 * than the previous, we place the label to the wider area.
				 * This way, we exhibit as much information of the label
				 * as possible.
				 */
				if ((sx2 - sx1 + 1 > cut_w) && !placed) {
					cv->lx = sx1;
					cv->ly = sy1;
					cut_w = sx2 - sx1 + 1;
				}
			}
		}

		/* check the next view of the view stack */
		if (cv->next == NULL) return;

		if (sy1 > cy1) place_label_rec(cv->next, lv, cx1, cy1, cx2, sy1 - 1);
		if (sx1 > cx1) place_label_rec(cv->next, lv, cx1, cy1, sx1 - 1, cy2);
		if (sx2 < cx2) place_label_rec(cv->next, lv, sx2 + 1, cy1, cx2, cy2);
		if (sy2 < cy2) place_label_rec(cv->next, lv, cx1, sy2 + 1, cx2, cy2);

	} else place_label_rec(cv->next, lv, cx1, cy1, cx2, cy2);
}


/*** UTILITY: PLACE LABEL TO A VISIBLE POSITION ***/
static void place_label(view *v) {
	int old_lx = v->lx, old_ly = v->ly;
	int x1 = v->x, x2 = x1 + v->w - 1;
	int y1 = v->y, y2 = y1 + v->h - 1;

	v->lx = v->x;
	v->ly = v->y;
	v->lw = font_string_width (default_font, v->label)
	      + font_string_width (default_font, v->client_name) + 5;
	v->lh = font_string_height(default_font, v->label);

	place_label_rec(first_view->next, v, MAX(0, x1), MAX(0, y1),
	                MIN(scr_width - 1, x2), MIN(scr_height - 1, y2));

	refresh_view(v, v, old_lx - v->x, old_ly - v->y, v->lw, v->lh);
	refresh_view(v, v, v->lx  - v->x, v->ly  - v->y, v->lw, v->lh);
}


/*** UTILITY: POSITION LABELS THAT ARE AFFECTED BY SPECIFIED AREA ***/
static void label_area(int x1, int y1, int x2, int y2) {
	view *cv = first_view->next;   /* ignore the mouse view */
	int ix1, iy1, ix2, iy2;        /* intersection area     */

	/* reposition label of each intersecting view but the last one (background) */
	for (; cv->next ; cv = cv->next) {
		if (intersect(cv, x1, y1, x2, y2, &ix1, &iy1, &ix2, &iy2))
			place_label(cv);
	}
}


/*** UTILITY: DETERMINE THE LAST STAYTOP VIEW OF THE VIEW STACK ***/
static view *get_last_staytop_view(void) {
	view *lv = first_view;

	for (; lv && (lv->flags & VIEW_FLAGS_STAYTOP); lv = lv->next)
		if (!lv->next || !(lv->next->flags & VIEW_FLAGS_STAYTOP))
			break;

	return (lv && !(lv->flags & VIEW_FLAGS_STAYTOP)) ? NULL : lv;
}


/*** UTILITY: DETERMINE THE LAST VIEW THAT IS NOT A BACKGROUND VIEW ***/
static view *get_last_normal_view(void) {
	view *lv = first_view;

	for (; lv && lv->next; lv = lv->next)
		if (lv->next->flags & VIEW_FLAGS_BACKGROUND)
			break;

	return lv;
}


/*** UTILITY: FIND PREDECESSOR VIEW IN VIEW STACK ***/
static view *find_predecessor(view *nv, int behind) {

	view *cv = get_last_staytop_view();

	for ( ; cv; cv = cv->next)
		if (( behind &&  (cv       == nv))
		 || (!behind && ((cv->next == nv) || !nv))) break;

	return cv ? cv : get_last_staytop_view();
}


/*** SET ACTIVE BACKGROUND TO SPECIFIED BACKGROUND VIEW ***/
static void activate_background(view *v) {

	UNCHAIN_LISTELEMENT(view, &first_view, next, v);

	if (dice_obj_eq(&curr_view->owner, &v->owner))
		CHAIN_LISTELEMENT(&first_view, next, get_last_normal_view(), v);
}


/***************************
 *** NITPICKER FUNCTIONS ***
 ***************************/

/*** DRAW VIEWS IN SPECIFIED AREA ***/
void draw_rec(view *cv, view *dst, buffer *exc,
              int cx1, int cy1, int cx2, int cy2) {

	int sx1, sy1, sx2, sy2;

	if (!cv) return;

	/* if there is an intersection - subdivide area */
	if (intersect(cv, cx1, cy1, cx2, cy2, &sx1, &sy1, &sx2, &sy2)) {

		/* draw current view */
		if ((!dst || (dst == cv) || (cv->flags & VIEW_FLAGS_TRANSPARENT))) {
			push_clipping(sx1, sy1, sx2 - sx1 + 1, sy2 - sy1 + 1);
			if (!cv->buf || cv->buf != exc) draw_view(cv);
			else if (mode) draw_frame(cv);
			pop_clipping();
		}

		/* take care about the rest */
		if (!(cv = cv->next)) return;
		if (sx1 > cx1) draw_rec(cv, dst, exc, cx1, MAX(cy1, sy1), sx1 - 1, MIN(cy2, sy2));
		if (sy1 > cy1) draw_rec(cv, dst, exc, cx1, cy1, cx2, sy1 - 1);
		if (sx2 < cx2) draw_rec(cv, dst, exc, sx2 + 1, MAX(cy1, sy1), cx2, MIN(cy2, sy2));
		if (sy2 < cy2) draw_rec(cv, dst, exc, cx1, sy2 + 1, cx2, cy2);

	} else draw_rec(cv->next, dst, exc, cx1, cy1, cx2, cy2);
}


/*** LOOK UP VIEW STRUCT BY CLIENT ID AND VIEW ID ***/
view *lookup_view(CORBA_Object tid, int view_id) {
	client *c;
	
	if ((c = find_client(tid)) && (VALID_ID(tid, view_id, c->views, c->max_views)))
		return &c->views[view_id];

	return NULL;
}



/*** UPDATE VIEW AREA ON SCREEN ***/
void refresh_view(view *v, view *dst, int x, int y, int w, int h) {
	int x1, y1, x2, y2;
	if (!v) return;

	/* clip argument agains view boundaries */
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > v->w - w) x = v->w - w;
	if (y > v->h - h) y = v->h - h;

	/* calculate area to redraw */
	x1 = v->x + x - BORDER;
	y1 = v->y + y - BORDER;
	x2 = v->x + x + w - 1 + BORDER*2;
	y2 = v->y + y + h - 1 + BORDER*2;

	draw_rec(first_view, dst, NULL, x1, y1, x2, y2);
}


/*** FIND THE VIEW AT THE SPECIFIED SCREEN POSITION ***/
view *find_view(int x, int y) {
	view *cv = first_view->next;

	for (; cv; cv = cv->next) {
		if ((x >= cv->x) && (x < cv->x + cv->w)
		 && (y >= cv->y) && (y < cv->y + cv->h)) return cv;
	}

	return NULL;
}


/*** SET NEW ACTIVE VIEW ***/
void activate_view(view *nv) {

	client *c = nv ? find_client(&nv->owner) : NULL;

	if (nv == curr_view) return;
	curr_view = nv;

	/* set background for the focused client */
	if (c && c->bg) activate_background(c->bg);

	/* refresh screen */
	draw_rec(first_view, NULL, NULL, 0, 0, scr_width - 1, scr_height - 1);

	if (nv && nv->client_name) menubar_set_text(nv->client_name);
	else menubar_set_text("");
}


/***************************
 *** INTERFACE FUNCTIONS ***
 ***************************/

/*** INTERFACE: CREATE NEW VIEW ***/
int nitpicker_new_view_component(CORBA_Object _dice_corba_obj,
                                 int buf_id, const_CORBA_Object listener,
                                 CORBA_Server_Environment *env) {
	int id;
	client *c;
	buffer *b;

	/* look up client data structure */
	if (!(c = find_client(_dice_corba_obj))) return -1;

	TRY((id = ALLOC_ID(c->views, c->max_views)) < 0, "Unable to allocate new view id");

	memset(&c->views[id], 0, sizeof(view));
	c->views[id].state       = STATE_ALLOCATED;
	c->views[id].flags       = VIEW_FLAGS_BORDER | VIEW_FLAGS_LABEL;
	c->views[id].owner       = *_dice_corba_obj;
	c->views[id].token       = id;
	c->views[id].lw          = 32;
	c->views[id].lh          = 16;
	c->views[id].client_name = c->label;

	strncpy(&c->views[id].label[0], "<unknown>", MAX_LABEL);
	c->views[id].label[MAX_LABEL - 1] = 0;

	if (listener) c->views[id].listener = *listener;

	if ((b = lookup_buffer(_dice_corba_obj, buf_id))) {

		/* assign buffer to view */
		c->views[id].buf = b;

		/* add view to the buffer's view list */
		c->views[id].share_next = b->view;
		b->view                 = &c->views[id];
	}

	/* place view at top most stack position after staytop views */
	CHAIN_LISTELEMENT(&first_view, next, get_last_staytop_view(), (&c->views[id]));

	return id;
}


/*** INTERFACE: DESTROY VIEW ***/
void nitpicker_destroy_view_component(CORBA_Object _dice_corba_obj,
                                 int view_id,
                                 CORBA_Server_Environment *env) {
	int x1, y1, x2, y2;
	view   *v;
	client *c = find_client(_dice_corba_obj);

	if (!(v = lookup_view(_dice_corba_obj, view_id))) return;

	/* remember original view location */
	x1 = v->x - BORDER; x2 = v->x + v->w - 1 + BORDER;
	y1 = v->y - BORDER; y2 = v->y + v->h - 1 + BORDER;

	/* reset focus if current view has focus */
	if (v == curr_view) activate_view(NULL);

	/* if view is a background, reset background reference */
	if (c && (v == c->bg)) c->bg = NULL;

	/* remove view from its buffers list */
	if (v->buf) UNCHAIN_LISTELEMENT(view, &v->buf->view, share_next, v);

	/* remove view from the view stack list */
	UNCHAIN_LISTELEMENT(view, &first_view, next, v);

	/* dealloc view label and mark view as free */
	v->state = STATE_FREE;

	/* refresh the original view area and update labels */
	draw_rec(first_view, NULL, NULL, x1, y1, x2, y2);
	label_area(x1, y1, x2, y2);
}


/*** INTERFACE: SET VIEW POSITION, SIZE, AND BUFFER OFFSET ***/
int nitpicker_set_view_port_component(CORBA_Object _dice_corba_obj,
                                     int view_id, int buf_x, int buf_y,
                                     int x, int y, int w, int h,
                                     int do_redraw,
                                     CORBA_Server_Environment *env) {
	int x1, y1, x2, y2;
	view *v;

	if (!(v = lookup_view(_dice_corba_obj, view_id))) return NITPICKER_ERR_ILLEGAL_ID;

	/* remember old position */
	x1 = v->x; x2 = v->x + v->w - 1;
	y1 = v->y; y2 = v->y + v->h - 1;

	v->buf_x = buf_x;
	v->buf_y = buf_y;
	v->x = x;
	v->y = y;
	v->w = w;
	v->h = h;

	/* determine compound area */
	x1 = MIN(x1, x) - BORDER;
	y1 = MIN(y1, y) - BORDER;
	x2 = MAX(x2, x + w - 1) + 2*BORDER;
	y2 = MAX(y2, y + h - 1) + 2*BORDER;

	/* update affected views */
	draw_rec(first_view, NULL, do_redraw ? NULL : v->buf, x1, y1, x2, y2);

	/* reposition labels that are affected of the changed area */
	if (!(v->flags & VIEW_FLAGS_STAYTOP)) label_area(x1, y1, x2, y2);

	return NITPICKER_OK;
}


/*** INTERFACE: POSITION VIEW IN VIEW STACK ***/
int nitpicker_stack_view_component(CORBA_Object _dice_corba_obj, int view_id,
                                   int neighbor_id, int behind,
                                   CORBA_Server_Environment *env) {
	view *v, *nv;

	if (!(v = lookup_view(_dice_corba_obj, view_id))) return NITPICKER_ERR_ILLEGAL_ID;

	nv = (neighbor_id == -1) ? NULL : lookup_view(_dice_corba_obj, neighbor_id);

	/* remove view from original view stack position */
	UNCHAIN_LISTELEMENT(view, &first_view, next, v);

	/* insert view at new stack position */
	CHAIN_LISTELEMENT(&first_view, next, find_predecessor(nv, behind), v);

	/* refresh affected screen area */
	refresh_view(v, 0, 0, 0, v->w, v->h);
	label_area(v->x, v->y, v->x + v->w - 1, v->y + v->h - 1);

	return NITPICKER_OK;
}


/*** INTERFACE: REDEFINE VIEW TITLE ***/
int nitpicker_set_view_title_component(CORBA_Object _dice_corba_obj,
                                   int view_id, const char* title,
                                   CORBA_Server_Environment *env) {
	view *v;

	if (!(v = lookup_view(_dice_corba_obj, view_id))) return NITPICKER_ERR_ILLEGAL_ID;

	/* set new title */
	strncpy(&v->label[0], title, MAX_LABEL);
	v->label[MAX_LABEL - 1] = 0;

	refresh_view(v, v, 0, 0, v->w, v->h);

	return NITPICKER_OK;
}


/*** INTERFACE: SET MOUSE OPERATING MODE ***/
int nitpicker_set_background_component(CORBA_Object _dice_corba_obj, int view_id,
                                       CORBA_Server_Environment *env) {

	client *c = find_client(_dice_corba_obj);

	if (!c) return NITPICKER_ERR_ILLEGAL_ID;

	/* define new background for the client (invalid view_id -> no background) */
	if ((c->bg = lookup_view(_dice_corba_obj, view_id))) {

		/* remove background attribute from old background */
		c->bg->flags |= VIEW_FLAGS_BACKGROUND;

		if (curr_view && dice_obj_eq(&c->bg->owner, &curr_view->owner))
			activate_background(c->bg);
	}

	return NITPICKER_OK;
}


/*** INTERFACE: SET MOUSE OPERATING MODE ***/
int nitpicker_set_mousemode_component(CORBA_Object _dice_corba_obj,
                                      int view_id, int mode,
                                      CORBA_Server_Environment *env) {
	return NITPICKER_OK;
}


