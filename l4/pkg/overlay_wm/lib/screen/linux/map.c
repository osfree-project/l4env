/*
 * \brief   Linux specific functions to handle shared memory (via mmap)
 * \date    2003-08-04
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

/*** LOCAL INCLUDES ***/
#include "map.h"


/*** UTILITY: CONVERT ASCII HEX NUMBER TO AN UNSIGNED 32BIT VALUE ***/
static unsigned long hex2u32(char *s) {
	int i;
	unsigned long result=0;
	for (i=0;i<8;i++,s++) {
		if (!(*s)) return result;
		result = result*16 + (*s & 0xf);
		if (*s > '9') result += 9;
	}
	return result;
}


/*** LINUX: MAP SHARED MEMORY BLOCK INTO LOCAL ADDRESS SPACE ***/
void *ovl_screen_get_framebuffer(char *smb_ident) {
	int fh;
	void *addr;
	
	fh = open(smb_ident+21, O_RDWR);
	addr = mmap(NULL, hex2u32(smb_ident+7), PROT_READ | PROT_WRITE,
	            MAP_SHARED, fh, 0);
	printf("libVScreen(get_fb): mmap file %s to addr 0x%x\n", smb_ident+21,
	                                                          (int)addr);
	if ((int)addr == -1) return NULL;
	return addr;
}

