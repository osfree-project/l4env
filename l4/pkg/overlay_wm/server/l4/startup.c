/*
 * \brief   OverlayWM - native startup
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** L4 INCLUDES ***/
#include <l4/util/getopt.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#include "startup.h"
#include "main.h"

l4_ssize_t l4libc_heapsize = 6*1024*1024;

void native_startup(int argc, char **argv) {
	char c;

	static struct option long_options[] = {
		{"scale", 0, 0, 's'},
		{0, 0, 0, 0}
	};

	/* read command line arguments */
	while (1) {
		c = getopt_long(argc, argv, "if", long_options, NULL);
		if (c == -1) break;
		switch (c) {
			case 's':
				config_scale = 1;
				break;
			default:;
		}
	}
}
