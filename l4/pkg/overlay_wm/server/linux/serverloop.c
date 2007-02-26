/*
 * \brief   OverlayWM - start Linux socket server
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** LOCAL INCLUDES ***/
#include "serverloop.h"
#include "overlay-server.h"

static CORBA_Environment ovl_env = dice_default_environment;

void enter_overlay_server_loop(void *arg) {
	ovl_env.srv_port = htons(13246);
	overlay_server_loop(&ovl_env);
}

