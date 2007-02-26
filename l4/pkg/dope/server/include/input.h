/*
 * \brief   Interface of input event layer of DOpE
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
#define WIDGET void
#endif

struct input_services {
	long    (*get_mx)       (void);
	long    (*get_my)       (void);
	long    (*get_mb)       (void);
	void    (*set_pos)      (long x,long y);
	long    (*get_keystate) (long keycode);
	char    (*get_ascii)    (long keycode);
	void    (*update)       (WIDGET *dst);
	void    (*update_properties)(void);
};
