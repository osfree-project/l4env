/*
 * \brief   OverlayWM - register and start L4 server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "serverloop.h"
#include "overlay-server.h"

void enter_overlay_server_loop(void *arg) {
	CORBA_Environment env = dice_default_environment;
	env.timeout = L4_IPC_TIMEOUT(250,14,0,0,0,0); /* send timeout 1ms */
	if (!names_register("OvlWM")) {
		return;
	}
	overlay_server_loop(&env);
}

