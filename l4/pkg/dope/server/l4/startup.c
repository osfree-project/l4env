/*
 * \brief	DOpE L4 specific startup code
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

void native_startup(int, char **);

#include <l4/util/getopt.h>
#include <l4/log/l4log.h>
#include <stdio.h>

/*** screen.c ***/
extern int screen_use_l4io;

static void do_args(int argc, char **argv)
{
	char c;

	static struct option long_options[] =
	{
		{"l4io", 0, 0, 'i'},
		{0, 0, 0, 0}
	};

	/* read command line arguments */
	while (1)
	{
		c = getopt_long(argc, argv, "i", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'i':
				screen_use_l4io = 1;
				printf("DOpE(init): using L4 IO server\n");
				break;
			default:
				/* ignore */
		}
	}
}

void native_startup(int argc, char **argv) {
	do_args(argc, argv);

	LOG_init("l4dope");

}
