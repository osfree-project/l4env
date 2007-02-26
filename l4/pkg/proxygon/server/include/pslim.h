/*
 * \brief   Proxygon internal pSLIM interface
 * \date    2004-09-30
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

#ifndef _PROXYGON_PSLIM_H_
#define _PROXYGON_PSLIM_H_

#include <l4/l4con/l4con_pslim.h>

#define pSLIM_FONT_CHAR_W       8
#define pSLIM_FONT_CHAR_H       12

/*** SOME TYPEDEFS FOR CONVENIENCE ONLY ***/
typedef l4con_pslim_color_t pslim_color_t;
typedef l4con_pslim_rect_t  pslim_rect_t;

struct pslim_data;

struct pslim {

	/*** QUERY GRAPHICS MODE ***/
	int  (*probe_mode) (struct pslim *, int width, int height, int depth);

	/*** SET CANVAS MODE ***
	 *
	 * \param  buf  buffer to which the drawing operation should be applied.
	 */
	int (*set_mode) (struct pslim *, int width, int height, int depth, void *buf);

	/*** DRAW FILLED RECTANGLE ***/
	int (*fill) (struct pslim *, const pslim_rect_t *, pslim_color_t);

	/*** BLIT RECTANGULAR PIXEL AREA ***/
	int (*copy) (struct pslim *, const pslim_rect_t *, int dx, int dy);

	/*** DRAW BITMAP ***/
	int (*bmap) (struct pslim *, const pslim_rect_t *, pslim_color_t fg,
	                             pslim_color_t bg, const void *bmap, char type);

	/*** DRAW COLOR IMAGE ***/
	int (*set) (struct pslim *, const pslim_rect_t *, const void *pmap);

	/*** DRAW YUV IMAGE ***/
	int (*cscs) (struct pslim *, const pslim_rect_t *, const char *y,
	                             const char *u, const char *v,
	                             long yuv_type, char scale);

	/*** DRAW STRING ***/
	int (*puts) (struct pslim *, const char *s, int len, int x, int y,
	                             pslim_color_t fg, pslim_color_t bg);

	/*** DRAW ANSI STRING ***/
	int (*puts_attr) (struct pslim *, const char *s, int len, int x, int y);

	/*** FREE PSLIM CANVAS ***/
	void (*destroy) (struct pslim *);

	struct pslim_data *pd;   /* private data of pSLIM object */
};


/*** CREATE A NEW PSLIM CANVAS OBJECT ***
 *
 * \param update      function that is called to update a canvas region
 *                    after drawing operations were performed. The update
 *                    function gets as arguments the x, y position, width
 *                    and height of the affected area and a private pointer.
 * \param update_arg  private pointer to be supplied for update calls.
 */
extern struct pslim *create_pslim_canvas(void (*update)(int, int, int, int, void *),
                                         void *update_arg);

#endif /* _PROXYGON_PSLIM_H_ */
