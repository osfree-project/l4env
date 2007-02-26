/*
 * \brief   DOpE L4 specific startup code
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

void native_startup(int, char **);

#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/generic_io/libio.h>
#include <stdio.h>

/* name at logserver */
char LOG_tag[] = "l4dope";
l4_ssize_t l4libc_heapsize = 6*1024*1024;

int use_l4io = 0;  /* whether to use L4IO server or not, default no */
l4io_info_t *l4io_page = (l4io_info_t*) 0; /* L4IO info page */

int use_vidfix = 0; /* certain graphic adapter deliver garbage in vesa info */

static int dope_l4io_init(void) {

	if (l4io_init(&l4io_page, L4IO_DRV_INVALID)) {
		Panic("Couldn't connect to L4IO server!");
		return 1;
	}
	return 0;
}


static void do_args(int argc, char **argv)
{
	char c;

	static struct option long_options[] =
	{
		{"l4io",    0, 0, 'i'},
		{"vidfix",  0, 0, 'f'},
		{0, 0, 0, 0}
	};

	/* read command line arguments */
	while (1)
	{
		c = getopt_long(argc, argv, "if", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'i':
				use_l4io = 1;
				printf("DOpE(init): using L4 IO server\n");
				break;
			case 'f':
				use_vidfix = 1;
				printf("DOpE(init): using video fix\n");
				break;
			default:
				printf("DOpE(init): unknown option!\n");
		}
	}
}

void native_startup(int argc, char **argv) {
	do_args(argc, argv);


	if (use_l4io)
	  dope_l4io_init();

}
