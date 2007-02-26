/*
 * \brief	DOpE shared memory management module
 * \date	2002-02-04
 * \author	Norman Feske <no@atari.org>
 *
 * This component provides an abstraction for handling
 * shared memory.
 */

#include <stdio.h>

#include "dope-config.h"
#include "module.h"
#include "memory.h"
#include "thread.h"
#include "sharedmem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static struct memory_services *mem;

static int ident_cnt = 0;

struct shared_memory_struct {
	int   fh;
	s32   size;
	char  fname[64];
	void *addr;
};

int init_sharedmem(struct dope_services *d);


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


/*** ALLOCATE SHARED MEMORY BLOCK OF SPECIFIED SIZE ***/
static SHAREDMEM *alloc(s32 size) {
	SHAREDMEM *new = mem->alloc(sizeof(SHAREDMEM));
	if (!new) {
		ERROR(printf("SharedMemory(alloc): out of memory.\n"));
		return NULL;
	}
	
	/* open file */
	sprintf(new->fname, "/tmp/dopevscr%d", ident_cnt++);
	new->fh = open(new->fname, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
	new->size = size;
	ftruncate(new->fh, new->size);
	new->addr = mmap(NULL, new->size, PROT_READ | PROT_WRITE, 
	                 MAP_SHARED, new->fh, 0);
	printf("SharedMem(alloc): mmap file %s to addr %x\n", new->fname, (int)new->addr);				
	return new;
}


/*** FREE SHARED MEMORY BLOCK ***/
static void free(SHAREDMEM *sm) {
	if (!sm) return;
	munmap(sm->addr, sm->size);
	close(sm->fh);
	mem->free(sm);
}


/*** RETURN THE ADRESS OF THE SHARED MEMORY BLOCK ***/
static void *get_adr(SHAREDMEM *sm) {
	if (!sm) return NULL;
	return sm->addr;
}


/*** GENERATE A GLOBAL IDENTIFIER FOR THE SPECIFIED SHARED MEMORY BLOCK ***/
static void get_ident(SHAREDMEM *sm, u8 *dst) {
	if (!sm) return;
	sprintf(dst, "size=0x%08X file=%s", (int)sm->size, sm->fname);
}


/*** SHARE MEMORY BLOCK TO ANOTHER THREAD ***/
static void share(SHAREDMEM *sm, THREAD *dst_thread) {
	return;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct sharedmem_services sharedmem = { 
	alloc,
	free,
	get_adr,
	get_ident,
	share,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_sharedmem(struct dope_services *d) {

	mem = d->get_module("Memory 1.0");
	d->register_module("SharedMemory 1.0",&sharedmem);
	return 1;
}

