/*
 * \brief   Memory subsystem
 * \author  Thomas Friebel <tf13@os.inf.tu-dresden.de>
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \date    2006-11-03
 *
 * The memory subsystem provides the backing store for DMA-able memory via
 * large malloc and slabs.
 *
 * FIXME check thread-safety and add locks where appropriate
 */

#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/panic.h>
#include <l4/dde/ddekit/pgtab.h>
#include <l4/dde/ddekit/printf.h>

#include <l4/lock/lock.h>
#include <l4/slab/slab.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/atomic.h>
#include <l4/util/util.h>


/****************
 ** Page cache **
 ****************/

/* FIXME revisit two-linked-lists approach - we could get rid of the
 * pcache_free lists, because these descriptors can also be allocated
 * whenever the need arises. (XXX: Check how this hurts performance.)
 */

/*****************************************************************************
  DDEKit maintains a list of free pages that can be used for growing
  existing DDEKit slabs. We distinguish between 2 types of pages: the ones
  that have been allocated from physically contiguous dataspaces and the the
  ones that have not.

  For each type of cache, we maintain two lists: a list of free pages that
  can be used to grow a cache (pcache_used) and a list of free pcache entries
  that can be used to store pages when they get freed (pcache_free). The
  reason for this is that by using these two lists, cache operations can be
  done atomically using cmpxchg.

  The function ddekit_slab_setup_page_cache() is used to tune the number of
  cached pages.
 ******************************************************************************/

/* page cache to minimize allocations from external servers */
struct ddekit_pcache
{
	struct ddekit_pcache *next;
	void *page;
	int contig;
};


/* head of single-linked list containing used page-cache entries, non-contiguous */
static struct ddekit_pcache *pcache_used;
/* head of single-linked list containing free page-cache entries, non-contiguous */
static struct ddekit_pcache *pcache_free;

/* head of single-linked list containing used page-cache entries, contiguous */
static struct ddekit_pcache *pcache_used_contig;
/* head of single-linked list containing free page-cache entries, contiguous */
static struct ddekit_pcache *pcache_free_contig;

/* maximum number of pages to cache. defaults to a minimum of 1 page
 * because having none hits the performance too hard. */
static l4_uint32_t pcache_num_entries = 1;


/**
 * Setup page cache for all slabs
 *
 * \param pages  maximal number of memory pages
 *
 * If the maximal number of unused pages is exceeded, subsequent deallocation
 * will be freed at the memory server. This page cache caches pages from all
 * slabs.
 */
void ddekit_slab_setup_page_cache(unsigned pages)
{
	/* FIXME just allowing to grow at the moment */
	while (pcache_num_entries < pages) {
		struct ddekit_pcache *new_entry, *new_contig;

		/* create new list element */
		new_entry = (struct ddekit_pcache *) ddekit_simple_malloc(sizeof(*new_entry));
		new_contig = (struct ddekit_pcache *) ddekit_simple_malloc(sizeof(*new_contig));

		/* insert into lists of unused cache entries */
		do {
			new_entry->next = pcache_free;
		} while (!l4util_cmpxchg32((l4_uint32_t*)&pcache_free,
		                           (l4_uint32_t)new_entry->next,
		                           (l4_uint32_t)new_entry));

		do {
			new_contig->next = pcache_free_contig;
		} while (!l4util_cmpxchg32((l4_uint32_t*)&pcache_free_contig,
		                           (l4_uint32_t)new_entry->next,
		                           (l4_uint32_t)new_entry));

		/* increment number of list elements */
		l4util_inc32(&pcache_num_entries);
	}
}


/**
 * Try to allocate a new page from the pcache.
 */
static inline struct ddekit_pcache *_try_from_cache(int contig)
{
	struct ddekit_pcache *head = NULL;
	struct ddekit_pcache *the_cache = contig ? pcache_used_contig : pcache_used;

	do {
		head = the_cache;
		if (!head) break;
	} while (!l4util_cmpxchg32((l4_uint32_t*)&the_cache, (l4_uint32_t)head,
	                           (l4_uint32_t)head->next));

	return head;
}


static inline void _add_to_cache(struct ddekit_pcache *entry, int contig)
{
	struct ddekit_pcache *the_cache = contig ? pcache_used_contig : pcache_used;
	do {
		entry->next = the_cache;
	} while (! l4util_cmpxchg32((l4_uint32_t*)&the_cache, (l4_uint32_t)entry->next,
	                            (l4_uint32_t)entry));
}

/**
 * Return free entry to cached entry list.
 */
