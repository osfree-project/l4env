/*
 * \brief   Nitpicker misc interface
 * \date    2004-08-24
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

#ifndef _NITPICKER_MAIN_H_
#define _NITPICKER_MAIN_H_


/*** NITPICKER MODE ***/
extern int mode;

#define MODE_SECURE 0x1
#define MODE_KILL   0x2


/*** USER INTERACTION STATE ***/
extern int userstate;

#define USERSTATE_IDLE 0
#define USERSTATE_DRAG 1


/*** SET TEXT IN MENUBAR ***/
extern void menubar_set_text(char *trusted_text, char *untrusted_text);


/*** USER INPUT HANDLER ***/
extern void handle_normal_input(int type, int code, int rx, int ry);


extern CORBA_Object myself;


#endif /* _NITPICKER_MAIN_H_ */
