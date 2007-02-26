/*
 * \brief   Nitpicker buffer functions
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

/*** L4 INCLUDES ***/
#include <l4/l4rm/l4rm.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


/*** LOOK UP BUFFER STRUCT BY CLIENT ID AND BUFFER ID ***/
buffer *lookup_buffer(CORBA_Object tid, int buf_id) {
	client *c;
	
	if ((c = find_client(tid)) && (VALID_ID(tid, buf_id, c->buffers, c->max_buffers)))
		return &c->buffers[buf_id];

	return NULL;
}


/*** INTERFACE: IMPORT NEW BUFFER INTO NITPICKER ***/
int nitpicker_import_buffer_component(CORBA_Object _dice_corba_obj,
                                  const l4dm_dataspace_t *ds, int w, int h,
                                  CORBA_Server_Environment *_dice_corba_env) {
	int id;
	client *c;

	/* look up client data structure */
	if (!(c = find_client(_dice_corba_obj))) return -1;

	TRY((id = ALLOC_ID(c->buffers, c->max_buffers)) < 0, "Unable to allocate new buffer id");

	/* set initial values for the buffer */
	memset(&c->buffers[id], 0, sizeof(buffer));
	c->buffers[id].state   = STATE_ALLOCATED;
	c->buffers[id].owner   = *_dice_corba_obj;
	c->buffers[id].w       = w;
	c->buffers[id].h       = h;

	if (!ds) return id;

	c->buffers[id].ds = *ds;

	TRY(l4rm_attach(ds, w*h*scr_depth/8, 0, L4DM_RW, &c->buffers[id].data),
	    "Cannot attach dataspace");

	return id;
}


/*** INTERFACE: REMOVE BUFFER FROM NITPICKER ***/
void nitpicker_remove_buffer_component(CORBA_Object _dice_corba_obj,
                                  int buf_id,
                                  CORBA_Server_Environment *_dice_corba_env) {

	buffer *b;

	if (!(b = lookup_buffer(_dice_corba_obj, buf_id))) return;

	/* clean up local address space */
	l4rm_detach(b->data);

	/* mark buffer id as free */
	b->state = STATE_FREE;
}


/*** INTERFACE: REFRESH BUFFER AREA ***/
void nitpicker_refresh_component(CORBA_Object _dice_corba_obj,
                                int buf_id, int x, int y, int w, int h,
                                CORBA_Server_Environment *_dice_corba_env) {
	int x1, y1, x2, y2;
	view *v;
	buffer *b;

	if (!(b = lookup_buffer(_dice_corba_obj, buf_id))) return;

	/* for all views at this buffer */
	for (v = b->view; v; v = v->share_next) {

		/* calculate intersection of refresh area and view area */
		x1 = MAX(x - v->buf_x, 0) - BORDER;
		y1 = MAX(y - v->buf_y, 0) - BORDER;
		x2 = MIN(x + w - 1 - v->buf_x, v->w - 1) + BORDER;
		y2 = MIN(y + h - 1 - v->buf_y, v->h - 1) + BORDER;

		/* redraw intersection */
		if ((x1 <= x2) && (y1 <= y2))
			refresh_view(v, v, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	}
}
