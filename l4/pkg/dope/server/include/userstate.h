/*
 * \brief   Interface of the userstate handler module of DOpE
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

#define USERSTATE_IDLE       0
#define USERSTATE_KEYREPEAT  1
#define USERSTATE_TOUCH      2
#define USERSTATE_DRAG       3
#define USERSTATE_GRAB       4

struct userstate_services {

	void     (*idle)    (void);
	void     (*touch)   (WIDGET *, void (*tick)   (WIDGET *, int dx, int dy),
	                               void (*release)(WIDGET *, int dx, int dy));
	void     (*drag)    (WIDGET *, void (*motion) (WIDGET *, int dx, int dy),
	                               void (*tick)   (WIDGET *, int dx, int dy),
	                               void (*release)(WIDGET *, int dx, int dy));
	void     (*grab)    (WIDGET *, void (*tick)   (WIDGET *, int dx, int dy));
	long     (*get)     (void);
	void     (*handle)  (void);
	WIDGET  *(*get_curr_focus)  (void);

};
