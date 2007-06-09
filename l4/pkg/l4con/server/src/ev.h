/* $Id$ */
/**
 * \file	l4con/server/src/ev.h
 * \brief	internals of `con' submodule, event stuff
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _EV_H
#define _EV_H

extern int use_omega0;
extern int nomouse;
extern int noshift;

void ev_init(void);

#endif /* !_EV_H */
