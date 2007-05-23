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
#include <l4/sigma0/kip.h>
#include <l4/env/errno.h>
#ifndef ARCH_arm
#include <l4/util/rdtsc.h>  /* XXX x86 specific */
#endif

/* C */
#include <stdio.h>

/* local */
#include <l4/input/libinput.h>

#include "internal.h"

/** Backend operations */
struct l4input_ops *ops;


int l4input_ispending()
{
	if (ops->ispending)
		return ops->ispending();
	else
		return 0;
}

int l4input_flush(void *buf, int count)
{
	if (ops->flush)
		return ops->flush(buf, count);
	else
		return 0;
}

int l4input_pcspkr(int tone)
{
	if (ops->pcspkr)
		return ops->pcspkr(tone);
	else
		return -L4_EINVAL;
}

/* Okay ...

   We have to initialize requested devices only here because there is only one
   general event device.

   After discussions with Norman I also think one event device is fine. Maybe
   we could add device ids to the event struct later and inject UPDATE events
   in the case of hotplugging.
*/

/** L4INPUT LIBRARY INITIALIZATION **/
int l4input_init(int prio, void (*handler)(struct l4input *))
{
	if (!l4sigma0_kip_kernel_is_ux()) {
		printf("L4INPUT native mode activated\n");

		/* for usleep */
#ifndef ARCH_arm
		l4_calibrate_tsc();
#endif

		/* lib state */
		l4input_internal_jiffies_init();
		l4input_internal_irq_init(prio);
		l4input_internal_wait_init();

		printf("L4INPUT:                !!! W A R N I N G !!!\n"
		       "L4INPUT:  Please, do not use Fiasco's \"-esc\" with L4INPUT.\n"
		       "L4INPUT:                !!! W A R N I N G !!!\n");

		if (handler)
			printf("L4INPUT: Registered %p for callbacks.\n", handler);

		int error;
		if ((error=l4input_internal_input_init()) ||
#ifndef ARCH_arm
		    (error=l4input_internal_i8042_init()) ||
		    (error=l4input_internal_psmouse_init()) ||
#else
		    (error=l4input_internal_amba_kmi_init()) ||
#endif
		    (error=l4input_internal_atkbd_init()) ||
#ifndef ARCH_arm
		    (error=l4input_internal_pcspkr_init()) ||
#endif
		    (error=l4input_internal_proxy_init(prio)))
			return error;

		if (!(ops = l4input_internal_l4evdev_init(handler))) {
			printf("L4INPUT: evdev initialization failed\n");
			return -L4_EUNKNOWN;
		}

	} else {
		printf("L4INPUT Fiasco-UX mode activated\n");

		l4input_internal_irq_init(prio);

#ifdef ARCH_x86
		if (!(ops = l4input_internal_ux_init(handler))) {
			printf("L4INPUT: Fiasco-UX H/W initialization failed\n");
			return -L4_EUNKNOWN;
		}
#endif
	}

	return 0;
}
