/*!
 * \file	con/examples/linux_stub/xf86if.h
 * \brief	
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __CON_EXAMPLES_LINUX_STUB_XF86IF_H_
#define __CON_EXAMPLES_LINUX_STUB_XF86IF_H_

extern int  xf86if_init(void);
extern void xf86if_done(void);
extern int  xf86if_handle_redraw_event(void);
extern int  xf86if_handle_background_event(void);


extern int xf86used;

#endif

