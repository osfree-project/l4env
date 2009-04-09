/*
 * \brief  DDEUSB based USB HID driver
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */





#include <l4/dde/linux26/dde26.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/usb/libddeusb.h>

#include <l4/sys/ipc.h>
#include <l4/input/libinput.h>
#include <l4/names/libnames.h>




#include "internal.h"

/* FIXME proxy name and protocol hard-coded here and in pkg/input. */
static l4_threadid_t l4i_proxy = L4_INVALID_ID;


void callback(struct l4input *ev)
{
	if (l4_is_invalid_id(l4i_proxy)) return;

	l4_umword_t d0, d1;
	l4_msgdope_t res;

	d0 = ev->type | (ev->code << 16);
	d1 = ev->value;

	int error;
	static unsigned alex_drop_counter = 0;

	error = l4_ipc_send(l4i_proxy, 0, d0, d1, l4_ipc_timeout(1000,0,0,1), &res);
	if (error) alex_drop_counter++;
}


int main(int argc, char **argv)
{
	LOG("DDEUSB client side initiaizing:");
	l4dde26_init();
	l4dde26_kmalloc_init();
	l4dde26_process_init();
	l4dde26_init_timers();
	l4dde26_softirq_init();
	l4dde26_do_initcalls();
	
	names_waitfor_name("l4i_proxy", &l4i_proxy, 20000);
	
	if (l4_is_invalid_id(l4i_proxy))
	  LOG("Did not find l4i_proxy");
	else
	  LOG("found l4i_proxy at %x.%x", l4i_proxy.id.task,
	      l4i_proxy.id.lthread);
	
	l4input_internal_l4evdev_init(&callback);
	
	l4_sleep_forever();
}
