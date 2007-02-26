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

#include <l4/dm_phys/dm_phys.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>

static struct memory_services *mem;

struct shared_memory_struct {
	l4dm_dataspace_t  ds;
	s32               size;
	void             *addr;
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
	new->addr = l4dm_mem_ds_allocate(size, 
	                                 L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED,
	                                 &new->ds);
	new->size = size;
	printf("SharedMem(alloc): hl.low=%x, lh.high=%x, id=%x\n",
					new->ds.manager.lh.low,
					new->ds.manager.lh.high,
					new->ds.id);
	return new;
}


/*** FREE SHARED MEMORY BLOCK ***/
static void free(SHAREDMEM *sm) {
	if (!sm) return;
	l4dm_mem_release(sm->addr);
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
	sprintf(dst,"t_id=0x%08X,%08X ds_id=0x%08x size=0x%08x",
	        sm->ds.manager.lh.low, 
	        sm->ds.manager.lh.high, 
	        sm->ds.id,
			(int)sm->size);
}


/*** SHARE MEMORY BLOCK TO ANOTHER THREAD ***/
static void share(SHAREDMEM *sm, THREAD *dst_thread) {
	l4_threadid_t *tid;
	if (!sm) return;

	DOPEDEBUG(printf("VScreen(map): check_rights = %d\n",
		l4dm_check_rights(&sm->ds,L4DM_RW)
	));
			
	tid = (l4_threadid_t *)dst_thread;
	l4dm_share(&sm->ds, *tid, L4DM_RW);
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

