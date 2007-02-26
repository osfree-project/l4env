/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "ifconfig.h"

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage:\n  ifconfig <ifname> <inaddr> <inmask>\n");
		return 0;
	}

	ifconfig(argv[1], argv[2], argv[3]);
	return 0;
}
