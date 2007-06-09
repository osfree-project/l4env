/*
 * \brief   Nitpicker client library
 * \date    2007-06-05
 * \author  Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2007 Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nitpicker package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nitpicker/nitpicker.h>
#include <l4/nitpicker/nitevent-server.h>


static nitpicker_event_func _nitpicker_cb_func[128];   /* event callbacks */


void nitpicker_mainloop(nitpicker_event_func cb) {
	l4_threadid_t myself = l4_myself();

	_nitpicker_cb_func[myself.id.lthread] = cb;

	nitevent_server_loop(NULL);
}


void nitevent_event_component(CORBA_Object _dice_corba_obj,
                              unsigned long token,
                              int type,
                              int keycode,
                              int rx,
                              int ry,
                              int ax,
                              int ay,
                              CORBA_Server_Environment *_dice_corba_env) {
	l4_threadid_t myself = l4_myself();

	if (_nitpicker_cb_func[myself.id.lthread])
		_nitpicker_cb_func[myself.id.lthread](token,
		                                      type, keycode, rx, ry, ax, ay);
}
