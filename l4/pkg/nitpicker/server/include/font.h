/*
 * \brief   Nitpicker font interface
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

#ifndef _NITPICKER_FONT_H_
#define _NITPICKER_FONT_H_

#include "types.h"

typedef struct font {
	s32 *width_table;
	s32 *offset_table;
	s32  img_w, img_h;
	u8  *image;
} font;

extern font *default_font;


/*** CALCULATE WIDTH OF TEXT STRING WITH THE SPECIFIED FONT ***
 *
 * \param f    font
 * \param str  text string
 * \return     width of text string in pixels
 */
extern int font_string_width(font *f, char *str);


/*** DETERMINE HEIGHT OF TEXT STRING WITH THE SPECIFIED FONT ***
 *
 * \param f    font
 * \param str  text string
 * \return     height of text string in pixels
 */
extern int font_string_height(font *f, char *str);


/*** INIT FONT ***/
extern void font_init(void);

#endif /* _NITPICKER_FONT_H_ */
