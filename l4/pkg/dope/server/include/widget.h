/*
 * \brief   General DOpE widget structures
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

#if !defined(WIDGET)
#define WIDGET struct widget
#endif

#if !defined(WIDGETARG)
#define WIDGETARG WIDGET
#endif

#if !defined(EVENT)
#define EVENT void
#endif

#define WID_UPDATE_HIDDEN 0
#define WID_UPDATE_REDRAW 1

struct widget_methods;

struct gfx_ds;

struct widget {
	struct widget_methods *gen;
};

/*** FUNCTIONS THAT ALL WIDGETS HAVE IN COMMON ***/
struct widget_methods {
	void    (*destroy)      (WIDGETARG *);
	char   *(*get_type)     (WIDGETARG *);
	s32     (*get_app_id)   (WIDGETARG *);
	void    (*set_app_id)   (WIDGETARG *,s32 app_id);
	long    (*get_x)        (WIDGETARG *);
	void    (*set_x)        (WIDGETARG *,long new_x);
	long    (*get_y)        (WIDGETARG *);
	void    (*set_y)        (WIDGETARG *,long new_y);
	long    (*get_w)        (WIDGETARG *);
	void    (*set_w)        (WIDGETARG *,long new_w);
	long    (*get_h)        (WIDGETARG *);
	void    (*set_h)        (WIDGETARG *,long new_h);
	long    (*get_min_w)    (WIDGETARG *);
	long    (*get_min_h)    (WIDGETARG *);
	long    (*get_max_w)    (WIDGETARG *);
	long    (*get_max_h)    (WIDGETARG *);
	long    (*get_abs_x)    (WIDGETARG *);
	long    (*get_abs_y)    (WIDGETARG *);
	int     (*get_state)    (WIDGETARG *);
	void    (*set_state)    (WIDGETARG *,int new_state);
	WIDGET *(*get_parent)   (WIDGETARG *);
	void    (*set_parent)   (WIDGETARG *,void *parent);
	void   *(*get_context)  (WIDGETARG *);
	void    (*set_context)  (WIDGETARG *,void *context);
	WIDGET *(*get_next)     (WIDGETARG *);
	void    (*set_next)     (WIDGETARG *,WIDGETARG *next);
	WIDGET *(*get_prev)     (WIDGETARG *);
	void    (*set_prev)     (WIDGETARG *,WIDGETARG *prev);
	void    (*draw)         (WIDGETARG *,struct gfx_ds *dst,long x,long y);
	void    (*update)       (WIDGETARG *,u16 redraw_flag);
	WIDGET *(*find)         (WIDGETARG *,long x,long y);
	void    (*inc_ref)      (WIDGETARG *);
	void    (*dec_ref)      (WIDGETARG *);
	void    (*force_redraw) (WIDGETARG *);
	int     (*get_focus)    (WIDGETARG *);
	void    (*set_focus)    (WIDGETARG *,int new_focus);
	void    (*handle_event) (WIDGETARG *,EVENT *);
	WIDGET *(*get_window)   (WIDGETARG *);
	u16     (*do_layout)    (WIDGETARG *,WIDGETARG *,u16 redraw_flag);
	void    (*bind)         (WIDGETARG *,char *bind_ident,char *message);
	u8     *(*get_bind_msg) (WIDGETARG *,u8 *bind_ident);
};
