/*
 * \brief   Interface of DOpE Frame widget module
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

struct frame_methods;
struct frame_data;

#if !defined(WIDGET)
#define WIDGET void
#endif

#define FRAME struct frame

struct frame {
	struct widget_methods *gen;
	struct frame_methods  *frame;
	struct widget_data    *wd;
	struct frame_data     *fd;
};

struct frame_methods {
	void    (*set_content) (FRAME *,WIDGET *new_content);
	WIDGET *(*get_content) (FRAME *);
	void    (*set_scrollx) (FRAME *,int flag);
	int     (*get_scrollx) (FRAME *);
	void    (*set_scrolly) (FRAME *,int flag);
	int     (*get_scrolly) (FRAME *);
	void    (*set_xview)   (FRAME *,s32 xview);
	s32     (*get_xview)   (FRAME *);
	void    (*set_yview)   (FRAME *,s32 yview);
	s32     (*get_yview)   (FRAME *);
	void    (*set_background) (FRAME *,int bg);
	int     (*get_background) (FRAME *);
	void    (*xscroll_update) (FRAME *,u16 redraw_flag);
	void    (*yscroll_update) (FRAME *,u16 redraw_flag);
};


struct frame_services {
	FRAME *(*create) (void);
};
