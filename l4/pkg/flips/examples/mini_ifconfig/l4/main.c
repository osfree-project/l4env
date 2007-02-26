/*
 * \brief   Mini ifconfig test client
 * \date    2003-08-07
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
	
	int i = 2;
	int last = 1;

	LOG("ifconfig");
	if (argc < 5 || strncmp("-n", argv[1], 2)) {
		LOG("usage:\n ifconfig -n <ifname> <inaddr> <inmask> [gateway] -n ... -n ...");
		LOG("No [standard] configurations have been set.");
		exit(-1);
	}
	else
	{
		while (i <= argc)
		{
			if (i == argc || !strncmp("-n", argv[i], 2))
		       	{
				switch( i - last )
			       	{
				case 4 :
					last = i;
					ifconfig(argv[i - 3], argv[i - 2], argv[i - 1], NULL);
					break;
				case 5 :
					last = i;
					ifconfig(argv[i - 4], argv[i - 3], argv[i - 2], argv[i - 1]);
					break;
				default :
					LOG("invalid configuration last=%d i=%d argc=%d", last, i, argc); 
					exit(-1);
	      			}
			}
			i++;
		}
	}

	/* register at names just that other programs can sync their startup */
	LOG("register at names");
	names_register("ifconfig");
	
	LOG("eternal sleep");
	l4thread_sleep(-1);

	exit(0);
}

