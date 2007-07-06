/*
 * \brief   DOpE event structure
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _DOPE_EVENT_H_
#define _DOPE_EVENT_H_


#define EVENT_PRESS         1
#define EVENT_RELEASE       2
#define EVENT_MOTION        3
#define EVENT_MOUSE_ENTER   4
#define EVENT_MOUSE_LEAVE   5
#define EVENT_KEY_REPEAT    6
#define EVENT_ABSMOTION     7
#define EVENT_ACTION       99

#define EVENT struct event
struct event {
	long type;
	long code;          /* key/button-code                 */
	long abs_x, abs_y;  /* current absolute mouse position */
	long rel_x, rel_y;  /* relative mouse position change  */
};


#endif /* _DOPE_EVENT_H_ */