static inline void _free_cache_entry(struct ddekit_pcache *entry, int contig)
{
	struct ddekit_pcache *the_cache = contig ? pcache_free_contig : pcache_free;
	do {
		entry->next = the_cache;
	} while (!l4util_cmpxchg32((l4_uint32_t*)&the_cache, (l4_uint32_t)entry->next,
	                           (l4_uint32_t)entry));
}


static inline struct ddekit_pcache *_get_free_cache_entry(int contig)
{
	struct ddekit_pcache *the_cache = contig ? pcache_free_contig : pcache_free;
	struct ddekit_pcache *head = NULL;

	do {
		head = the_cache;
		if (!head) break;
	} while (!l4util_cmpxchg32((l4_uint32_t*)&the_cache, (l4_uint32_t)head,
	                           (l4_uint32_t) head->next));

	return head;
}


/*******************************
 ** Slab cache implementation **
 *******************************/

/* ddekit slab facilitates l4slabs */
struct ddekit_slab
{
	l4slab_cache_t cache;
	/* 
	 * Lock to prevent concurrent access to the slab's grow() and
	 * shrink() functions.
	 */
	l4lock_t       lock;
	int            contiguous;
};


/** 
 * Get the ddekit slab for a given L4 slab. 
 *
 * We cheat here, because we know that the L4 slab is the first member
 * of the ddekit slab.
 */
static inline struct ddekit_slab *ddekit_slab_from_l4slab(l4slab_cache_t *s)
{
	return (struct ddekit_slab *)s;
}

/**
 * Grow slab cache
 */
static void *_slab_grow(l4slab_cache_t *cache, void **data)
{
	/* the page(s) to be returned */
	void *res = NULL;
	/* whether this cache needs physically contiguous pages */
	int is_contig = ddekit_slab_from_l4slab(cache)->contiguous;

	/* free cache entry, will be used afterwards */
	struct ddekit_pcache *head = NULL;

	/* We don't reuse pages for slabs > 1 page, because this makes caching
	 * somewhat harder. */
	if (cache->slab_size <= L4_PAGESIZE)
		/* try to aquire cached page */
		head = _try_from_cache(is_contig);

	if (head) {
		/* use cached page */
		res = head->page;
		/* this cache entry is now available */
		_free_cache_entry(head, is_contig);
	} else {
		/* allocate new page at memory server */
		int err;
		l4_size_t tmp;
		l4dm_mem_addr_t dm_paddr;
		int num_pages = cache->slab_size / L4_PAGESIZE;
		int flags = L4DM_PINNED | L4RM_MAP | L4RM_LOG2_ALIGNED;

		if (is_contig)
			flags |= L4DM_CONTIGUOUS;

		/* allocate and map new page(s) */
		res = l4dm_mem_allocate_named(num_pages * L4_PAGESIZE,
		                              flags,
		                              "ddekit slab");
		if (res == NULL)
			ddekit_debug("__grow: error allocating a new page");

		/* physically contiguous pages need some special treatment */
		if (is_contig) {
			err = l4dm_mem_phys_addr(res, num_pages, &dm_paddr, 1, &tmp);
			if (err != 1)
				ddekit_debug("__grow: error getting physical address of new page!");

			ddekit_pgtab_set_region(res, dm_paddr.addr, num_pages, PTE_TYPE_UMA);
		}
	}

	/* save pointer to cache in page for ddekit_slab_get_slab() */
	*data = cache;

	return res;
}

/**
 * Shrink slab cache
 */
static void _slab_shrink(l4slab_cache_t *cache, void *page, void *data)
{
	/* cache deallocated page here */
	struct ddekit_pcache *head = NULL;
	/* whether this cache needs physically contiguous pages */
	int is_contig = ddekit_slab_from_l4slab(cache)->contiguous;

	/* we do not return slabs to the page cache, if they are larger than one page */
	if (cache->slab_size <= L4_PAGESIZE)
		/* try to aquire free cache entry */
		head = _get_free_cache_entry(is_contig);

	if (head) {
		/* use free cache entry to cache page */

		/* set the page info */
		head->page = page;

		/* this cache entry contains a free page */
		_add_to_cache(head, is_contig);
	} else {
		/* cache is full */
		
		if (is_contig)
			/* unset pte */
			ddekit_pgtab_clear_region(page, PTE_TYPE_UMA);

		/* free page */
		l4dm_mem_release(page);
	}
}


/**
 * Allocate object in slab
 */
