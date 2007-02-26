/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/lib/src/slab.c
 * \brief  Simple slab memory allocator
 *
 * \date   07/26/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Simple Slab-based memory allocator.
 * It's a very simple implementation, restrictions are:
 * - the size of a single slab is one page, thus the max. size of an object 
 *   is the pages size minus the size of the slab descriptor
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4env includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/slab/slab.h>
#include "__slab.h"
#include "__debug.h"

/*****************************************************************************
 *** Helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Resort cache slab free list
 * 
 * \param  cache         Cache descriptor
 * \param  slab          Slab
 */
/*****************************************************************************/ 
static void
__resort(l4slab_cache_t * cache, l4slab_slab_t * slab)
{
  l4slab_slab_t * tmp;

  if ((slab->free_prev != NULL) && 
      (slab->free_prev->num_free > slab->num_free))
    {
      /* move slab towards beginning of the free list */
      do 
	{
	  tmp = slab->free_prev;
	  slab->free_prev = tmp->free_prev;
	  if (slab->free_prev != NULL)
	    slab->free_prev->free_next = slab;
	  
	  tmp->free_next = slab->free_next;
	  if (tmp->free_next != NULL)
	    tmp->free_next->free_prev = tmp;

	  slab->free_next = tmp;
	  tmp->free_prev = slab;
	}
      while ((slab->free_prev != NULL) && 
	     (slab->free_prev->num_free > slab->num_free));
    }

  if ((slab->free_next != NULL) && 
      (slab->num_free > slab->free_next->num_free))
    {
      /* move slab towards end of the free list */
      do
	{
	  tmp = slab->free_next;

	  slab->free_next = tmp->free_next;
	  if (slab->free_next != NULL)
	    slab->free_next->free_prev = slab;

	  tmp->free_prev = slab->free_prev;
	  if (tmp->free_prev != NULL)
	    tmp->free_prev->free_next = tmp;
	  else
	    cache->slabs_free = tmp;

	  slab->free_prev = tmp;
	  tmp->free_next = slab;
	}
      while ((slab->free_next != NULL) && 
	     (slab->num_free > slab->free_next->num_free));
    }
}

/*****************************************************************************/
/**
 * \brief  Add page to cache.
 * 
 * \param  cache         Cache descriptor
 * \param  page          New page
 * \param  data          Application data
 */
/*****************************************************************************/ 
static void
__add_page(l4slab_cache_t * cache, void * page, void * data)
{
  l4_addr_t * p;
  l4slab_slab_t * slab;
  int i;
  
  /* setup new slab */
  slab = &((l4slab_page_t *)page)->slab;
  slab->num_free = cache->num_objs;
  slab->cache = cache;
  slab->data = data;

  /* create object free list */
  slab->free_objs = page;
  p = page;
  for (i = 0; i < (slab->num_free - 1); i++)
    {
      *p = (l4_addr_t)(p + (cache->obj_size / sizeof(l4_addr_t)));
      p = (l4_addr_t *)*p;
    }
  *p = 0;

  /* insert slab in slab list */
  if (cache->slabs == NULL)
    slab->slab_next = NULL;
  else
    {
      slab->slab_next = cache->slabs;
      slab->slab_next->slab_prev = slab;
    }
  slab->slab_prev = NULL;
  cache->slabs = slab;
  cache->num_slabs++;
  cache->num_free++;

  /* insert slab free list */
  slab->free_prev = NULL;
  slab->free_next = cache->slabs_free;
  if (slab->free_next != NULL)
    slab->free_next->free_prev = slab;
  cache->slabs_free = slab;

  /* resort free list */
  __resort(cache,slab);
}

/*****************************************************************************/
/**
 * \brief Grow cache
 * 
 * \param  cache         Cache descriptor
 *	
 * \return 0 on success, -1 if allocation failed.
 */
