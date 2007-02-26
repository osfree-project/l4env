/*
 * \brief   Interface of DOpE Button widget module
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

struct button_methods;
struct button_data;

#define BUTTON struct button

struct button {
	struct widget_methods *gen;
	struct button_methods *but;
	struct widget_data    *wd;
	struct button_data    *bd;
};

struct button_methods {
	void    (*set_text)    (BUTTON *, char *new_txt);
	char   *(*get_text)    (BUTTON *);
	void    (*set_font)    (BUTTON *, s32 new_font_id);
	s32     (*get_font)    (BUTTON *);
	void    (*set_style)   (BUTTON *, s32 new_style);
	s32     (*get_style)   (BUTTON *);
	void    (*set_click)   (BUTTON *, void (*)(BUTTON *));
	void    (*set_release) (BUTTON *, void (*)(BUTTON *));
};

struct button_services {
	BUTTON *(*create) (void);
};