void *ddekit_slab_alloc(struct ddekit_slab * slab)
{
	void *ret = NULL;
	l4lock_lock(&slab->lock);
	ret = l4slab_alloc(&slab->cache);
	l4lock_unlock(&slab->lock);

	return ret;
}


/**
 * Free object in slab
 */
void  ddekit_slab_free(struct ddekit_slab * slab, void *objp)
{
	l4lock_lock(&slab->lock);
	l4slab_free(&slab->cache, objp);
	l4lock_unlock(&slab->lock);
}


/**
 * Store user pointer in slab cache
 */
void  ddekit_slab_set_data(struct ddekit_slab * slab, void *data)
{
	l4slab_set_data(&slab->cache, data);
}


/**
 * Read user pointer from slab cache
 */
void *ddekit_slab_get_data(struct ddekit_slab * slab)
{
	return l4slab_get_data(&slab->cache);
}


/**
 * Destroy slab cache
 *
 * \param slab  pointer to slab cache structure
 */
void  ddekit_slab_destroy (struct ddekit_slab * slab)
{
	l4slab_destroy(&slab->cache);
	ddekit_simple_free(slab);
}


/**
 * Initialize slab cache
 *
 * \param size          size of cache objects
 * \param contiguous    make this slab use physically contiguous memory
 *
 * \return pointer to new slab cache or 0 on error
 */
struct ddekit_slab * ddekit_slab_init(unsigned size, int contiguous)
{
	struct ddekit_slab * slab;
	int err;

	slab = (struct ddekit_slab *) ddekit_simple_malloc(sizeof(*slab));
	slab->lock = L4LOCK_UNLOCKED;
	err = l4slab_cache_init(&slab->cache, size, 0, _slab_grow, _slab_shrink);
	if (err) {
		ddekit_debug("error initializing cache");
		ddekit_simple_free(slab);
		return 0;
	}

	slab->contiguous = contiguous;
	
	return slab;
}


/**********************************
 ** Large block memory allocator **
 **********************************/

/**
 * Free large block of memory
 *
 * This is no useful for allocation < page size.
 */
void ddekit_large_free(void *objp)
{
	/* clear PTEs */
	ddekit_pgtab_clear_region(objp, PTE_TYPE_LARGE);

	/* release */
	l4dm_mem_release(objp);
}


/**
 * Allocate large block of memory
 *
 * This is no useful for allocation < page size.
 */
void *ddekit_large_malloc(int size)
{
	void *res;
	int err;
	l4_size_t tmp;
	int pages;
	l4dm_mem_addr_t dm_paddr;

	size  = l4_round_page(size);
	pages = size >> L4_PAGESHIFT;

	res = l4dm_mem_allocate_named(size,
	                              L4DM_CONTIGUOUS | L4DM_PINNED |
	                              L4RM_MAP | L4RM_LOG2_ALIGNED,
	                              "ddekit mem");
	if (! res)
		return NULL;

	err = l4dm_mem_phys_addr(res, 1, &dm_paddr, 1, &tmp);
	if (err != 1)
		ddekit_debug("ddekit_large_malloc: error getting physical address of new memory!\n");

	ddekit_pgtab_set_region(res, dm_paddr.addr, pages, PTE_TYPE_LARGE);

	return res;
}


/**
 * Allocate large block of memory (special interface)
 *
 * This is no useful for allocation < page size.
 *
 * FIXME implementation missing...
 */
void *ddekit_contig_malloc(unsigned long size,
                           unsigned long low, unsigned long high,
                           unsigned long alignment, unsigned long boundary)
{
#if 0
	void *res;
	int err;
	int pages;
	l4_size_t tmp;
	l4dm_mem_addr_t dm_paddr;

	size  = l4_round_page(size);
	pages = size >> L4_PAGESHIFT;

	res = l4dm_mem_allocate_named(size, L4DM_CONTIGUOUS | L4DM_PINNED | L4RM_MAP | L4RM_LOG2_ALIGNED);

	if (res) {
		/* check if res meets low/high/alignment/boundary
		 * and panic if it is not the case
		 * XXXY
		 */

		/* get physical address */
		err = l4dm_mem_phys_addr(res, 1, &dm_paddr, 1, &tmp);
		if (err != 1)
			ddekit_debug("contigmalloc: error getting physical address of new page!\n");
	
		/* set PTE */
		ddekit_set_ptes(res, dm_paddr.addr, pages, PTE_TYPE_CONTIG);
	}

	return res;
#else
	ddekit_debug("%s: not implemented\n", __func__);
	return 0;
#endif
}
