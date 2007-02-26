/*
 * \brief   Interface of the real-time manager of DOpE
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
#include "widget.h"
#endif

#if !defined(MUTEX)
#define MUTEX void
#endif

struct rtman_services {
	void (*execute)        (void);
	s32  (*add)            (WIDGET *w,float duration);
	void (*remove)         (WIDGET *w);
	void (*set_sync_mutex) (WIDGET *w,MUTEX *);
};
