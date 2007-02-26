/*
 * \brief   Linux specific DOpE VScreen library
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

#include <vscreen.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_VSCREENS 32

struct vscr {
	char *name;
} vscreens[MAX_VSCREENS];


/*** ALLOCATE NEW VSCR ID ***/
static int get_new_index(void) {
	int i;
	
	/* find free vscr id */
	for (i=0;i<MAX_VSCREENS;i++) {
		if (!vscreens[i].name) break;
	}

	if (i>=MAX_VSCREENS) return -1;
	return i;
}


/*** CHECK IF A GIVEN VSCR ID IS VALID ***/
static int valid_index(int id) {

	if ((id<0) || (id>=MAX_VSCREENS)) return 0;
	if (!vscreens[id].name) return 0;
	return 1;
}


static void release_index(int i) {
	
	if (!valid_index(i)) return;
	vscreens[i].name = NULL;
}


void *vscr_connect_server(char *ident) {
	int i;
	
	if ((i = get_new_index()) <0) return NULL;
	vscreens[i].name = ident;
	return (void *)i+1;
}


void vscr_release_server_id(void *id) {
	int i = ((int)id) - 1;
	
	if (!valid_index(i)) return;
	
	/* !!! unmap vscreen memory !!! */
	release_index(i);
}


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


void *vscr_map_smb(char *smb_ident) {
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


void vscr_server_waitsync(void *id) { }

void vscr_server_refresh(void *id, int x, int y, int w, int h) { }
