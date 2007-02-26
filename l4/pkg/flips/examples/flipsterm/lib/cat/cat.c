/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <sys/socket.h>

/*** L4 INCLUDES ***/
#include <l4/flips/libflips.h>

/*** LOCAL INCLUDES ***/
#include "cat.h"

#define BUF_SIZE 2048

int main(int argc,char **argv) {
	static char buf[BUF_SIZE];
	int i;
	int num;
	if (argc<2) {
		printf("usage: cat <filename>...\n");
		return 0;
	}
	for (i=1;i<argc;i++) {
		num = flips_proc_read(argv[i], buf, 0, BUF_SIZE-2);
		if (num >= 0) {
			buf[num] = 0;
			buf[BUF_SIZE-1] = 0;
			printf("%s\n",buf);
		} else {
			printf("Error while reading file %s.\n",argv[i]);
			return -1;
		}
	}
	return 0;
}
