/*
 * \brief   Dummylib for Petze on Linux
 * \date    2003-07-29
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

#include <stdlib.h>
#include <stdio.h>

void *petz_malloc(char *poolname, unsigned int size);
void *petz_malloc(char *poolname, unsigned int size) {
	printf("malloc called with poolname=%s, size=%d\n", poolname, size);
	return malloc(size);
}
