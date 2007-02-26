/*
 * \brief   Nitpicker clipping stack handling
 * \date    2004-08-23
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
#include "clipping.h"


int clip_x1, clip_y1, clip_x2, clip_y2;  /* current clipping values */

static int csp;                          /* clipping stack pointer */
static int cstack_x1[MAX_CLIPSTACK];
static int cstack_y1[MAX_CLIPSTACK];
static int cstack_x2[MAX_CLIPSTACK];
static int cstack_y2[MAX_CLIPSTACK];


/*** SET (SHRINK) GLOBAL CLIPPING VALUES ***/
int push_clipping(int x, int y, int w, int h) {
	if (csp >= MAX_CLIPSTACK - 1) return -1;

	csp++;
	clip_x1 = cstack_x1[csp] = MAX(clip_x1, x);
	clip_y1 = cstack_y1[csp] = MAX(clip_y1, y);
	clip_x2 = cstack_x2[csp] = MIN(clip_x2, x + w - 1);
	clip_y2 = cstack_y2[csp] = MIN(clip_y2, y + h - 1);

	return 0;
}


/*** RESTORE PREVIOUS CLIPPING STATE ***/
int pop_clipping(void) {
	if (csp <= 0) return -1;

	csp--;
	clip_x1 = cstack_x1[csp];
	clip_y1 = cstack_y1[csp];
	clip_x2 = cstack_x2[csp];
	clip_y2 = cstack_y2[csp];

	return 0;
}


/*** SET INITIAL CLIPPING VALUES ***/
void clipping_init(int x, int y, int w, int h) {
	csp = 0;
	clip_x1 = cstack_x1[csp] = x;
	clip_y1 = cstack_y1[csp] = y;
	clip_x2 = cstack_x2[csp] = x + w - 1;
	clip_y2 = cstack_y2[csp] = y + h - 1;
}
