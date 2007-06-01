/*
 * \brief   DOpE shared memory management module
 * \date    2002-02-04
 * \author  Norman Feske <no@atari.org>
 *
 * This component provides an abstraction for handling
 * shared memory.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>

#include "dopestd.h"
#include "module.h"
#include "thread.h"
#include "sharedmem.h"

#include <l4/dm_phys/dm_phys.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>


struct shared_memory {
	l4dm_dataspace_t  ds;
	s32               size;
	void             *addr;
};

struct thread {
	l4_threadid_t tid;
};

int init_sharedmem(struct dope_services *d);


/*************************
 *** SERVICE FUNCTIONS ***
 *************************/


/*** ALLOCATE SHARED MEMORY BLOCK OF SPECIFIED SIZE ***/
static SHAREDMEM *shm_alloc(long size) {
	SHAREDMEM *new = malloc(sizeof(SHAREDMEM));
	if (!new) {
		ERROR(printf("SharedMemory(alloc): out of memory.\n"));
		return NULL;
	}
	new->addr = l4dm_mem_ds_allocate_named(size,
	                                       L4RM_LOG2_ALIGNED | L4RM_MAP,
	                                       "DOpE shm",
	                                       &new->ds);
	new->size = size;
	printf("SharedMem(alloc): hl.raw=%lx, id=%x, size=%lx\n",
		(unsigned long)new->ds.manager.raw,
		new->ds.id,
		(int long)size);
	return new;
}


/*** FREE SHARED MEMORY BLOCK ***/
static void shm_destroy(SHAREDMEM *sm) {
	if (!sm) return;
	l4dm_mem_release(sm->addr);
	free(sm);
}


/*** RETURN THE ADRESS OF THE SHARED MEMORY BLOCK ***/
static void *shm_get_adr(SHAREDMEM *sm) {
	if (!sm) return NULL;
	printf("SharedMem(get_adr): address = %p\n", sm->addr);
	return sm->addr;
}


/*** GENERATE A GLOBAL IDENTIFIER FOR THE SPECIFIED SHARED MEMORY BLOCK ***/
static void shm_get_ident(SHAREDMEM *sm, u8 *dst) {
	if (!sm) return;
	sprintf(dst, "t_id=0x%08lX ds_id=0x%08x size=0x%08lx",
	        (unsigned long)sm->ds.manager.raw,
	        sm->ds.id,
	        (long)sm->size);
}


/*** SHARE MEMORY BLOCK TO ANOTHER THREAD ***/
static s32 shm_share(SHAREDMEM *sm, THREAD *dst_thread) {
	int res;
	if (!sm) return -1;

	if ((res = l4dm_check_rights(&sm->ds, L4DM_RW)) != 0)
		return -1;

	INFO(printf("SharedMem(share): share to %x.%x\n",
	            dst_thread->tid.id.task, dst_thread->tid.id.lthread));

	if ((res = l4dm_share(&sm->ds, dst_thread->tid, L4DM_RW)) != 0)
		return -1;
	return 0;
}


/****************************************
 *** SERVICE STRUCTURE OF THIS MODULE ***
 ****************************************/

static struct sharedmem_services sharedmem = {
	shm_alloc,
	shm_destroy,
	shm_get_adr,
	shm_get_ident,
	shm_share,
};



/**************************
 *** MODULE ENTRY POINT ***
 **************************/

int init_sharedmem(struct dope_services *d) {
	d->register_module("SharedMemory 1.0", &sharedmem);
	return 1;
}
