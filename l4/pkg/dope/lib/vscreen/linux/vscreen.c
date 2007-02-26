#include <vscreen.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_VSCREENS 32

struct vscr_struct {
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


void *vscr_get_server_id(char *ident) {
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


void *vscr_get_fb(char *smb_ident) {
	int fh;
	void *addr;
	
	fh = open(smb_ident+21, O_RDWR);
	addr = mmap(NULL, hex2u32(smb_ident+7), PROT_READ | PROT_WRITE,
	            MAP_SHARED, fh, 0);
	printf("libVScreen(get_fb): mmap file %s to addr 0x%x\n", smb_ident+21,
	                                                          (int)addr);
	return addr;
}


void vscr_server_waitsync(void *id) { }
