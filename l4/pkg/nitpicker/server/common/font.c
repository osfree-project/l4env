/*
 * \brief   Nitpicker font handling
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

extern char _binary_default_tff_start[];

font builtin_font, *default_font = &builtin_font;


/*** CALCULATE WIDTH OF TEXT STRING WITH THE SPECIFIED FONT ***/
int font_string_width(font *f, char *str) {
	int result = 0;
	for (; str && *str; str++) result += f->width_table[(int)(*str)];
	return result;
}


/*** DETERMINE HEIGHT OF TEXT STRING WITH THE SPECIFIED FONT ***
 *
 * The height of the text string does not depend on the string
 * because we currently support only horizontal text.
 */
int font_string_height(font *f, char *str) {
	return f->img_h;
}


/*** INITIALIZE FONT HANDLING ***/
void font_init(void) {
	builtin_font.offset_table =   (s32 *)&_binary_default_tff_start;
	builtin_font.width_table  =   (s32 *)((u32)&_binary_default_tff_start + 1024);
	builtin_font.img_w        = *((u32 *)((u32)&_binary_default_tff_start + 2048));
	builtin_font.img_h        = *((u32 *)((u32)&_binary_default_tff_start + 2052));
	builtin_font.image        =    (u8 *)((u32)&_binary_default_tff_start + 2056);
}
