/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/internal_alloc.c
 * \brief  DMphys internal memory allocation
 *
 * \date   02/04/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * DMphys uses the L4Env slab allocator (l4/slab/slab.h) for the dynamic 
 * allocation of various descriptors:
 * - memory map area (memmap.c)
 * - page pool memory area (pages.c)
 * - internal dataspace descriptor (dataspace.c)
 * - DSMlib dataspace descriptor / client descriptor
 * To allocate the pages used by these slab caches we cannot use the regular
 * page allocation function (dmphys_pages_allocate()) directly in the 
 * grow-callback function, because the page pools might not yet be initialized
 * (during setup) and dmphys_pages_allocate() itself can allocate new 
 * descriptors which would lead to a deadlocked situation. 
 * Instead we use a separate memory pool which holds a small number of 
 * preallocated pages to add more memory to the slab caches in the grow
 * callback function. Initially, this memory pool is filled with pages 
 * allocated directly at the sigma0 server. After the setup has finished, 
 * further pages will be allocated using the regular page allocation 
 * function, but we make sure that the memory pool allways holds enough 
 * pages to fullfill a grow-callback of one of the slab caches directly
 * without allocating pages form the regular page pools. To ensure that, 
 * the memory pool must be refilled as soon as possible after a page might 
 * have been allocated from the pool. To do that, dmphys_update_desc_pool()
 * must be called after a descriptor was allocated. If a page was allocated,
 * this will refill the memory pool.
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h> 
#include <l4/util/macros.h>
#include <l4/slab/slab.h>
#include <l4/dm_generic/dm_generic.h>

/* DMphys includes */
#include "__internal_alloc.h"
#include "__sigma0.h"
#include "__memmap.h"
#include "__pages.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * descriptor allocation memory pool size
 */
#define POOL_SIZE  (DMPHYS_INT_POOL_INITIAL + DMPHYS_INT_POOL_MAX_ALLOC)

/**
 * descriptor allocation memory pool
 */
static int_pool_t mem_pool[POOL_SIZE];

/**
 * number of free page in memory pool
 */
static int num_free;

/**
 * update memory pool in progress
 */
static int update_in_progress = 0;

/**
 * released page area table
 */
static page_area_t * freed_areas[DMPHYS_INT_MAX_FREED];

/**
 * number of page areas in freed table 
 */
static volatile int num_freed_areas = 0;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate page from descriptor memory pool
 *	
 * \retval area          Page area descriptor
 *
 * \return Pointer to page, NULL if allocation failed
 */
/*****************************************************************************/ 
static void *
__allocate(page_area_t ** area)
{
  int    i;
  void * page;

  for (i = 0; i < POOL_SIZE; i++)
    {
      if (mem_pool[i].available)
	{
	  ASSERT(mem_pool[i].map_addr != -1);

	  LOGdL(DEBUG_INT_ALLOC,"using page %2d at 0x%08x",
                i,mem_pool[i].map_addr);

	  page = (void *)mem_pool[i].map_addr;
	  mem_pool[i].available = 0;

	  if (mem_pool[i].area == NULL)
	    {
	      /* got page allocated at sigma0 */
	      ASSERT(i < DMPHYS_INT_POOL_INITIAL);
	      *area = NULL;
	    }
	  else
	    {
	      /* got page allocated from regular page pool */
	      *area = mem_pool[i].area;

	      /* mark pool entry unallocated, force refill */
	      mem_pool[i].map_addr = -1;
	      mem_pool[i].area = NULL;
	    }
	  num_free--;

	  return page;
	}
    }

  Panic("DMphys: no page found in internal memory pool!");
  return NULL;
}

/*****************************************************************************/
/**
 * \brief  Release page
 * 
 * \param  page          Page address
 * \param  area          Page area descriptor
 */
