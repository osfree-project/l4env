/*
 * \brief   Nitpicker buffer interface
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

#ifndef _NITPICKER_BUFFER_H_
#define _NITPICKER_BUFFER_H_

#include "nitpicker-server.h"

struct view;

typedef struct buffer {
	int   state;               /* state of the buffer index      */
	int   w, h;                /* width and height of the buffer */
	void *data;                /* pointer to pixel data          */
	l4dm_dataspace_t ds;       /* id of shared memory buffer     */
	struct view *view;         /* first view at this buffer      */
	CORBA_Object_base owner;   /* associated client id           */
} buffer;


/*** LOOK UP BUFFER STRUCT BY CLIENT ID AND BUFFER ID ***/
extern buffer *lookup_buffer(CORBA_Object tid, int buf_id);


#endif /* _NITPICKER_BUFFER_H_ */
