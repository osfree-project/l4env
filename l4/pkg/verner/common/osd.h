/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Font header (contains the font definition) -
 *
 *  Copyright(C) 2002-2003 Peter Ross <pross@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 ****************************************************************************/

/*
 * cr7:
 * adapted to use without XviD and w/ VERNER
 */

#ifndef _FONT_H_
#define _FONT_H_

void yuv_image_printf(unsigned char *out_buffer, int xdim, int ydim, int x, int y, char *fmt, ...);

#endif /* _FONT_H_ */
