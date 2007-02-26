/*
 * \brief   Mini ifconfig test client
 * \date    2003-08-07
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>
#include <netinet/in.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#include "ifconfig.h"

l4_ssize_t l4libc_heapsize = 500*1024;
char LOG_tag[9] = "minifcfg";

int main(int argc, char **argv) {
	
	LOG("ifconfig");
	if (argc < 4) {
		LOG("usage:\n  ifconfig <ifname> <inaddr> <inmask> [gateway]");
		LOG("setting default value: lo 127.0.0.1 255.0.0.0 none");
		ifconfig("lo", "127.0.0.1", "255.0.0.0", NULL);
	}
	else
	{
	  if (argc < 5)
		ifconfig(argv[1], argv[2], argv[3], NULL);
	  else
		ifconfig(argv[1], argv[2], argv[3], argv[4]);
	}

	/* register at names just that other programs can sync their startup */
	LOG("register at names");
	names_register("ifconfig");
	
	LOG("eternal sleep");
	l4thread_sleep(-1);

	exit(0);
}

