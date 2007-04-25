/*!
 * \file   libc_backends_l4env/lib/buddy_slab_mem/malloc.c
 * \brief  malloc/free implementation
 *
 * \date   08/18/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <l4/dm_generic/types.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/crtx/crt0.h>
#include <l4/crtx/ctor.h>
#include <l4/slab/slab.h>
#include <l4/semaphore/semaphore.h>
#include <l4/util/bitops.h>
#include "buddy.h"
#include "config.h"

static l4dm_dataspace_t malloc_ds = L4DM_INVALID_DATASPACE_INITIALIZER;
static unsigned char*baseaddr;
static unsigned char*owner_map;
static l4buddy_root *buddy;
static l4semaphore_t buddy_lock = L4SEMAPHORE_UNLOCKED;
static l4semaphore_t slab_lock  = L4SEMAPHORE_UNLOCKED;

#define MIN_SLAB_SIZE_LD 4				// 16
#define MAX_SLAB_SIZE_LD (L4BUDDY_BUDDY_SHIFT)		// 1024
static l4slab_cache_t slabs[MAX_SLAB_SIZE_LD-MIN_SLAB_SIZE_LD+1];

l4_ssize_t l4libc_heapsize __attribute__((weak)) = L4LIBC_HEAPSIZE;
void l4libc_init_mem(void);

extern inline void set_owner_slab(void *addr);
extern inline void set_owner_buddy(void *addr);
extern inline int  get_owner_slab(void *addr);
extern inline l4slab_cache_t* get_slab(size_t size);
static void* slab_grow(l4slab_cache_t *slab, void**data);
static void  slab_release(l4slab_cache_t *slab, void*addr, void*data);

static int init_malloc(size_t size){
    unsigned size_owner_map;
    int err, i;

    baseaddr = l4dm_mem_ds_allocate_named(size, L4RM_LOG2_ALIGNED, "libc heap",
					  &malloc_ds);
    if(baseaddr == NULL){
	LOG_Error("l4dm_mem_ds_allocate_named() failed");
	return -L4_ENOMEM;
    }

    /* map heap */
    if((err = l4dm_map(baseaddr, size, L4DM_RW))<0){
	LOG_Error("l4dm_map(): %s", l4env_errstr(err));
	goto e_free;
    }

    /* dm_phys now delivers zero'ed memory */
    /*
    memset(baseaddr, 0, size);
    */

    /* allocate the small/big-map at the end. We have 1 bit per page. */
    size_owner_map = (size+L4_PAGESIZE*8-1) >>(L4_PAGESHIFT+3);
    size-=size_owner_map;
    owner_map = (unsigned char*)baseaddr + size;
    memset(owner_map, 0, size_owner_map);

    /* initialize the buddy system */
    if((buddy = l4buddy_create(baseaddr, size))==0){
	LOG_Error("l4buddy_create(%p, %ld) failed", baseaddr, (l4_addr_t)size);
	goto e_free;
    }

    LOGd(LOG_MALLOC_INIT, "Allocated buddy at %p", buddy);
    LOGd(LOG_MALLOC_INIT,
	 "base_addr = %p, size_owner_map=%d, owner_map=%p",
	 baseaddr, size_owner_map, owner_map);
    LOGd(LOG_MALLOC_INIT,
	 "initializing slabs %d-%d", MIN_SLAB_SIZE_LD, MAX_SLAB_SIZE_LD);
    /* initialize the slabs */
    for(i=MIN_SLAB_SIZE_LD; i<=MAX_SLAB_SIZE_LD; i++){
	LOGd(LOG_MALLOC_INIT, "initializing slab %d: size=%d, keep=%d",
	     i-MIN_SLAB_SIZE_LD, 1<<i, 2);
	err = l4slab_cache_init(&slabs[i-MIN_SLAB_SIZE_LD],
				1<<i,
				2,
				slab_grow, slab_release);
	if(err){
	    LOG_Error("l4slab_cache_init(): %s", l4env_errstr(err));
	    goto e_free;
	}
    }
    return 0;

  e_free:
    l4dm_mem_release(baseaddr);
    return err;
}

