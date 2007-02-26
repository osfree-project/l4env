/*
 * \brief   Graphics functions of Nitpicker
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

#ifndef _NITPICKER_GFX_H_
#define _NITPICKER_GFX_H_

#include "types.h"
#include "font.h"


#define GFX_RGBA(r,g,b,a) (((r)<<24)|((g)<<16)|((b)<<8)|(a))
#define GFX_RGB(r,g,b) (((r)<<24)|((g)<<16)|((b)<<8)|255)

#define GFX_OP_SOLID   0
#define GFX_OP_DARKEN  1
#define GFX_OP_ALPHA   2
#define GFX_OP_TINT    3

/*
 * The interface to the gfx component is generic.
 * It can be implemented for different color depths.
 * The information about screen width and clipping are
 * taken from the global scr_* and clip_* variables.
 */

typedef struct gfx_interface {

	/*** DRAW FILLED BOX ***
	 *
	 * \param dst       destination screen
	 * \param dst_llen  destination line length
	 * \param x, y      position of the top left corner
	 * \param w, h      size
	 * \param rgba      color
	 */
	void (*draw_box) (void *dst, int dst_llen, int x, int y, int w, int h,
	                  u32 rgba);


	/*** DRAW IMAGE ***
	 *
	 * \param dst       destination screen
	 * \param dst_llen  destination line length
	 * \param x, y      position of the top left corner
	 * \param img_w     width of image
	 * \param img_h     height of image
	 * \param src       image pixels
	 * \param op        paint operation type
	 * \param rgba      color for tinting operation type
	 *
	 * The source and destination pixel
	 * format must be identical.
	 */
	void (*draw_img) (void *dst, int dst_llen, int x, int y, int img_w, int img_h,
	                  void *src, int op, u32 rgba);


	/*** DRAW TEXT STRING ***
	 *
	 * \param dst       destination screen
	 * \param dst_llen  destination line length
	 * \param x, y      position of the top left corner
	 * \param fnt       font to use for drawing
	 * \param rgba      color
	 * \param str       null-terminated ASCII string
	 */
	void (*draw_string) (void *dst, int dst_llen, int x, int y, font *fnt,
	                     u32 rgba, u8 *str);

} gfx_interface;

extern gfx_interface *gfx;

#endif /* _NITPICKER_GFX_H_ */
