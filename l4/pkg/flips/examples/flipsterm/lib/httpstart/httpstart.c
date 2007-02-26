/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "httpstart.h"

static int started_flag;

int main(int argc,char **argv) {
	if (started_flag) {
		printf("HTTP server already started\n");
		return 0;
	}
	printf("triggering start of HTTP server\n");

	if (!names_register("httpstart")) {
		printf("Error: Cannot register \"httpstart\" at nameserver\n");
		return -1;
	}
	started_flag = 1;
	return 0;
}
