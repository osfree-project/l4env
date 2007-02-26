/*
 * \brief   Interface of VScreen server module
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

#if !defined(THREAD)
#define THREAD void
#endif

#if !defined(VSCREEN)
#define VSCREEN void
#endif

struct vscr_server_services {
	THREAD *(*start) (VSCREEN *);
};
