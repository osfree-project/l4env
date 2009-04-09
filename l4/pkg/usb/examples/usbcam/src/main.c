/*
 * \brief   DDEUSB usbcam example
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




void start_gui(void);


int main(int argc, char **argv)
{
	LOG("DDEUSB client side initiaizing:");
	l4dde26_init();
	l4dde26_kmalloc_init();
	l4dde26_process_init();
	l4dde26_init_timers();
	l4dde26_softirq_init();
	l4dde26_do_initcalls();
	
	start_gui();
	
	l4_sleep_forever();
}
