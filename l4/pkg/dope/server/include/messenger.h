/*
 * \brief   Interface of DOpE messenger module
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

#if !defined EVENT
#define EVENT void
#endif


struct messenger_services {
	void    (*send_input_event) (s32 app_id,EVENT *e,char *bindarg);
	void    (*send_action_event)(s32 app_id,char *action,char *bindarg);
};
