/*
 * \brief   Nitpicker L4 specific startup and configuration
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

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"


/*** SEND TIMEOUT 0, RECEIVE TIMEOUT 33ms ***/
#define REQUEST_TIMEOUT  l4_ipc_timeout(0, 1, 515, 6)


CORBA_Object_base nit_tid, *nit_server;


/*** COMPARE IF TWO CORBA OBJECTS ARE THE SAME ***
 *
 * We compare if both objects are of the same tasks.
 */
int dice_obj_eq(CORBA_Object o1, CORBA_Object o2) {
	if (!o1 && !o2) return 1;
	if (!o1 || !o2) return 0;
	if (l4_task_equal(*o1, *o2)) return 1;
	return 0;
}


/*** CUSTOM IMPLEMENTATION FOR NITPICKER SERVER LOOP ***
 *
 * We need the custom implementation to check for input events
 * after each IPC call.
 *
 * The other parts of the function are copied from the DICE-
 * generated stub code.
 */
static void custom_nitpicker_server_loop(void* dice_server_param) {
	CORBA_Object_base __dice_corba_obj = INVALID_CORBA_OBJECT_BASE;
	nitpicker_msg_buffer_t _dice_msg_buffer;
	CORBA_Server_Environment *_dice_corba_env;
	CORBA_Object _dice_corba_obj;
	short _dice_reply;
	long _dice_opcode;
	l4_msgtag_t _dice_tag = l4_msgtag(0,0,0,0);
	_dice_corba_obj = &__dice_corba_obj;
	if (dice_server_param)
		_dice_corba_env = (CORBA_Server_Environment*)dice_server_param;
	else
	{
		_dice_corba_env = (CORBA_Server_Environment*)_dice_alloca(sizeof(CORBA_Server_Environment));
		*_dice_corba_env = (CORBA_Server_Environment)dice_default_server_environment;
	}
	_dice_msg_buffer._word._dice_size_dope = L4_IPC_DOPE( sizeof(_dice_msg_buffer)/sizeof(long)-3, 0);
	_dice_msg_buffer._word._dice_rcv_fpage = _dice_corba_env->rcv_fpage;
	_dice_opcode = nitpicker_wait_any (_dice_corba_obj,
	                                   &_dice_tag,
	                                   &_dice_msg_buffer,
	                                   _dice_corba_env);
	while (1) {
		_dice_reply = nitpicker_dispatch (_dice_corba_obj,
		                                  _dice_opcode,
		                                  &_dice_msg_buffer,
		                                  _dice_corba_env);
		if (_dice_reply == DICE_REPLY)
			_dice_opcode = nitpicker_reply_and_wait (_dice_corba_obj,
			                                         &_dice_tag,
			                                         &_dice_msg_buffer,
			                                         _dice_corba_env);
		else
			_dice_opcode = nitpicker_wait_any (_dice_corba_obj,
			                                   &_dice_tag,
			                                   &_dice_msg_buffer,
			                                   _dice_corba_env);
		/* handle user input */
		foreach_input_event(handle_normal_input);
	}
}


/*** START PROCESSING CLIENT REQUESTS ***/
int start_server(void) {
	CORBA_Server_Environment env = dice_default_server_environment;

	TRY(names_register("Nitpicker") == 0, "Cannot register at names");

	nit_tid = l4_myself();
	nit_server = &nit_tid;
	env.timeout = REQUEST_TIMEOUT;
	custom_nitpicker_server_loop(&env);
	return 0;
}
