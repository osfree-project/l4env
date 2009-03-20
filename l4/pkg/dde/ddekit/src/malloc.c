/*
 * \brief   Simple allocator implementation
 * \author  Christian Helmuth
 * \date    2006-10-30
 *
 * This simple allocator provides malloc() and free() using dm_mem dataspaces
 * as backing store. The actual list-based allocator implementation is from
 * l4util resp. Fiasco.
 *
 * For large allocations and slab-based OS-specific allocators
 * ddekit_large_malloc and ddekit_slab_*() should be used. The blocks
 * allocated via this allocator CANNOT be used for DMA or other device
 * operations, i.e., there exists no virt->phys mapping.
 *
 * FIXME check thread-safety and add locks where appropriate
 */

#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/printf.h>
#include <l4/dde/ddekit/panic.h>

#include <l4/sys/consts.h>
#include <l4/util/list_alloc.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/lock/lock.h>

/* configuration */
#define ALLOC_SIZE     (4 * L4_PAGESIZE)

/* malloc pool is a list allocator */
static l4la_free_t *malloc_pool;
static l4lock_t     malloc_lock = L4LOCK_UNLOCKED;


/**
 * Allocate memory block via simple allocator
 *
 * \param size  block size
 * \return pointer to new memory block
 *
 * The blocks allocated via this allocator CANNOT be used for DMA or other
 * device operations, i.e., there exists no virt->phys mapping.
 *
 * Each chunk stores its size in the first word for free() to work.
 */
void *ddekit_simple_malloc(unsigned size)
{
	l4lock_lock(&malloc_lock);
	/* we store chunk size in the first word of the chunk */
	size += sizeof(unsigned);

	/* try to allocate */
	unsigned *p = l4la_alloc(&malloc_pool, size, 0);

	/* fill pool if allocation fails */
	if (!p) {
		/* size of allocated dataspace is at least ALLOC_SIZE */
		unsigned ds_size = l4_round_page(size);
		ds_size = (ds_size > ALLOC_SIZE) ? ds_size : ALLOC_SIZE;

		void *res = l4dm_mem_allocate_named(ds_size, L4RM_MAP, "ddekit malloc");
		if (!res) 
			p = NULL;
		else
		{
			l4la_free(&malloc_pool, res, ds_size);
			p = l4la_alloc(&malloc_pool, size, 0);
		}
	}

	/* store chunk size */
	if (p) {
		*p = size;
		p++;
	}

	l4lock_unlock(&malloc_lock);
	return p;
}


/**
 * Free memory block via simple allocator
 *
 * \param p  pointer to memory block
 */
void ddekit_simple_free(void *p)
{
	l4lock_lock(&malloc_lock);
	unsigned *chunk = (unsigned *)p - 1;
	if (p)
		l4la_free(&malloc_pool, chunk, *chunk);
	l4lock_unlock(&malloc_lock);
}
