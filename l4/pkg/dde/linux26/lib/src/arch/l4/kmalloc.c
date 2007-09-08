/*
 * \brief   kmalloc() implementation
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \date    2007-01-24
 *
 * In Linux 2.6 this resides in mm/slab.c.
 *
 * This implementation of kmalloc() stays with Linux's and uses kmem_caches for
 * some power of two bytes. For larger allocations ddedkit_large_malloc() is
 * used. This way, we optimize for speed and potentially waste memory
 * resources.
 */

/* Linux */
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <asm/io.h>

/* DDEKit */
#include <l4/dde/ddekit/debug.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/dde/linux26/dde26.h>

/* This stuff is needed by some drivers, e.g. for ethtool.
 * XXX: This is a fake, implement it if you really need ethtool stuff.
 */
struct page* mem_map = NULL;
static bootmem_data_t contig_bootmem_data;
struct pglist_data contig_page_data = { .bdata = &contig_bootmem_data };

/*******************
 ** Configuration **
 *******************/

#define DEBUG_MALLOC 0

/********************
 ** Implementation **
 ********************/

/*
 * These are the default caches for kmalloc. Custom caches can have other sizes.
 */
static struct cache_sizes malloc_sizes[] = {
#define CACHE(x) { .cs_size = (x) },
#include <linux/kmalloc_sizes.h>
	CACHE(ULONG_MAX)
#undef CACHE
};


/*
 * kmalloc() cache names
 */
static const char *malloc_names[] = {
#define CACHE(x) "size-" #x,
#include <linux/kmalloc_sizes.h>
	NULL
#undef CACHE
};


/**
 * Find kmalloc() cache for size
 */
static struct kmem_cache *find_cache(size_t size)
{
	struct cache_sizes *sizes;

	for (sizes = malloc_sizes; size > sizes->cs_size; ++sizes) ;

	return sizes->cs_cachep;
}


/**
 * Free previously allocated memory
 * @objp: pointer returned by kmalloc.
 *
 * If @objp is NULL, no operation is performed.
 *
 * Don't free memory not originally allocated by kmalloc()
 * or you will run into trouble.
 */
void kfree(const void *objp)
{
	if (!objp) return;

	/* find cache back-pointer */
	void **p = (void **)objp - 1;

	ddekit_log(DEBUG_MALLOC, "objp=%p cache=%p (%d)",
	           p, *p, *p ? kmem_cache_size(*p) : 0);

	if (*p)
		/* free from cache */
		kmem_cache_free(*p, p);
	else
		/* no cache for this size - use ddekit free */
		ddekit_large_free(p);
}


/**
 * Allocate memory
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate.
 *
 * kmalloc is the normal method of allocating memory
 * in the kernel.
 */
void *__kmalloc(size_t size, gfp_t flags)
{
	/* add space for back-pointer */
	size += sizeof(void *);

	/* find appropriate cache */
	struct kmem_cache *cache = find_cache(size);

	void **p;
	if (cache)
		/* allocate from cache */
		p = kmem_cache_alloc(cache, flags);
	else
		/* no cache for this size - use ddekit malloc */
		p = ddekit_large_malloc(size);

	ddekit_log(DEBUG_MALLOC, "size=%d, cache=%p (%d) => %p",
	           size, cache, cache ? kmem_cache_size(cache) : 0, p);

	/* return pointer to actual chunk */
	if (p) {
		*p = cache;
		p++;
	}
	return p;
}


void *dma_alloc_coherent(struct device *dev, size_t size, 
                         dma_addr_t *dma_handle, gfp_t flag)
{
	void *ret = (void *)__get_free_pages(flag, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = virt_to_bus(ret);
	}
	return ret;
}


void dma_free_coherent(struct device *dev, size_t size,
                       void *vaddr, dma_addr_t dma_handle)
{
	free_pages((unsigned long)vaddr, get_order(size));
}


/********************
 ** Initialization **
 ********************/

/**
 * dde_linux kmalloc initialization
 */
void l4dde26_kmalloc_init(void)
{
	struct cache_sizes  *sizes = malloc_sizes;
	const char         **names = malloc_names;

	/* init malloc sizes array */
	for (; sizes->cs_size != ULONG_MAX; ++sizes, ++names)
		sizes->cs_cachep = kmem_cache_create(*names, sizes->cs_size, 0, 0, 0, 0);
}
