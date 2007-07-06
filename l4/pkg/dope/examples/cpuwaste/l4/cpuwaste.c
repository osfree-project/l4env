/*
 * \brief   DOpE cpu waste demo
 * \date    2005-10-03
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This program burns a configurable percentage of the
 * available CPU time.
 */

/*
 * Copyright (C) 2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** DOPE INCLUDES ***/
#include <l4/dope/dopelib.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/util/kip.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/thread/thread.h>

char LOG_tag[9] = "CPUwaste";
l4_ssize_t l4libc_heapsize = 500*1024;


static long app_id;

static int waste_level = 0;  /* cpu waste level 0..99 */ 

static void cpu_waste_thread(void *arg) {
	
	while (1) {

		/* wait 100 - wastelevel miliseconds */
		l4_usleep(1000*(100 - waste_level));

		/* waste wastelevel miliseconds */
		l4_busy_wait_us(1000*waste_level);

	}
}


static void scale_callback(dope_event *e, void *arg) {
	static char buf[16];
	dope_req(app_id, buf, 16, "sc.value");
	waste_level = atol(buf);
	printf("setting waste level to %d percent\n", waste_level);
}


int main(int argc,char **argv) {

	if (!l4_calibrate_tsc()) printf("l4_calibrate_tsc: fucked up\n");

	if (dope_init()) return -1;

	app_id = dope_init_app("CPU Waste");

	dope_cmd(app_id, "sc = new Scale(-from 0 -to 99 -value 0)");
	dope_cmd(app_id, "win = new Window(-content sc)");
	dope_cmd(app_id, "win.open()");
	dope_bind(app_id, "sc", "change", scale_callback, NULL);

	/* start cpu waste thread */
	l4thread_create(cpu_waste_thread, NULL, L4THREAD_CREATE_ASYNC);

	dope_eventloop(app_id);
	return 0;
}
