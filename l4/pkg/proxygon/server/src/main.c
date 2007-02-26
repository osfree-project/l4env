/*
 * \brief   Proxygon main - mapping the con API to DOpE
 * \date    2004-09-30
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

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/l4con/l4con.h>
#include <l4/l4con/l4con-server.h>
#include <l4/names/libnames.h>
#include <l4/dope/dopelib.h>
#include <l4/util/getopt.h>

/*** LOCAL INCLUDES ***/
#include "if.h"
#include "events.h"


char LOG_tag[] = "proxygon";                /* tag that is used for log output */
l4_ssize_t l4libc_heapsize = 1*1024*1024;   /* heap to waste                   */
int config_events;                          /* use events mechanism or not     */

int app_id;  /* DOpE application id */


/*** PROXYGON MAIN FUNCTION ***/
int main(int argc, char **argv) {
	int opt;

	/* process command line options */
	static struct option long_options[] = {
		{"events",        0, 0, 'e'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "if", long_options, NULL)) != -1) {;
		switch (opt) {
			case 'e':
				config_events = 1;
				printf("Using events mechanism\n");
				break;
			default:
				printf("Warning: unknown option!\n");
		}
	}

	/* init DOpE application */
	if (dope_init()) return -1;

	app_id = dope_init_app("Proxygon");

	/* start thread that handles the if serverloop */
	if (start_if_server()) {
		printf("Error: could not start if server.\n");
		exit(-1);
	}

	/* start events server thread */
	if (config_events && start_events_server())
		printf("Warning: could not start events server.\n");

	/* process DOpE events */
	dope_eventloop(app_id);
	return 0;
}
