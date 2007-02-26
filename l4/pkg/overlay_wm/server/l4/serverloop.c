/*
 * \brief   OverlayWM - register and start L4 server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "serverloop.h"
#include "overlay-server.h"

extern char *overlay_name;

void enter_overlay_server_loop(void *arg) {
	CORBA_Server_Environment env = dice_default_server_environment;
	if (!names_register(overlay_name)) {
		return;
	}
	overlay_server_loop(&env);
}