void l4libc_init_mem(void)
{
    int ret;

    LOGdL(LOG_MALLOC_INIT, "Called");
    if (l4libc_heapsize)
    {
	ret = init_malloc(l4libc_heapsize);
        if (ret != 0)
        {
            // Something went really wrong. As this is a constructor
            // we must shutdown
            _exit(1);
        }
    }
}
L4C_CTOR(l4libc_init_mem, 1100);

extern inline void set_owner_slab(void *addr){
    unsigned off;

    off = ((unsigned char*)addr-baseaddr)>>L4_PAGESHIFT;
    owner_map[off>>3] |= 1<<(off & 7);
}
extern inline void set_owner_buddy(void *addr){
    unsigned off;

    off = ((unsigned char*)addr-baseaddr)>>L4_PAGESHIFT;
    owner_map[off>>3] &= ~(1<<(off & 7));
}
extern inline int get_owner_slab(void *addr){
    unsigned off;

    off = ((unsigned char*)addr-baseaddr)>>L4_PAGESHIFT;
    return owner_map[off>>3] & (1<<(off & 7));
}

extern inline l4slab_cache_t* get_slab(size_t size){
    int size_ld = l4util_log2(size);

    if(size > (1<<size_ld)) size_ld++;

    if(size_ld<=MIN_SLAB_SIZE_LD) return &slabs[0];
    if(size_ld>MAX_SLAB_SIZE_LD) return 0;
    return &slabs[size_ld-MIN_SLAB_SIZE_LD];
}

static void* slab_grow(l4slab_cache_t *slab, void**data){
    void *addr;

    LOGd_Enter(LOG_MALLOC_SLAB, "getting new page...");
    addr = l4buddy_alloc(buddy, slab->slab_size);
    if(addr){
	set_owner_slab(addr);
    }
    return addr;
}
static void  slab_release(l4slab_cache_t *slab, void*addr, void*data){
    LOGd_Enter(LOG_MALLOC_SLAB, "releasing page at %p", addr);
    l4buddy_free(buddy, addr);
    set_owner_buddy(addr);
}

void* malloc(size_t size){
    LOGd_Enter(LOG_MALLOC_MALLOC, "size=%ld", (l4_addr_t)size);

    if(size>=L4BUDDY_BUDDY_SIZE-sizeof(l4slab_cache_t*)){
	void *addr;
	l4semaphore_down(&buddy_lock);
	addr = l4buddy_alloc(buddy, size);
	l4semaphore_up(&buddy_lock);
	return addr;
    } else {
	l4slab_cache_t *slab;
	void *addr;

	slab = get_slab(size+sizeof(l4slab_cache_t*));
	if(!slab){
	    LOG_Error("Could not get a slab for size %ld", (l4_addr_t)size);
	    return 0;
	}
	l4semaphore_down(&slab_lock);
	addr = l4slab_alloc(slab);
	l4semaphore_up(&slab_lock);
	if(addr){
	    *((l4slab_cache_t **)addr)=slab;
	    LOGd(LOG_MALLOC_MALLOC, "allocated at slab %p: %p", slab, addr);
	    return (void*)((l4slab_cache_t **)addr + 1);
	}
	return 0;
    }
}

void free(void *addr){
    LOGd_Enter(LOG_MALLOC_FREE, "addr=%p", addr);

    if(!addr) return;

    if(get_owner_slab(addr)){
	l4slab_cache_t *slab;

	addr = (void*)((l4slab_cache_t **)addr - 1);
	slab = *((l4slab_cache_t **)addr);
	LOGd(LOG_MALLOC_FREE, "freeing at slab %p: %p", slab, addr);
	l4slab_free(slab, addr);
    } else {
	LOGd(LOG_MALLOC_FREE, "freeing ad buddy");
	l4buddy_free(buddy, addr);
    }
}

void *calloc(size_t nmemb, size_t size) {
    void *addr;
    size_t s=size*nmemb;

    if (nmemb && s/nmemb!=size) {
	errno = ENOMEM;  /* it is not guaranteed that __set_errno() is
			    globally visible */
	return 0;
    }
    addr = malloc(s);
    if(addr) memset(addr, 0, s);
    return addr;
}

void *realloc(void*addr, size_t size){
    void *newaddr;

    if(size){
	newaddr = malloc(size);
	if(!newaddr) return 0;
    } else {
	newaddr=0;
    }
    if(addr){
	if(newaddr) memcpy(newaddr, addr, size);
	free(addr);
    }
    return newaddr;
}
