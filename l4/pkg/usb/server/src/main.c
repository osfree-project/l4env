/*
 * \brief   DDEUSB Server
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */




#include <l4/names/libnames.h>
#include "usb_common.h" /* USB_NAMESERVER_NAME, */
#include "usb_local.h"

void *CORBA_alloc(unsigned long size)
{
	return kmalloc(size, GFP_KERNEL);

}

void ddeusb_init_client_list(void);

/* CORBA_free() - not that tricky. We only free ptr if it is != NULL. */
void CORBA_free(void *ptr)
{
	if (ptr == NULL)
		return;
	kfree(ptr);
}

l4_threadid_t ddeusb_main_server = L4_INVALID_ID;

int main(int argc, char **argv)
{
	DICE_DECLARE_SERVER_ENV(env);
	env.malloc = (dice_malloc_func)CORBA_alloc;
	env.free   = (dice_free_func)CORBA_free;

	ddeusb_init_client_list();

	LOG("DDE USB CORE initiaizing:");

	LOG("Initializing DDE base system.");
	l4dde26_init();

	LOG("Initializing DDE kmalloc.");
	l4dde26_kmalloc_init();

	LOG("Initializing DDE process.");
	l4dde26_process_init();

	LOG("Initializing DDE timers.");
	l4dde26_init_timers();

	LOG("Initializing DDE softirq.");
	l4dde26_softirq_init();

	LOG("Doing initcalls...");
	l4dde26_do_initcalls();
	ddeusb_main_server = l4_myself();

	if(!names_register(USB_NAMESERVER_NAME))
	{
		LOG_Error("Could not register at nameserver.");
	}

	LOG("Registered at names.");

	LOG("READY.");

	ddeusb_core_server_loop (&env);

	return 0;
}
