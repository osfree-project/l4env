/*
 * \brief   Process DOpE commands from stdin
 * \date    2004-10-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This program connects to DOpE, reads commands from stdin and
 * sends them to DOpE. It is meant to be used for debugging
 * purposes only.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERIC INCLUDES ***/
#include <stdio.h>
#include <string.h>

/*** DOpE INCLUDES ***/
#include <dopelib.h>


int main(int argc,char **argv) {
	char buf[80];
	char res[80];
	long app_id;

	dope_init();
	app_id = dope_init_app("DOpE stdin");

	/* read commands from stdin and sent them to DOpE */
	while (fgets(buf, sizeof(buf), stdin)) {
		int i = strlen(buf) - 1;
		if (i < 0) i = 0;
		buf[i] = 0;

		/* ignore comments and empty lines */
		if ((i == 0) || (buf[0] == '#')) continue;

		/* execute DOpE command */
		dope_req(app_id, res, sizeof(res), buf);
		if (res[0]) printf("%s\n", res);
	}

	return 0;
}
