/*
 * \brief   DOpE timer module
 * \date    2002-11-13
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

#include "dopestd.h"

#include <l4/sigma0/kip.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#ifndef ARCH_arm
#include <l4/util/thread_time.h>
#endif

#include "timer.h"

static l4_kernel_info_t *kip;

int init_timer(struct dope_services *d);


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/

/*** RETURN CURRENT SYSTEM TIME COUNTER IN MICROSECONDS ***/
static u32 get_time(void) {
#ifdef ARCH_arm
	return (u32)kip->clock;
#else
	return l4_tsc_to_us(l4util_thread_time(kip));
#endif
}


/*** RETURN DIFFERENCE BETWEEN TWO TIMES ***/
static u32 get_diff(u32 time1, u32 time2) {

	/* overflow check */
	if (time1>time2) {
		time1 -= time2;
		return (u32)0xffffffff - time1;
	}
	return time2-time1;
}


/*** WAIT THE SPECIFIED NUMBER OF MICROSECONDS ***/
static void usleep(u32 num_usec) {
	l4thread_usleep(num_usec);
}

/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct timer_services services = {
	get_time,
	get_diff,
	usleep,
};


/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_timer(struct dope_services *d) {
	kip = l4sigma0_kip();
	if (!kip) Panic("kip map failed");

#ifndef ARCH_arm
	l4_calibrate_tsc();
#endif

	d->register_module("Timer 1.0", &services);
	return 1;
}
