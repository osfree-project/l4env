/*
 * \brief   Nitpicker input event interface
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

#ifndef _NITPICKER_INPUT_H_
#define _NITPICKER_INPUT_H_


extern int mouse_x, mouse_y;      /* current mouse position */
extern int userstate;             /* user interaction state */


/*** INITIALIZE INPUT HANDLING ***/
extern int input_init(void);


/*** POLL INPUT LIB FOR NEW EVENTS AND HANDLE THEM BY CALLING A CALLBACK ***/
extern void foreach_input_event(void (*handle)(int type, int code, int rx, int ry));


#endif /* _NITPICKER_VIEW_H_ */
