/*
 * \brief   Nitpicker clipping handling interface
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

#ifndef _NITPICKER_CLIPPING_H_
#define _NITPICKER_CLIPPING_H_


/*** CURRENT CLIPPING VALUES ***/
extern int clip_x1, clip_y1, clip_x2, clip_y2;


/*** SET (SHRINK) GLOBAL CLIPPING VALUES ***/
extern int push_clipping(int x, int y, int w, int h);


/*** RESTORE PREVIOUS CLIPPING STATE ***/
extern int pop_clipping(void);


/*** SET INITIAL CLIPPING AREA ***/
extern void clipping_init(int x, int y, int w, int h);


#endif /* _NITPICKER_CLIPPING_H_ */
