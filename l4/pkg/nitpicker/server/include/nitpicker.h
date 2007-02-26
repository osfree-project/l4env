/*
 * \brief   General definitions an macros for Nitpicker
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

#ifndef _NITPICKER_NITPICKER_H_
#define _NITPICKER_NITPICKER_H_


/**********************************
 *** COMPILE TIME CONFIGURATION ***
 **********************************/

#define MAX_CLIPSTACK    64   /* maximum size of clipping stack */
#define MAX_INPUT_EVENTS 64   /* size of input event queue      */


#define NITPICKER_OK                0
#define NITPICKER_ERR_ILLEGAL_ID   -11
#define NITPICKER_ERR_OUT_OF_MEM   -12


/************************
 *** GENERAL INCLUDES ***
 ************************/

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>


/**********************
 *** LOCAL INCLUDES ***
 **********************/

#include "nitpicker-server.h"
#include "listmacros.h"
#include "clipping.h"
#include "startup.h"
#include "screen.h"
#include "server.h"
#include "buffer.h"
#include "client.h"
#include "input.h"
#include "types.h"
#include "view.h"
#include "main.h"
#include "font.h"
#include "gfx.h"


#if !defined(NULL)
#define NULL (void *)0
#endif


/***********************
 *** GENERAL DEFINES ***
 ***********************/

#define STATE_FREE      0
#define STATE_ALLOCATED 1


/**********************
 *** UTILITY MACROS ***
 **********************/

/*** TRY TO EXECUTE A COMMAND AND PRINT EXPRESSIVE ERROR MESSAGE ***
 *
 * \param command   command to execute
 * \param fmt       format string of error message
 * \param args      format string arguments
 */
#define TRY(command, fmt, args...) {                  \
	int ret = command;                                \
	if (ret) {                                        \
		printf("Error: " fmt, ## args); printf("\n"); \
		return ret;                                   \
	}                                                 \
}


/*** FIND FREE ARRAY ELEMENT ***
 *
 * The array elements must be structures with a member called
 * state. An element is marked as free when its state equals
 * STATE_FREE.
 *
 * \param elem_array   array to search in
 * \param max_elems    size of array
 * \return             index of free element or -1 if
 *                     there is not free element
 */
#define ALLOC_ID(elem_array, max_elems) ({                               \
	int i = 0;                                                           \
	for (; (i<(max_elems)) && (elem_array[i].state != STATE_FREE); i++); \
	if (i == (max_elems)) i = -1;                                        \
	i;   /* return value of the macro */                                 \
})


/*** CHECK IF VIEW OR BUFFER ID IS VALID ***
 *
 * \param id      id to check
 * \param array   array to check the id against
 * \param max_id  size of array
 * \return        0 if the id is valid
 *                1 if the id is invalid
 */
#define VALID_ID(tid, id, array, max_id) \
	((id >= 0) && (id < max_id) &&  \
	 dice_obj_eq(&array[id].owner, tid))


#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/*** DRAW A TEXT WITH AN OUTLINE ***/
#define DRAW_LABEL(dst_adr, dst_llen, x, y, bgcol, fgcol, text) { \
	gfx->draw_string(dst_adr, dst_llen, x - 1, y, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x + 1, y, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x - 1, y - 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x + 1, y - 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x - 1, y + 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x + 1, y + 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x, y - 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x, y + 1, default_font, bgcol, text); \
	gfx->draw_string(dst_adr, dst_llen, x, y,     default_font, fgcol, text); \
}


#endif /* _NITPICKER_NITPICKER_H_ */
