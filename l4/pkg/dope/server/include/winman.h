/*
 * \brief   Interface of the window manager module of DOpE
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#if !defined(WINDOW)
#include "window.h"
#endif

#if !defined(WIDGET)
#include "widget.h"
#endif

struct winman_services {

	void     (*add)         (WINDOW *win);
	void     (*remove)      (WINDOW *win);
	void     (*draw)        (WINDOW *win,long x1, long y1, long x2, long y2);
	void     (*draw_behind) (WINDOW *win,long x1, long y1, long x2, long y2);
	void     (*activate)    (WINDOW *win);
	void     (*top)         (WINDOW *win);
	void     (*move)        (WIDGET *win,long ox,long oy,long nx,long ny);
	WIDGET  *(*find)        (long x,long y);
	void     (*reorder)     (void);
	void     (*update_properties) (void);
	
};