/*****************************************************************************/ 
static int
__grow_cache(l4slab_cache_t * cache)
{
  void * page;
  void * data = NULL;

  /* grow function? */
  if (cache->grow_fn == NULL)
    return -1;

  /* allocate new page */
  page = cache->grow_fn(cache,&data);
  if (page == NULL)
    return -1;

  LOGdL(DEBUG_SLAB_GROW,"new slab at 0x%08x",(unsigned)page);

  /* add page to slab cache */
  __add_page(cache,page,data);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Shrink cache
 * 
 * \param  cache         Cache descriptor
 */
/*****************************************************************************/ 
static void
__shrink_cache(l4slab_cache_t * cache)
{
  l4slab_slab_t * slab;
  void * ptr, * data;

  if (cache->release_fn == NULL)
    /* cannot shrink, no release callback */
    return;

  /* shrink */
  slab = cache->slabs_free;
  while (cache->num_free > cache->max_free)
    {
      if (slab == NULL)
	{
	  /* something's wrong: free counter > 0, but no free slab found */
	  Panic("L4slab: corrupted slab free list!");
	  return;
	}

      if (slab->num_free == cache->num_objs)
	{
	  /* remove from free list */
	  if (slab->free_prev == NULL)
	    {
	      cache->slabs_free = slab->free_next;
	      if (cache->slabs_free != NULL)
		cache->slabs_free->free_prev = NULL;
	    }
	  else
	    {
	      slab->free_prev->free_next = slab->free_next;
	      if (slab->free_next != NULL)
		slab->free_next->free_prev = slab->free_prev;
	    }
	      
	  /* remove from slab list */
	  if (slab->slab_prev == NULL)
	    {
	      cache->slabs = slab->slab_next;
	      if (slab->slab_next != NULL)
		slab->slab_next->slab_prev = NULL;
	    }
	  else
	    {
	      slab->slab_prev->slab_next = slab->slab_next;
	      if (slab->slab_next != NULL)
		slab->slab_next->slab_prev = slab->slab_prev;
	    }

	  cache->num_slabs--;
	  cache->num_free--;

	  data = slab->data;
	  ptr = (void *)l4_trunc_page(slab);
	  slab = slab->free_next;

	  /* release slab */
	  cache->release_fn(cache,ptr,data);

	  /* restart search at the beginning, this is necessary because the 
	   * release callback might cause another invocation of 
	   * __shrink_cache (HUHH: recursions are bad!) if it releases 
	   * another object of the same cache. This will modify the free 
	   * list, so tmp might not be valid anymore. */
	  slab = cache->slabs_free;
	}
      else
	slab = slab->free_next;
    }
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize slab cache.
 * 
 * \param  cache         Cache descriptor
 * \param  size          Size of cache objects
 * \param  max_free      Max. allowed free slabs, if more free slabs get 
 *                       completely free, they are released (if a release 
 *                       callback function is specified)
 * \param  grow_fn       Cache grow callback function
 * \param  release_fn    Slab release callback function
 *	
 * \return 0 on success (initialized cache descriptor), error code otherwise:
 *         - -L4_EINVAL  size too big / invalid cache descriptor
 *
 * Setup cache descriptor. The function initializes the internal cache 
 * descriptor structures, but does not allocate any memory. Memory can be 
 * added using the l4slab_add_page function or memory is allocated 
 * on demand by a cache if the grow callback function is specified. 
 */
/*****************************************************************************/ 
int
l4slab_cache_init(l4slab_cache_t * cache, l4_size_t size,
                  unsigned int max_free, l4slab_grow_fn_t grow_fn,
                  l4slab_release_fn_t release_fn)
{
  /* sanity checks */
  if ((size > L4SLAB_MAX_SIZE) || (cache == NULL))
    return -L4_EINVAL;

  /* align size to pointer size */
  size = (size + (sizeof(l4_addr_t) - 1)) & ~(sizeof(l4_addr_t) - 1);

  /* setup descriptor */
  cache->obj_size = size;
  cache->num_objs = L4SLAB_MAX_SIZE / size;
  cache->num_slabs = 0;
  cache->num_free = 0;
  cache->max_free = max_free;

  cache->slabs = NULL;
  cache->slabs_free = NULL;

  cache->grow_fn = grow_fn;
  cache->release_fn = release_fn;

  cache->data = NULL;

  LOGdL(DEBUG_SLAB_INIT,"object size = %u\n" \
        "  max size = %u, objects per slab = %d",size,
        L4SLAB_MAX_SIZE,cache->num_objs);
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Destroy slab cache
 * 
 * \param  cache         Cache descriptor
 *
 * Release slab descriptor and free all allocated memory. This function is 
 * only useful if a release callback function is specified for the cache, 
 * otherwise it has no effect.
 */
/*****************************************************************************/ 
void
l4slab_destroy(l4slab_cache_t * cache)
{
  l4slab_slab_t * slab;
  void * ptr, * data;

  if ((cache == NULL) || (cache->release_fn == NULL))
    return;

  slab = cache->slabs;
  while (slab != NULL)
    {
      data = slab->data;
      ptr = (void *)l4_trunc_page(slab);
      slab = slab->slab_next;

      /* release slab */
      cache->release_fn(cache,ptr,data);
    }
}

/*****************************************************************************/
/**
 * \brief  Allocate object
 * 
 * \param  cache         Cache descriptor
 *	
 * \return pointer to object, NULL if allocation failed.
 */
/*****************************************************************************/ 
void *
l4slab_alloc(l4slab_cache_t * cache)
{
  int ret;
  l4slab_slab_t * slab;
  void * objp;

  if (cache == NULL)
    return NULL;

  /* need to allocate new slab ? */
  if (cache->slabs_free == NULL)
    {
      ret = __grow_cache(cache);
      if (ret < 0)
	return NULL;
    }

  /* get slab which contains free objects */
  slab = cache->slabs_free;

  /* allocate object, just use the first slab from the caches free list */
  objp = slab->free_objs;
  slab->free_objs = (void *)*((l4_addr_t *)objp);

  if (slab->num_free == cache->num_objs)
    /* used first object of a free slab */
    cache->num_free--;
  slab->num_free--;

  if (slab->free_objs == NULL)
    {
      /* no more free elements in slab's free list, remove slab from cache 
       * free list */
      cache->slabs_free = slab->free_next;
      if (cache->slabs_free != NULL)
	cache->slabs_free->free_prev = NULL;
    }

  /* done */
  return objp;
}

/*****************************************************************************/
/**
 * \brief  Release object
 * 
 * \param  cache         Cache descriptor
 * \param  objp          Pointer to object
 */
/*****************************************************************************/ 
void
l4slab_free(l4slab_cache_t * cache, void * objp)
{
  l4slab_slab_t * slab;

  /* sanity */
  if ((cache == NULL) || (objp == NULL))
    return;

  /* get slab descriptor */
  slab = &(((l4slab_page_t *)l4_trunc_page(objp))->slab);

  /* correct cache? */
  if (cache != slab->cache)
    return;

  /* insert obj in free list */
  *((l4_addr_t *)objp) = (l4_addr_t)slab->free_objs;
  slab->free_objs = objp;

  if (slab->num_free == 0)
    {
      /* first free object in slab, insert slab into cache's free slab list */
      slab->free_next = cache->slabs_free;
      if (slab->free_next != NULL)
	slab->free_next->free_prev = slab;
      cache->slabs_free = slab;
    }
  slab->num_free++;

  if (slab->num_free == cache->num_objs)
    {
      /* slab completely freed */
      cache->num_free++;
      if (cache->num_free > cache->max_free)
	__shrink_cache(cache);
      else
	__resort(cache,slab);
    }
  else
    __resort(cache,slab);
}

/*****************************************************************************/
/**
 * \brief Add page to slab cache
 * 
 * \param  cache         Cache descriptor
 * \param  page          Pointer to new page
 * \param  data          Application data
 * 
 * Add the page to the slab cache. 
 */
/*****************************************************************************/ 
void
l4slab_add_page(l4slab_cache_t * cache, void * page, void * data)
{
  /* sainty checks */
  if ((cache == NULL) || (page == NULL))
    return;

  /* add page */
  __add_page(cache,page,data);
}

/*****************************************************************************/
/**
 * \brief  Set cache application data pointer.
 * 
 * \param  cache         Cache descriptor
 * \param  data          Application data pointer
 */
/*****************************************************************************/ 
void 
l4slab_set_data(l4slab_cache_t * cache, void * data)
{
  if (cache == NULL)
    return;

  cache->data = data;
}

/*****************************************************************************/
/**
 * \brief Get cache application data.
 * 
 * \param  cache         Cache descriptor
 *	
 * \return Application data pointer, NULL if invalid cache descriptor or no
 *         data pointer set.
 */
/*****************************************************************************/ 
void * 
l4slab_get_data(l4slab_cache_t * cache)
{
  if (cache == NULL)
    return NULL;

  return cache->data;
}

/*****************************************************************************/
/**
 * \brief DEBUG: Dump cache slab list
 * 
 * \param  cache         Cache descriptor
 * \param  dump_free     Dump free list of slabs
 */
/*****************************************************************************/ 
void 
l4slab_dump_cache(l4slab_cache_t * cache, int dump_free)
{
  l4slab_slab_t * slab;
  unsigned int * p;

  if (cache == NULL)
    return;

  printf("  L4slab cache list, cache at 0x%08x\n",(unsigned)cache);
  printf("  object size %u, %u per slab, %d slab(s), %d free\n",
         cache->obj_size,cache->num_objs,cache->num_slabs,
         cache->num_free);

  slab = cache->slabs;
  while (slab != NULL)
    {
      printf("    slab at 0x%08x, %d free object(s):\n",
	     (unsigned)slab,slab->num_free);

      if (dump_free)
	{
	  p = slab->free_objs;
	  while (p != NULL)
	    {
	      printf("      free at 0x%08x\n",(unsigned)p);
	      p = (unsigned int *)*p;
	    }
	}

      slab = slab->slab_next;
    }
}

/*****************************************************************************/
/**
 * \brief  DEBUG: Dump cache free slab list
 * 
 * \param  cache         Cache descriptor
 */
/*****************************************************************************/ 
void 
l4slab_dump_cache_free(l4slab_cache_t * cache)
{
  l4slab_slab_t * slab;
  if (cache == NULL)
    return;

  printf("  L4slab cache free list, cache at 0x%08x\n",(unsigned)cache);
  printf("  object size %u, %u per slab, %d slab(s), %d free\n",
         cache->obj_size,cache->num_objs,cache->num_slabs,
         cache->num_free);

  slab = cache->slabs_free;
  while (slab != NULL)
    {
      printf("    slab at 0x%08x, %3d free object(s)\n",
	     (unsigned)slab,slab->num_free);
      slab = slab->free_next;
    }
}
