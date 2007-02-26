/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/init.c
 * \brief  L4INPUT: Initialization
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/util/rdtsc.h>  /* XXX x86 specific */

/* C */
#include <stdio.h>

/* local */
#include <l4/input/libinput.h>

#include "internal.h"

/* Okay ...

   We have to initialize requested devices only here because there is only one
   general event device.

   After discussions with Norman I also think one event device is fine. Maybe
   we could add device ids to the event struct later and inject UPDATE events
   in the case of hotplugging.
*/

/** L4INPUT LIBRARY INITIALIZATION **/
int l4input_init(int omega0, int prio, void (*handler)(struct l4input *))
{
	int error;

	/* for usleep */
	l4_calibrate_tsc();

	/* lib state */
	l4input_internal_jiffies_init();
	l4input_internal_irq_init(omega0, prio);
	l4input_internal_wait_init();

	/* XXX */
	printf("L4INPUT:                !!! W A R N I N G !!!\n"
	       "L4INPUT:  Please, do not use Fiasco's \"-esc\" with L4INPUT.\n"
	       "L4INPUT:                !!! W A R N I N G !!!\n");

	if (omega0)
		printf("L4INPUT: Using omega0 for IRQs.\n");
	if (handler)
		printf("L4INPUT: Registered %p for callbacks.\n", handler);

	if ((error=l4input_internal_input_init()) ||
	    (error=l4input_internal_i8042_init()) ||
	    (error=l4input_internal_psmouse_init()) ||
	    (error=l4input_internal_atkbd_init()) ||
	    (error=l4input_internal_pcspkr_init()) ||
	    (error=l4input_internal_proxy_init(prio)))
		return error;

	if ((error=l4input_internal_l4evdev_init(handler))) {
		printf("L4INPUT: evdev initialization failed (%d)", error);
		return error;
	}

	return 0;
}
