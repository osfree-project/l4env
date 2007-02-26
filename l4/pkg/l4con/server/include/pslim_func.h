/* $Id$ */

/*	con/server/include/pslim_func.h
 *
 *	pseudoSLIM functions
 */

#ifndef _pSLIM_FUNC_H
#define _pSLIM_FUNC_H

#include <l4/con/l4con_pslim.h>

#include "con.h"

extern
void pslim_fill(struct l4con_vc*, int from_user, pslim_rect_t *rect,
		pslim_color_t color);

extern
void pslim_bmap(struct l4con_vc*, int from_user, pslim_rect_t *rect,
		pslim_color_t fgc, pslim_color_t bgc, void* bmap, 
		l4_uint8_t mode);

extern
void pslim_set(struct l4con_vc*, int from_user, pslim_rect_t *rect,
	       void* pmap);

extern
void pslim_copy(struct l4con_vc *vc, int from_user, pslim_rect_t *rect,
		l4_int16_t dx, l4_int16_t dy);

extern
void pslim_cscs(struct l4con_vc *vc, int from_user, pslim_rect_t *rect,
		void* y, void* u, void* v, l4_uint8_t mode, l4_uint32_t scale);

extern void sw_copy(struct l4con_vc*, int, int, int, int, int, int);
extern void sw_fill(struct l4con_vc*, int, int, int, int, unsigned col);

#endif /* !_pSLIM_FUNC_H */