/*****************************************************************************/ 
static void
__release(void * page, 
	  page_area_t * area)
{
  l4_addr_t addr = (l4_addr_t)page;
  int i;

  LOGdL(DEBUG_INT_ALLOC,"release page at 0x%08x",addr);

  if (area != NULL)
    {
      /* got page area descriptor, first try to insert it into an unused 
       * entry of the memory pool. This would cause the page area to be 
       * reused in future allocations. By reuising page areas (instead 
       * of just freeing the area) we reduce the number of pages 
       * allocated from the default memory pool which hopefully reduces
       * the fragmentation of the pool caused by internal allocations. */
      i = DMPHYS_INT_POOL_INITIAL;
      while ((i < POOL_SIZE) && (mem_pool[i].map_addr != -1))
	i++;

      if (i < POOL_SIZE)
	{
	  /* found an entry */
	  mem_pool[i].available = 1;
	  mem_pool[i].map_addr = AREA_MAP_ADDR(area);
	  mem_pool[i].area = area;
	  num_free++;

#if DEBUG_INT_ALLOC
	  printf("  reuse area at pool page %2d\n",i);
#endif
	}
      else
	{
	  /* no unused memory pool entry fopund, add to freed area table.
	   * we cannot release the page area directly, because we might 
	   * already be called from dmphys_pages_release() and we must not
	   * call it recursively (the page area list might be in an 
	   * inconsistent state) */
	  if (num_freed_areas >= DMPHYS_INT_MAX_FREED)
	    printf("DMphys: to many freed areas in internal memory pool!");
	  else
	    {
	      freed_areas[num_freed_areas] = area;
	      num_freed_areas++;
	    }
#if DEBUG_INT_ALLOC
	  printf("  inserted area into freed table\n");
#endif
	}
    }
  else
    {
      /* page was allocated at sigma0, find in memory pool table and mark 
       * available */
      for (i = 0; i < DMPHYS_INT_POOL_INITIAL; i++)
	{
	  if (mem_pool[i].map_addr == addr)
	    {
#if DEBUG_INT_ALLOC
	      printf("  mark pool page %2d free\n",i);
#endif
	      mem_pool[i].available = 1;
	      break;
	    }
	}

      if (i == DMPHYS_INT_POOL_INITIAL)
	printf("DMphys: not a page from memory pool (0x%08x)",addr);
    }
}

/*****************************************************************************/
/**
 * \brief  Refill descriptor memory pool
 */
/*****************************************************************************/ 
static void
__refill(void)
{
  int i;
  page_pool_t * pool = dmphys_get_default_pool();
  page_area_t * pa;

  /* check update_in_progress to avoid recursive invocation */
  if (update_in_progress)
    return;
  update_in_progress = 1;
  
  for (i = DMPHYS_INT_POOL_INITIAL; i < POOL_SIZE; i++)
    {
      if (mem_pool[i].map_addr == -1)
	{
	  /* allocate new page */
	  if (dmphys_pages_allocate(pool,L4_PAGESIZE,0,
				    L4DM_CONTIGUOUS,PAGES_INTERNAL,&pa) < 0)
	    /* Huhh: no more memory available */
	    break;

	  mem_pool[i].available = 1;
	  mem_pool[i].map_addr = AREA_MAP_ADDR(pa);
	  mem_pool[i].area = pa;
	  num_free++;

	  LOGdL(DEBUG_INT_ALLOC,"pool page %2d at 0x%08x",
                i,mem_pool[i].map_addr);
	}
    }

  if (num_free < DMPHYS_INT_POOL_MIN)
    printf("DMphys: running low on internal memory!\n");

  /* done */
  update_in_progress = 0;
}

/*****************************************************************************
 *** DMphys internal API function
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize descriptor allocation
 *	
 * \return 0 on success, -1 if something went wrong.
 */
/*****************************************************************************/ 
int
dmphys_internal_alloc_init(void)
{
  int    i;
  void * page;
  
  /* initialize memory pool table */
  for (i = 0; i < POOL_SIZE; i++)
    {
      mem_pool[i].available = 0;
      mem_pool[i].map_addr = -1;
      mem_pool[i].area = NULL;
    }

  /* allocate initial memory pool pages at sigma0 */
  num_free = 0;
  for (i = 0; i < DMPHYS_INT_POOL_INITIAL; i++)
    {
      page = dmphys_sigma0_map_any_page();
      if (page != NULL)
	{
	  LOGdL(DEBUG_INT_ALLOC,"\n  got page at 0x%08x",(l4_addr_t)page);

	  mem_pool[i].map_addr = (l4_addr_t)page;
	  mem_pool[i].available = 1;
	  num_free++;
	}
      else
	{
	  Panic("DMphys: initial allocate internal memory pool failed!");
	  return -1;
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Reserve initial pages in memory map, this cannot be done in 
 *         dmphys_internal_alloc_init() because at this point the memory 
 *         map is not yet initialized.
 */
/*****************************************************************************/ 
void
dmphys_internal_alloc_init_reserve(void)
{
  int i;
  l4_addr_t addr;

  for (i = 0; i < DMPHYS_INT_POOL_INITIAL; i++)
    {
      /* mark page reserved in memory map */
      if (mem_pool[i].map_addr != -1)
	{
	  addr = PHYS_ADDR(mem_pool[i].map_addr);
	  if (dmphys_memmap_reserve(addr,L4_PAGESIZE) < 0)
	    Panic("DMphys: reserve page at 0x%08x in memory map failed!",
		  addr);
	}
    }
}

/*****************************************************************************/
/**
 * \brief  Free unused page areas
 */
/*****************************************************************************/ 
void
dmphys_internal_alloc_update_free(void)
{
  page_pool_t * pool = dmphys_get_default_pool();
  page_area_t * pa;

  while (num_freed_areas > 0)
    {
      /* free page area, this might again add an area to the freed table */
      num_freed_areas--;
      pa = freed_areas[num_freed_areas];

      LOGdL(DEBUG_INT_ALLOC,"\n  free area at 0x%08x",AREA_MAP_ADDR(pa));

      dmphys_pages_release(pool,pa);
    }
}

/*****************************************************************************/
/**
 * \brief  Update internal memory pool if necessary
 */
/*****************************************************************************/ 
void
dmphys_internal_alloc_update(void)
{
  /* refill */
  if (num_free < DMPHYS_INT_POOL_MIN)
    __refill();

  /* also check if we can release something */
  dmphys_internal_alloc_update_free();
}

/*****************************************************************************/
/**
 * \brief  Generic descriptor slab cache grow callback function
 * 
 * \param  cache         Slab cache descriptor
 * \retval data          Slab cache data pointer, it is set to the page area
 *                       descriptor of the page used for the slab (or NULL 
 *                       if page directly allocated at sigma0), it is used
 *                       in the release callback to release the page
 *	
 * \return Pointer to the allocated page, NULL if allocation failed
 */
/*****************************************************************************/ 
void *
dmphys_internal_alloc_grow(l4slab_cache_t * cache, 
			   void ** data)
{
#if DEBUG_INT_ALLOC
  char * name = (char *)l4slab_get_data(cache);

  if (name != NULL)
    LOGL("%s",name);
#endif

  /* allocate page */
  return __allocate((page_area_t **)data);
}

/*****************************************************************************/
/**
 * \brief  Generic descriptor slab cache release callback funtion 
 * 
 * \param  cache         Slab cache descriptor
 * \param  page          Page address
 * \param  data          Slab cache data pointer, set to the page area 
 *                       descriptor (or NULL if page allocated directly at 
 *                       sigma0) in the grow callback function
 */
/*****************************************************************************/ 
void
dmphys_internal_alloc_release(l4slab_cache_t * cache, 
			      void * page, 
			      void * data)
{
#if DEBUG_INT_ALLOC
  char * name = (char *)l4slab_get_data(cache);

  if (name != NULL)
    LOGL("%s",name);
#endif

  /* release page */
  __release(page,(page_area_t *)data);
}

/*****************************************************************************/
/**
 * \brief  Allocate page from internal memory pool
 * 
 * \retval data          Data pointer, set to the page area descriptor 
 *                       (or NULL if page directly allocated at sigma0)
 *	
 * \return Pointer to allocated page, NULL if allocation failed.
 */
/*****************************************************************************/ 
void *
dmphys_internal_allocate(void ** data)
{
  LOGdL(DEBUG_INT_ALLOC,"allocate");

  /* allocate page */
  return __allocate((page_area_t **)data);  
}

/*****************************************************************************/
/**
 * \brief  Release page from internal memory pool
 * 
 * \param  page          Page address
 * \param  data          Data pointer, it is set to the page area descriptor
 *                       by dmphys_internal_allocate
 */
/*****************************************************************************/ 
void
dmphys_internal_release(void * page, 
			void * data)
{
  LOGdL(DEBUG_INT_ALLOC,"release");

  /* release page */
  __release(page,(page_area_t *)data);
}

     
