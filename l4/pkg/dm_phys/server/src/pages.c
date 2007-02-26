/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/pages.c
 * \brief  DMphys, page area pool handling
 *
 * \date   08/05/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <string.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/slab/slab.h>
#include <l4/dm_generic/dm_generic.h>

/* private includes */
#include <l4/dm_phys/consts.h>
#include "__pages.h"
#include "__dm_phys.h"
#include "__internal_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Page area descriptor slab cache
 */
l4slab_cache_t area_cache;

/**
 * Page area descriptor slab cache name
 */
static char * area_cache_name = "page area";

/**
 * Page pools
 */
static page_pool_t page_pools[DMPHYS_NUM_POOLS];

/* Shortcuts to default pools */
#define DEFAULT_POOL (&page_pools[DMPHYS_MEM_DEFAULT_POOL])
#define ISA_DMA_POOL (&page_pools[DMPHYS_MEM_ISA_DMA_POOL])

/*****************************************************************************
 *** page area descriptor allocation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate area descriptor
 *
 * \return Pointer to area descriptor, NULL if allocation failed.
 */
/*****************************************************************************/
static inline page_area_t *
__alloc_area_desc(void)
{
  page_area_t * pa = (page_area_t *)l4slab_alloc(&area_cache);

  if (pa == NULL)
    Panic("DMphys: page area descriptor allocation failed!");

  return pa;
}

/*****************************************************************************/
/**
 * \brief  Release page area descriptor
 *
 * \param  area          Area descriptor
 */
/*****************************************************************************/
static inline void
__release_area_desc(page_area_t * area)
{
  l4slab_free(&area_cache,area);
}

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Calculate max. alignment in area
 *
 * \param  addr          Area start address
 * \param  size          Area size
 *
 * \return Max. alignment in area.
 *
 * This function calculates the max. alignment (and thus the size of a
 * properly aligned memory area) which is possible using the specified memory
 * area.
 */
/*****************************************************************************/
static inline l4_addr_t
__max_alignment(l4_addr_t addr, l4_addr_t size)
{
  int max = l4util_bsr(size);
  l4_addr_t alignment = (1 << max);
  l4_addr_t offs_mask = (alignment - 1);
  l4_addr_t offs;

  if (size == 0)
    return 0;

  if ((addr & offs_mask) != 0)
    {
      /* area start address not aligned, check whether the area is big enough
       * to still provide the alignment */
      offs = alignment - (addr & offs_mask);
      if ((alignment + offs) > size)
	/* area not big enough */
	alignment >>= 1;
    }

  /* done */
  return alignment;
}

/*
 * Align value up to next alignment boundary.
 *
 * \param val       Value to round
 * \parma alignment Alignment, must be power of 2
 *
 * \return Rounded value
 */
static inline l4_addr_t
__round_up(l4_addr_t val, l4_addr_t alignment)
{
  return (val + alignment - 1) & ~(alignment - 1);
}

/*
 * Align value down to next alignment boundary.
 *
 * \param val       Value to round
 * \parma alignment Alignment, must be power of 2
 *
 * \return Rounded value
 */
static inline l4_addr_t
__round_down(l4_addr_t val, l4_addr_t alignment)
{
  return val & ~(alignment - 1);
}

/*****************************************************************************/
/**
 * \brief  Calculate page area size with alignment
 *
 * \param  area          Page area
 * \param  alignment     Alignment
 *
 * \return Size of page area with an aligned start address
 */
/*****************************************************************************/
static inline l4_size_t
__aligned_size(page_area_t * area, l4_addr_t alignment)
{
  l4_addr_t offs_begin, offs_end;

  offs_begin = __round_up(area->addr, alignment) - area->addr;
  if (offs_begin > area->size)
    return 0;

  offs_end = area->addr + area->size
             - __round_down(area->addr + area->size, alignment);
  if (offs_end > area->size)
    return 0;

  return area->size - offs_begin - offs_end;
}

/*****************************************************************************/
/**
 * \brief  Get free list number for area size
 *
 * \param  size          Area size
 *
 * \return free list number.
 *
 * We devide the availble free areas into groups with sizes of a power of two,
 * the of the appropriate free list number is log2(size / PAGESIZE).
 */
/*****************************************************************************/
static inline int
__get_free_list(l4_size_t size)
{
  int i = l4util_bsr(size) - DMPHYS_LOG2_PAGESIZE;

  if (i < 0)
    {
      /* this should never happen, size should be at least 4096 */
      LOGdL(DEBUG_ERRORS, "DMphys: invalid area size (%u)", size);
      return 0;
    }

  if (i >= DMPHYS_NUM_FREE_LISTS)
    /* the last list holds all larger areas */
    return (DMPHYS_NUM_FREE_LISTS - 1);

  return i;
}

/*****************************************************************************
 *** Page area lists
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add area to area list.
 *
 * \param  pool          Page pool
 * \param  area          Area descriptor
 *
 * \return 0 on success, -1 if something went wrong.
 */
/*****************************************************************************/
static int
__add_area(page_pool_t * pool, page_area_t * area)
{
  page_area_t * pa;
  l4_addr_t start = area->addr;
  l4_addr_t end   = area->addr + area->size;

  area->area_prev = NULL;
  area->area_next = NULL;

  if (pool->area_list == NULL)
    {
      /* first area in page pool */
      pool->area_list = area;
      return 0;
    }

  pa = pool->area_list;
  if (end <= pa->addr)
    {
      /* add area at list head */
      area->area_next = pa;
      pa->area_prev = area;
      pool->area_list = area;
      return 0;
    }

  /* find right place, after that pa should point to the last area in front
   * of the new area */
  while ((pa->area_next) && (pa->area_next->addr < start))
    pa = pa->area_next;

  if (((pa->addr + pa->size) > start) ||
      (pa->area_next && (end > pa->area_next->addr)))
    {
      /* uuh: the new area overlaps an existing area */
#if DEBUG_ERRORS
      if (pa->area_next)
	LOG_printf("  (0x%08lx-0x%08lx),(0x%08lx-0x%08lx),(0x%08lx-0x%08lx)\n",
               pa->addr, pa->addr + pa->size,
               area->addr, area->addr + area->size,
               pa->area_next->addr, pa->area_next->addr + pa->area_next->size);
      else
	LOG_printf("  (0x%08lx-0x%08lx),(0x%08lx-0x%08lx)\n", pa->addr,
               pa->addr + pa->size, area->addr, area->addr + area->size);
      Panic("DMphys: new area overlaps existing area!");
#endif
      return -1;
    }

  /* insert area */
  area->area_next = pa->area_next;
  if (area->area_next)
    area->area_next->area_prev = area;
  area->area_prev = pa;
  pa->area_next = area;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Remove area from area list
 *
 * \param  pool          Page pool
 * \param  area          Area descriptor
 *
 * \return 0 on success, -1 if something went wrong.
 */
/*****************************************************************************/
static int
__remove_area(page_pool_t * pool, page_area_t * area)
{
  if (area == pool->area_list)
    {
      /* remove first area in area list */
      pool->area_list = area->area_next;
      if (pool->area_list)
	pool->area_list->area_prev = NULL;
      return 0;
    }

  if (area->area_prev == NULL)
    {
      /* fatal error */
      Panic("DMphys: corrupted area list!");
      return -1;
    }

  /* remove */
  area->area_prev->area_next = area->area_next;
  if (area->area_next)
    area->area_next->area_prev = area->area_prev;

  return 0;
}

/*****************************************************************************
 *** Free lists
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add area to appropriate free list
 *
 * \param  pool          Page pool
 * \param  area          Area descriptor
 *
 * \return 0 on success, -1 if something went wrong.
 *
 * \note We should actually sort the free lists not according to the size of
 *       the areas, but at first according to the max. alignment provided by
 *       an area (the smaller alignment first) and only at second according
 *       to the size. This would ensure that we do not split a (smaller)
 *       area which provides a larger alignment than another (larger) area
 *       which provides a smaller alignment (because of a bad placement of
 *       that area in memory). Todo!
 */
/*****************************************************************************/
static int
__add_free(page_pool_t * pool, page_area_t * area)
{
  page_area_t * pa;
  int num = __get_free_list(area->size);

  /* update pool free counter */
  pool->free += area->size;

  /* add to free list */
  area->free_prev = NULL;
  area->free_next = NULL;

  if (pool->free_list[num] == NULL)
    {
      /* first area in free list */
      pool->free_list[num] = area;
      return 0;
    }

  pa = pool->free_list[num];
  if (pa->size > area->size)
    {
      /* add area at list head */
      area->free_next = pa;
      pa->free_prev = area;
      pool->free_list[num] = area;
      return 0;
    }

  /* find right place in free list */
  while ((pa->free_next) && (pa->free_next->size < area->size))
    pa = pa->free_next;

  /* add */
  area->free_next = pa->free_next;
  if (area->free_next)
    area->free_next->free_prev = area;
  area->free_prev = pa;
  pa->free_next = area;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Remove area from free list
 *
 * \param  pool          Page pool
 * \param  area          Area descriptor
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/
static int
__remove_free(page_pool_t * pool, page_area_t * area)
{
  int num = __get_free_list(area->size);

  if (area == pool->free_list[num])
    {
      /* remove first area in free list */
      pool->free_list[num] = pool->free_list[num]->free_next;
      if (pool->free_list[num])
	pool->free_list[num]->free_prev = NULL;
    }
  else
    {
      if (area->free_prev == NULL)
	{
	  /* fatal error */
	  Panic("DMphys: corrupted free list");
	  return -1;
	}

      /* remove */
      area->free_prev->free_next = area->free_next;
      if (area->free_next)
	area->free_next->free_prev = area->free_prev;
    }

  /* update pool free counter */
  pool->free -= area->size;

  return 0;
}

/*****************************************************************************
 *** Page area manipulation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Add free page area to area and free list
 *
 * \param  pool          Page pool
 * \param  addr          Area start address
 * \param  size          Area size
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/
static int
__add_free_area(page_pool_t * pool, l4_addr_t addr, l4_size_t size)
{
  page_area_t * area;

  LOGdL(DEBUG_PAGES_ADD, "pool %d: area 0x%08lx-0x%08lx (%6uKB)",
        pool->pool, addr, addr + size, size >> 10);

  /* allocate area descriptor */
  area = __alloc_area_desc();
  if (area == NULL)
    {
      Panic("DMphys: no area descriptor available!");
      return -1;
    }

  /* init descriptor */
  area->addr = addr;
  area->size = size;
  area->flags = 0;
  area->ds_next = NULL;
  SET_AREA_UNUSED(area);

  /* add to area list */
  if (__add_area(pool, area) < 0)
    return -1;

  /* add to free list */
  if (__add_free(pool, area) < 0)
    return -1;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Split free page area
 *
 * \param  pool          Page pool
 * \param  area          Area descriptor
 * \param  size          Requested size
 * \param  alignment     Requested alignment
 *
 * \return 0 on success (the area descriptor is adapted to the new
 *         address / size), -1 if something went wrong.
 *
 * Split page area, add remaining area(s) to page pool / free lists.
 */
/*****************************************************************************/
static int
__split_area(page_pool_t * pool, page_area_t * area, l4_size_t size,
	     l4_addr_t alignment)
{
  int at_beginning;
  l4_addr_t addr_sb, addr_se;
  l4_addr_t sb_addr_b, sb_addr_e, se_addr_b, se_addr_e;
  l4_size_t sb_size_b, sb_size_e, sb_size, se_size_b, se_size_e, se_size;
  l4_addr_t sb_max_b, sb_max_e, sb_max, se_max_b, se_max_e, se_max;

  /* we have two options for splitting, at the beginning of the area or at
   * the end of the area. We use the version which provides the larger
   * alignment in the remaining free areas */

  /* split at the beginning of the page area */
  addr_sb = (area->addr + alignment - 1) & ~(alignment - 1);
  sb_addr_b = area->addr;
  sb_size_b = addr_sb - area->addr;
  sb_addr_e = addr_sb + size;
  sb_size_e = (area->addr + area->size) - sb_addr_e;

  ASSERT(sb_addr_e <= (area->addr + area->size));

  sb_max_b = __max_alignment(sb_addr_b, sb_size_b);
  sb_max_e = __max_alignment(sb_addr_e, sb_size_e);
  sb_max = (sb_max_b > sb_max_e) ? sb_max_b : sb_max_e;
  sb_size = (sb_size_b > sb_size_e) ? sb_size_b : sb_size_e;

  /* split at the end of the page area */
  addr_se = ((area->addr + area->size) - size) & ~(alignment - 1);
  se_addr_b = area->addr;
  se_size_b = addr_se - area->addr;
  se_addr_e = addr_se + size;
  se_size_e = (area->addr + area->size) - se_addr_e;

  ASSERT(se_addr_e <= (area->addr + area->size));

  se_max_b = __max_alignment(se_addr_b, se_size_b);
  se_max_e = __max_alignment(se_addr_e, se_size_e);
  se_max = (se_max_b > se_max_e) ? se_max_b : se_max_e;
  se_size = (se_size_b > se_size_e) ? se_size_b : se_size_e;

  /* where to split? */
  if (sb_max > se_max)
    at_beginning = 1;
  else if (sb_max < se_max)
    at_beginning = 0;
  else
    {
      /* sb_max == se_max,
       * use the version which yields the larger remaining area */
      if (sb_size >= se_size)
	at_beginning = 1;
      else
	at_beginning = 0;
    }

#if DEBUG_PAGES_SPLIT
  LOGL("split area 0x%08lx-0x%08lx", area->addr, area->addr + area->size);
  LOG_printf("  size 0x%08x (%6uKB), alignment 0x%08lx\n",
         size, size >> 10, alignment);

  LOG_printf("  split at beginning (0x%08x/0x%08lx)\n", sb_size, sb_max);
  LOG_printf("    0x%08lx-0x%08lx, 0x%08x/0x%08lx\n",
         sb_addr_b, sb_addr_b + sb_size_b, sb_size_b, sb_max_b);
  LOG_printf("    0x%08lx-0x%08lx\n", addr_sb, addr_sb + size);
  LOG_printf("    0x%08lx-0x%08lx, 0x%08x/0x%08lx\n",
         sb_addr_e, sb_addr_e + sb_size_e, sb_size_e, sb_max_e);

  LOG_printf("  split at end (0x%08x/0x%08lx)\n", se_size, se_max);
  LOG_printf("    0x%08lx-0x%08lx, 0x%08x/0x%08lx\n",
         se_addr_b, se_addr_b + se_size_b, se_size_b, se_max_b);
  LOG_printf("    0x%08lx-0x%08lx\n", addr_se, addr_se + size);
  LOG_printf("    0x%08lx-0x%08lx, 0x%08x/0x%08lx\n",
         se_addr_e, se_addr_e + se_size_e, se_size_e, se_max_e);

  if (at_beginning)
    LOG_printf("  splitting at beginning\n");
  else
    LOG_printf("  splitting at end\n");
#endif

  if (at_beginning)
    {
      /* split at beginning */
      area->addr = addr_sb;
      area->size = size;

      /* add remaining areas */
      if (sb_size_b > 0)
	{
	  if (__add_free_area(pool, sb_addr_b, sb_size_b) < 0)
	    {
	      PANIC("DMphys: failed to add free area!");
	      return -1;
	    }
	}

      if (sb_size_e > 0)
	{
	  if (__add_free_area(pool, sb_addr_e, sb_size_e) < 0)
	    {
	      PANIC("DMphys: failed to add free area!");
	      return -1;
	    }
	}
    }
  else
    {
      /* split at end */
      area->addr = addr_se;
      area->size = size;

      /* add remaining areas */
      if (se_size_b > 0)
	{
	  if (__add_free_area(pool, se_addr_b, se_size_b) < 0)
	    {
	      PANIC("DMphys: failed to add free area!");
	      return -1;
	    }
	}

      if (se_size_e > 0)
	{
	  if (__add_free_area(pool, se_addr_e, se_size_e) < 0)
	    {
	      PANIC("DMphys: failed to add free area!");
	      return -1;
	    }
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** Page allocation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Release page area
 *
 * \param  pool          Page pool
 * \param  area          Page area
 *
 * \return 0 on success, -1 if something went wrong.
 *
 * Mark page area unused, try to merge with adjacent areas and add to free
 * list.
 */
/*****************************************************************************/
static int
__release_area(page_pool_t * pool, page_area_t * area)
{
  page_area_t * pa;

  /* mark area unused */
  SET_AREA_UNUSED(area);
  area->ds_next = NULL;

  LOGdL(DEBUG_PAGES_RELEASE, "pool %d: area 0x%08lx-0x%08lx",
        pool->pool, area->addr, area->addr + area->size);

  /* try to merge with previous area */
  if (area->area_prev != NULL)
    {
      pa = area->area_prev;
      if (IS_UNUSED_AREA(pa) && ((pa->addr + pa->size) == area->addr))
	{
	  /* merge, remove previous area from free list */
	  if (__remove_free(pool, pa) < 0)
	    {
	      PANIC("DMphys: remove previous area from free list failed!");
	      return -1;
	    }

	  /* remove previous area from area list */
	  if (__remove_area(pool, pa) < 0)
	    {
	      PANIC("DMphys: remove previous area from area list failed!");
	      return -1;
	    }

#if DEBUG_PAGES_RELEASE
	  LOG_printf(" merge 0x%08lx-0x%08lx with 0x%08lx-0x%08lx\n",
                 pa->addr, pa->addr + pa->size,
                 area->addr, area->addr + area->size);
#endif

	  /* add previous area to area */
	  area->addr = pa->addr;
	  area->size += pa->size;

#if DEBUG_PAGES_RELEASE
	  LOG_printf(" got 0x%08lx-0x%08lx\n",
	      area->addr, area->addr + area->size);
#endif

	  /* release previous area descriptor */
	  __release_area_desc(pa);
	}
    }

  /* try to merge with next area */
  if (area->area_next != NULL)
    {
      pa = area->area_next;
      if (IS_UNUSED_AREA(pa) && ((area->addr + area->size) == pa->addr))
	{
	  /* merge, remove next area from free list */
	  if (__remove_free(pool, pa) < 0)
	    {
	      PANIC("DMphys: remove next area from free list failed!");
	      return -1;
	    }

	  /* remove next area from area list */
	  if (__remove_area(pool, pa) < 0)
	    {
	      PANIC("DMphys: remove next area from area list failed!");
	      return -1;
	    }

#if DEBUG_PAGES_RELEASE
	  LOG_printf(" merge 0x%08lx-0x%08lx with 0x%08lx-0x%08lx\n",
                 area->addr, area->addr + area->size,
                 pa->addr, pa->addr + pa->size);
#endif

	  /* area next area to area */
	  area->size += pa->size;

#if DEBUG_PAGES_RELEASE
	  LOG_printf(" got 0x%08lx-0x%08lx\n",
	      area->addr, area->addr + area->size);
#endif

	  /* release next area descriptor */
	  __release_area_desc(pa);
	}
    }

  /* add area to free list */
  if (__add_free(pool, area) < 0)
    {
      PANIC("DMphys: add area to free list failed!");
      return -1;
    }

  /* we have released descriptors, free unused page areas in
   * descriptor memory pool */
  dmphys_internal_alloc_update_free();

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Find a single free page area
 *
 * \param  pool          Page pool
 * \param  size          Request size
 * \param  alignment     Alignment
 *
 * \return Pointer to area, NULL if no area found.
 */
/*****************************************************************************/
static page_area_t *
__find_single_area(page_pool_t * pool, l4_size_t size, l4_addr_t alignment)
{
  int num = __get_free_list(size);
  page_area_t * pa;

  LOGdL(DEBUG_PAGES_FIND_SINGLE, "pool %d: size 0x%08x, alignment 0x%08lx",
        pool->pool, size, alignment);

  /* search in free lists */
  while (num < DMPHYS_NUM_FREE_LISTS)
    {
      pa = pool->free_list[num];
      while (pa != NULL)
	{
	  if (__aligned_size(pa, alignment) >= size)
	    {
	      /* found an area, remove from free list */
	      __remove_free(pool, pa);

#if DEBUG_PAGES_FIND_SINGLE
	      LOG_printf(" using area at 0x%08lx-0x%08lx (%uKB)\n",
                     pa->addr, pa->addr + pa->size, pa->size >> 10);
#endif
	      /* split area if necessary, this also sets the right
	       * address / size in pa */
	      if (__split_area(pool, pa, size, alignment) < 0)
		{
		  PANIC("DMphys: split area failed!");
		  return NULL;
		}

	      SET_AREA_USED(pa);

	      /* return area */
	      return pa;
	    }
	  pa = pa->free_next;
	}

      /* no area found in this free list, try next larger list */
      num++;
    }

  /* nothing found */
  return NULL;
}

/*****************************************************************************/
/**
 * \brief  Allocate pages, try to assemble from smaller page areas
 *
 * \param  pool          Page pool
 * \param  size          Request size
 * \param  max_areas     Max. number of page areas allowed
 * \param  alignment     Alignment of page areas
 *
 * \return Pointer to area list, NULL if allocation failed.
 */
/*****************************************************************************/
static page_area_t *
__allocate_pages(page_pool_t * pool, l4_size_t size, int max_areas,
                 int alignment)
{
  int list,n,found;
  page_area_t * pl, * pa;

  LOGdL(DEBUG_PAGES_ALLOCATE, "pool %d: size 0x%x (%uKb)",
        pool->pool, size, size >> 10);

  n = max_areas;
  pl = NULL;
  while ((size > 0) && (n > 0))
    {
      if (n == 1)
	{
	  /* only one entry in area list left, search a single page area of
	   * the remaining size */
	  pa = __find_single_area(pool, size, alignment);
	  if (pa == NULL)
	    /* nothing found, stop */
	    break;

	  /* add area to allocated area list */
	  pa->ds_next = pl;
	  pl = pa;

	  size -= pa->size;

#if DEBUG_PAGES_ALLOCATE
	  LOG_printf(" %2d: 0x%08lx-0x%08lx, 0x%08x remaining\n",
                 DMPHYS_MAX_DS_AREAS - max_areas,
                 pa->addr, pa->addr + pa->size, size);
#endif
	  /* finished search */
	  break;
	}
      else
	{
	  /* Calculate free list index to start search. The free lists are
	   * devided into sizes of a power of 2, with a start index of
	   * free_list(size) - log2(max_areas) we can ensure that the
	   * area list will not have mor entries than DMPHYS_MAX_DS_AREAS */
	  list = __get_free_list(size) - l4util_bsr(n);
	  if (list < 0)
	    list = 0;

	  found = 0;
	  while ((list < DMPHYS_NUM_FREE_LISTS) && (!found))
	    {
	      l4_size_t pa_size_with_alignment;
	      pa = pool->free_list[list];
	      if (pa
                  && (pa_size_with_alignment = __aligned_size(pa, alignment)))
		{
		  /* found an area, remove from free list */
		  __remove_free(pool, pa);

		  if (pa_size_with_alignment > size)
		    {
		      /* area too large, split */
		      if (__split_area(pool, pa, size, alignment) < 0)
			{
			  PANIC("DMphys: split area failed!");
			  return NULL;
			}
		    }
                  else if (pa_size_with_alignment != pa->size)
                    {
                      /* page area is not too large but has insufficient
                       * alignment, so split it to the proper alignment */
                      if (__split_area(pool, pa, pa_size_with_alignment,
                                       alignment) < 0)
                        {
                          PANIC("DMphys: split area failed!");
                          return NULL;
                        }
                    }

		  SET_AREA_USED(pa);

		  /* add to area list */
		  pa->ds_next = pl;
		  pl = pa;

		  size -= pa->size;
		  found = 1;

#if DEBUG_PAGES_ALLOCATE
		  LOG_printf(" %2d: 0x%08lx-0x%08lx, 0x%08x remaining\n",
                         DMPHYS_MAX_DS_AREAS - max_areas,
                         pa->addr, pa->addr + pa->size, size);
#endif
		}
	      else
		/* no free area in this list, try next larger list */
		list++;
	    }

	  if (!found)
	    /* nothing found, stop */
	    break;

	  n--;
	}
    }

  if (size > 0)
    {
      /* allocation failed, release page areas */
      while (pl != NULL)
	{
	  pa = pl;
	  pl = pl->ds_next;

	  __release_area(pool, pa);
	}
      return NULL;
    }

  /* done */
  return pl;
}

/*****************************************************************************/
/**
 * \brief  Allocate a specific memory area
 *
 * \param  pool          Page pool
 * \param  addr          Memory area address
 * \param  size          Memory area size
 *
 * \return Pointer to area, NULL if area not found or already used.
 */
/*****************************************************************************/
static page_area_t *
__allocate_area(page_pool_t * pool, l4_addr_t addr, l4_size_t size)
{
  page_area_t * pa = pool->area_list;
  l4_addr_t ra;
  l4_size_t rs;

  LOGdL(DEBUG_PAGES_ALLOCATE_AREA, "pool %d: area 0x%08lx-0x%08lx (%uKB)",
        pool->pool, addr, addr + size, size >> 10);

  /* find area */
  while (pa && (addr >= (pa->addr + pa->size)))
    pa = pa->area_next;

  if (pa == NULL)
    /* area not found */
    return NULL;

  if (addr < pa->addr)
    /* area not found */
    return NULL;

  if ((addr + size) > (pa->addr + pa->size))
    /* area overlaps several areas */
    return NULL;

  if (IS_USED_AREA(pa))
    /* area already used */
    return NULL;

  /* remove area from free list */
  __remove_free(pool, pa);

#if DEBUG_PAGES_ALLOCATE_AREA
  LOG_printf(" using area at 0x%08lx-0x%08lx (%uKB)\n",
         pa->addr, pa->addr + pa->size, pa->size >> 10);
#endif

  /* resize area */
  if (addr > pa->addr)
    {
      ra = pa->addr;
      rs = addr - pa->addr;

#if DEBUG_PAGES_ALLOCATE_AREA
      LOG_printf(" free area at beginning: 0x%08lx-0x%08lx\n", ra, ra + rs);
#endif

      pa->addr = addr;
      pa->size -= rs;

      if (__add_free_area(pool, ra, rs) < 0)
	{
	  PANIC("DMphys: failed to add free area");
	  return NULL;
	}
    }

  if ((addr + size) < (pa->addr + pa->size))
    {
      ra = addr + size;
      rs = (pa->addr + pa->size) - ra;

#if DEBUG_PAGES_ALLOCATE_AREA
      LOG_printf(" free area at end:       0x%08lx-0x%08lx\n", ra, ra + rs);
#endif

      pa->size = size;

      if (__add_free_area(pool, ra, rs) < 0)
	{
	  PANIC("DMphys: failed to add free area");
	  return NULL;
	}
    }

  /* mark area used */
  SET_AREA_USED(pa);

  /* done */
  return pa;
}

/*****************************************************************************/
/**
 * \brief  Try to enlarge page area
 *
 * \param  pool          Page pool
 * \param  area          Page area list
 * \param  size          Size to add to the page area
 *
 * \return 0 on success, -1 if something went wrong.
 */
/*****************************************************************************/
static int
__enlarge_area(page_pool_t * pool, page_area_t * area, l4_size_t size)
{
  page_area_t * pa = area->area_next;

  LOGdL(DEBUG_PAGES_ENLARGE, "area 0x%08lx-0x%08lx, add 0x%x",
        area->addr, area->addr + area->size, size);

  if (pa == NULL)
    {
#if DEBUG_PAGES_ENLARGE
      LOG_printf(" no next area!\n");
#endif
      return -1;
    }

  if (IS_USED_AREA(pa))
    /* next area not free */
    return -1;

#if DEBUG_PAGES_ENLARGE
  LOG_printf(" next area 0x%08lx-0x%08lx, size 0x%x\n",
         pa->addr, pa->addr + pa->size, pa->size);
#endif

  if ((area->addr + area->size) != pa->addr)
    /* next area does not start at end of current area */
    return -1;

  if (size > pa->size)
    /* next area not big enough */
    return -1;

  /* ok, we can enlarge area, remove next area from area ande free list */
  if (__remove_free(pool, pa) < 0)
    {
      PANIC("DMphys: remove next area from free list failed!");
      return -1;
    }

  if (__remove_area(pool, pa) < 0)
    {
      PANIC("DMphys: remove next area from free list failed!");
      return -1;
    }

  /* clear out any pages we pass to clients (for security/robustness) */
  dmphys_pages_clear(pa);

  /* enlarge area */
  area->size += size;

#if DEBUG_PAGES_ENLARGE
  LOG_printf(" enlarged, area 0x%08lx-0x%08lx, size 0x%x\n", area->addr,
         area->addr + area->size, area->size);
#endif

  /* re-add rest of next area */
  pa->addr += size;
  pa->size -= size;
  if (pa->size > 0)
    {
      /* add to area list */
      if (__add_area(pool, pa) < 0)
	{
	  PANIC("DMphys: add remainder of next area to area list failed!");
	  return -1;
	}

      /* add to free list */
      if (__add_free(pool, pa) < 0)
	{
	  PANIC("DMphys: add remainder of next area to free list failed!");
	  return -1;
	}

#if DEBUG_PAGES_ENLARGE
      LOG_printf(" remaining next area 0x%08lx-0x%08lx, size 0x%x\n", pa->addr,
             pa->addr + pa->size, pa->size);
#endif
    }
  else
    {
      /* completely used next area, release descriptor */
      __release_area_desc(pa);

#if DEBUG_PAGES_ENLARGE
      LOG_printf(" no next area left\n");
#endif
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** DMphys internal API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Clear newly allocated memory pages for security resons.
 *
 * \param area          areas to clear
 */
/*****************************************************************************/
void
dmphys_pages_clear(page_area_t *area)
{
  while (area != NULL)
    {
      memset((void*)AREA_MAP_ADDR(area), 0, area->size);
      area = area->ds_next;
    }
}

/*****************************************************************************/
/**
 * \brief  Allocate memory pages
 *
 * \param  pool          Page pool
 * \param  size          Requested size
 * \param  alignment     Requested alignment, it is only used if the
 *                       #L4DM_CONTIGUOUS flag is set
 * \param  flags         Flags:
 *                       - #L4DM_CONTIGUOUS      allocate contiguous area,
 *                                               default is to assemble
 *                                               pages from smaller areas
 *                       - #L4DM_MEMPHYS_SUPERPAGES allocate super-pages, this
 *                                               implies #L4DM_CONTIGUOUS
 * \param  prio          Allocation priority
 * \retval areas         Allocated areas
 *
 * \return 0 on success (\a areas contains the list of allocated areas),
 *         error code otherwise:
 *         - -#L4_ENOMEM  no memory available
 */
/*****************************************************************************/
int
dmphys_pages_allocate(page_pool_t * pool, l4_size_t size, l4_addr_t alignment,
		      l4_uint32_t flags, int prio, page_area_t ** areas)
{
  page_area_t * pa;

  /* check available memory */
  if (pool->free < size)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB",
	    size / 1024, pool->free / 1024);
      return -L4_ENOMEM;
    }

  if ((prio == PAGES_USER) && ((pool->free - size) < pool->reserved))
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB, res. %uKB",
	    size / 1024, pool->free / 1024, pool->reserved / 1024);
      return -L4_ENOMEM ;
    }

  /* areas must be at least page-aligned */
  if (alignment < DMPHYS_PAGESIZE)
    alignment = DMPHYS_PAGESIZE;

  if (flags & L4DM_MEMPHYS_SUPERPAGES)
    {
      /* super-pages requested, adapt size and alignment */
      if (alignment < DMPHYS_SUPERPAGESIZE)
	alignment = DMPHYS_SUPERPAGESIZE;

      size = (size + DMPHYS_SUPERPAGESIZE - 1) & ~(DMPHYS_SUPERPAGESIZE - 1);
    }

  /* allocate pages */
  if (flags & L4DM_CONTIGUOUS)
    pa = __find_single_area(pool, size, alignment);
  else
    pa = __allocate_pages(pool, size, DMPHYS_MAX_DS_AREAS, alignment);

  *areas = pa;

  /* done */
  if (pa == NULL)
    return -L4_ENOMEM;
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Allocate a specific memory area
 *
 * \param  pool          Page pool
 * \param  addr          Area address
 * \param  size          Area size
 * \param  prio          Allocation priority
 *
 * \retval area          Allocated area
 *
 * \return 0 on success (\a areas contains the allocated areas),
 *         error code otherwise:
 *         - -#L4_ENOMEM  memory area not available
 */
/*****************************************************************************/
int
dmphys_pages_allocate_area(page_pool_t * pool, l4_addr_t addr, l4_size_t size,
			   int prio, page_area_t ** area)
{
  page_area_t * pa;

  /* check available memory */
  if (pool->free < size)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB",
	    size / 1024, pool->free / 1024);
      return -L4_ENOMEM;
    }

  if ((prio == PAGES_USER) && ((pool->free - size) < pool->reserved))
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB, res. %uKB",
	    size / 1024, pool->free / 1024, pool->reserved / 1024);
      return -L4_ENOMEM;
    }

  /* allocate area */
  pa = __allocate_area(pool, addr, size);

  *area = pa;

  /* done */
  if (pa == NULL)
    return -L4_ENOMEM;
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Release memory pages
 *
 * \param  pool          Page pool
 * \param  areas         Page area list
 */
/*****************************************************************************/
void
dmphys_pages_release(page_pool_t * pool, page_area_t * areas)
{
  page_area_t * pa;

  if (pool == NULL)
    return;

  /* release pages */
  while (areas != NULL)
    {
      pa = areas;
      areas = areas->ds_next;
      __release_area(pool, pa);
    }

  /* done */
}

/*****************************************************************************/
/**
 * \brief  Try to enlarge page area.
 *
 * \param  pool          Page pool
 * \param  area          Page area
 * \param  size          Size to add to the page area
 * \param  prio          Allocation priority
 *
 * \return 0 on success (enlarged page area by \a size),
 *         error code otherwise:
 *         - -#L4_EINVAL  invalid page area / page pool
 *         - -#L4_ENOMEM  memory area not available
 */
/*****************************************************************************/
int
dmphys_pages_enlarge(page_pool_t * pool, page_area_t * area, l4_size_t size,
		     int prio)
{
  if ((pool == NULL) || (area == NULL))
    return -L4_EINVAL;

  if (IS_UNUSED_AREA(area))
    return -L4_EINVAL;

  /* check available memory */
  if (pool->free < size)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB",
	    size / 1024, pool->free / 1024);
      return -L4_ENOMEM;
    }

  if ((prio == PAGES_USER) && ((pool->free - size) < pool->reserved))
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB, res. %uKB",
	    size / 1024, pool->free / 1024, pool->reserved / 1024);
      return -L4_ENOMEM;
    }

  /* area must be a single area  */
  Assert(area->ds_next == NULL);

  /* try to enlarge */
  if (__enlarge_area(pool, area, size) < 0)
    return -L4_ENOMEM;
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Add more pages to page list
 *
 * \param  pool          Page pool
 * \param  pages         Page list
 * \param  size          Size to add to the page list
 * \param  prio          Allocation priority
 *
 * \return 0 on success (added pages to page list), error code otherwise:
 *         - -#L4_EINVAL  invalid page area / page pool
 *         - -#L4_ENOMEM  memory area not available
 */
/*****************************************************************************/
int
dmphys_pages_add(page_pool_t * pool, page_area_t * pages, l4_size_t size,
		 int prio)
{
  int num_old,max_new;
  page_area_t * new_pages;
  page_area_t * pa;

  if ((pool == NULL) || (pages == NULL))
    return -L4_EINVAL;

  /* check available memory */
  if (pool->free < size)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB",
	    size / 1024, pool->free / 1024);
      return -L4_ENOMEM;
    }

  if ((prio == PAGES_USER) && ((pool->free - size) < pool->reserved))
    {
      LOGdL(DEBUG_ERRORS, "DMphys: no memory, size %uKB, left %uKB, res. %uKB",
	    size / 1024, pool->free / 1024, pool->reserved / 1024);
      return -L4_ENOMEM;
    }

  /* get max. number of page areas we can add */
  num_old = dmphys_pages_get_num(pages);
  if (num_old >= DMPHYS_MAX_DS_AREAS)
    max_new = 1;
  else
    max_new = DMPHYS_MAX_DS_AREAS - num_old;

  LOGdL(DEBUG_PAGES_ALLOCATE_ADD, "add 0x%x, max. %d area(s)", size, max_new);

  /* allocate new pages */
  new_pages = __allocate_pages(pool, size, max_new, DMPHYS_PAGESIZE);
  if (new_pages == NULL)
    return -L4_ENOMEM;

  /* clear out any pages we pass to clients (for security/robustness) */
  dmphys_pages_clear(new_pages);

#if DEBUG_PAGES_ALLOCATE_ADD
  LOGL("new page areas:");
  pa = new_pages;
  while (pa != NULL)
    {
      LOG_printf("  0x%08lx - 0x%08lx (%u bytes)\n",
             pa->addr, pa->addr + pa->size, pa->size);
      pa = pa->ds_next;
    }
#endif

  /* add pages to page list */
  pa = pages;
  while (pa->ds_next != NULL)
    pa = pa->ds_next;
  pa->ds_next = new_pages;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Shrink page area list
 *
 * \param  pool          Page pool
 * \param  pages         Page area list
 * \param  size          New size
 *
 * \return 0 on success (shrinked page list), error code otherwise:
 *         - -#L4_EINVAL invalid page area / page pool / size
 */
/*****************************************************************************/
int
dmphys_pages_shrink(page_pool_t * pool, page_area_t * pages, l4_size_t size)
{
  page_area_t * last, * pa;
  l4_size_t last_size,rs;
  l4_addr_t ra;

  if (pool == NULL)
    return -L4_EINVAL;

  /* find new last page area */
  last = dmphys_pages_find_offset(pages, size - 1, (l4_offs_t *)&last_size);
  if (last == NULL)
    return -L4_EINVAL;
  last_size++;

  LOGdL(DEBUG_PAGES_SHRINK,
        "new size 0x%x, new last area 0x%08lx-0x%08lx, new size 0x%x",
        size, last->addr, last->addr + last->size, last_size);

  /* shrink last page area */
  if (last->size > last_size)
    {
      ra = last->addr + last_size;
      rs = last->size - last_size;

      /* unmap freed area */
      dmphys_unmap_area(ra, rs);

#if DEBUG_PAGES_SHRINK
      LOG_printf(" split new last area, free area 0x%08lx-0x%08lx, size 0x%x\n",
             ra, ra + rs, rs);
#endif

      last->size = last_size;
      if (__add_free_area(pool, ra, rs) < 0)
	{
	  Panic("DMphys: add free page area failed!");
	  return -L4_EINVAL;
	}
    }

  /* release the rest of the page list */
  while (last->ds_next != NULL)
    {
      pa = last->ds_next;
      last->ds_next = pa->ds_next;
#if DEBUG_PAGES_SHRINK
      LOG_printf(" free area 0x%08lx-0x%08lx, size 0x%x\n", pa->addr,
             pa->addr + pa->size, pa->size);
#endif

      /* unmap */
      dmphys_unmap_area(pa->addr, pa->size);

      /* release */
      __release_area(pool, pa);
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** Init page area handling
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Add free page area to pool area list
 *
 * \param  pool          Page pool number
 * \param  addr          Area start address
 * \param  size          Area size
 *
 * \return 0 on success (area added to page area list and the appropriate
 *         free list), -1 if something went wrong.
 */
/*****************************************************************************/
int
dmphys_pages_add_free_area(int pool, l4_addr_t addr, l4_size_t size)
{
  int ret;

  if ((pool < 0) || (pool >= DMPHYS_NUM_POOLS))
    return -1;

  /* add page area */
  ret = __add_free_area(&page_pools[pool], addr, size);
  if (ret < 0)
    return ret;

  page_pools[pool].size += size;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Init page area handling.
 *
 * \return 0 on success, -1 if initialization failed.
 */
/*****************************************************************************/
int
dmphys_pages_init(void)
{
  int i,j;

  /* initialize page area descriptor slab cache */
  if (l4slab_cache_init(&area_cache, sizeof(page_area_t), 0,
			dmphys_internal_alloc_grow,
			dmphys_internal_alloc_release) < 0)
    return -1;

  l4slab_set_data(&area_cache, area_cache_name);

  /* init pool descriptors */
  for (i = 0; i < DMPHYS_NUM_POOLS; i++)
    {
      page_pools[i].pool = i;
      page_pools[i].name = NULL;
      page_pools[i].size = 0;
      page_pools[i].free = 0;
      page_pools[i].reserved = 0;
      page_pools[i].area_list = NULL;
      for (j = 0; j < DMPHYS_NUM_FREE_LISTS; j++)
	page_pools[i].free_list[j] = NULL;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Init page pool descriptor
 *
 * \param  pool          Page pool number
 * \param  reserved      Reserved memory in page pool
 * \param  name          Page pool name (optional)
 */
/*****************************************************************************/
void
dmphys_pages_init_pool(int pool, l4_size_t reserved, char * name)
{
  if ((pool < 0) || (pool >= DMPHYS_NUM_POOLS))
    return;

  /* setup descriptor */
  page_pools[pool].reserved = reserved;
  page_pools[pool].name = name;
}

/*****************************************************************************
 *** Misc
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return page pool descriptor
 *
 * \param  pool          Pool number
 *
 * \return Pointer to page pool descriptor, NULL if invalid pool number
 */
/*****************************************************************************/
page_pool_t *
dmphys_get_page_pool(int pool)
{
  if ((pool < 0) || (pool >= DMPHYS_NUM_POOLS))
    return NULL;
  else
    return &page_pools[pool];
}

/*****************************************************************************/
/**
 * \brief  Return default memory pool
 *
 * \return Pointer to default memory pool.
 */
/*****************************************************************************/
page_pool_t *
dmphys_get_default_pool(void)
{
  return DEFAULT_POOL;
}


/*****************************************************************************/
/**
 * \brief DEBUG: iterator function:
 *               print out DS id if a given page area is within the dataspace
 */
/*****************************************************************************/
static void
__debug_print_ds_for_page_area(dmphys_dataspace_t * ds, void * data)
{
  page_area_t *pa = data, *p_ds = ds->pages;

  while (p_ds)
    {
      if (pa == p_ds)
        {
          LOG_printf(" %d(" l4util_idfmt ")",
                     dmphys_ds_get_id(ds),
                     l4util_idstr(dsmlib_get_owner(ds->desc)));
          break;
        }
      p_ds = p_ds->ds_next;
    }
}

/*****************************************************************************/
/**
 * \brief DEBUG: dump used memory pools
 */
/*****************************************************************************/
void
dmphys_pages_dump_used_pools(void)
{
  int i;
  page_pool_t * pool;
  page_area_t * pa;

  LOG_printf("DMphys memory pools:\n");
  for (i = 0; i < DMPHYS_NUM_POOLS; i++)
    {
      if (page_pools[i].size != 0)
	{
	  pool = &page_pools[i];
	  pa = pool->area_list;

	  if ((pool->name != NULL) && (strlen(pool->name) > 0))
	    LOG_printf(" pool %d (%s):\n", pool->pool, pool->name);
	  else
	    LOG_printf(" pool %d:\n", pool->pool);
	  LOG_printf(" size: %6uKB total, %6uKB free, %3uKB reserved\n",
                 pool->size / 1024, pool->free / 1024, pool->reserved / 1024);

	  while (pa)
	    {
	      LOG_printf("  0x%08lx-0x%08lx (%6uKB, %4uMB) %s DS:",
                     pa->addr, pa->addr + pa->size, 
		     (pa->size+(1<<9)-1) >> 10, (pa->size+(1<<19)-1) >> 20,
                     IS_USED_AREA(pa) ? "used" : "free");

              dmphys_ds_iterate(__debug_print_ds_for_page_area,
                                pa, L4_INVALID_ID, 0);

              LOG_printf("\n");

	      pa = pa->area_next;
	    }
	}
    }
}

/*****************************************************************************/
/**
 * \brief  DEBUG: dump page area list
 *
 * \param  pool          Page pool
 */
/*****************************************************************************/
void
dmphys_pages_dump_areas(page_pool_t * pool)
{
  page_area_t * pa = pool->area_list;

  /* dump area list */
  LOG_printf("DMphys memory pool areas:\n");
  if ((pool->name != NULL) && (strlen(pool->name) > 0))
    LOG_printf("  pool %d (%s)\n", pool->pool, pool->name);
  else
    LOG_printf("  pool %d\n", pool->pool);
  LOG_printf("  size: %6uKB total, %6uKB free, %3uKB reserved\n",
         pool->size / 1024, pool->free / 1024, pool->reserved / 1024);

  while (pa)
    {
      LOG_printf("    0x%08lx-0x%08lx (%6uKB, %4uMB), ",
             pa->addr, pa->addr + pa->size,
	     (pa->size+(1<<9)-1) >> 10, (pa->size+(1<<19)-1) >> 20);
      if (IS_UNUSED_AREA(pa))
	LOG_printf("free\n");
      else
	LOG_printf("used\n");
      pa = pa->area_next;
    }
}

/*****************************************************************************/
/**
 * \brief  DEBUG: dump free lists
 *
 * \param  pool          Page pool
 */
/*****************************************************************************/
void
dmphys_pages_dump_free(page_pool_t * pool)
{
  int i;
  page_area_t * pa;

  /* dump free lists */
  LOG_printf("DMphys memory pool free lists:\n");
  if ((pool->name != NULL) && (strlen(pool->name) > 0))
    LOG_printf("  pool %d (%s)\n", pool->pool, pool->name);
  else
    LOG_printf("  pool %d\n", pool->pool);
  LOG_printf("  size: %6uKB total, %6uKB free, %3uKB reserved\n",
         pool->size / 1024, pool->free / 1024, pool->reserved / 1024);

  for (i = 0; i < DMPHYS_NUM_FREE_LISTS; i++)
    {
      LOG_printf("    %2d (sizes %6uKB - %6uKB):\n",i,
             (1 << (DMPHYS_LOG2_PAGESIZE + i)) >> 10,
             (1 << (DMPHYS_LOG2_PAGESIZE + i + 1)) >> 10);
      pa = pool->free_list[i];
      while (pa)
	{
	  LOG_printf("       0x%08lx-0x%08lx (%6uKB, %4uMB)\n",
                 pa->addr, pa->addr + pa->size,
		 (pa->size+(1<<9)-1) >> 10, (pa->size+(1<<19)-1) >> 20);
	  pa = pa->free_next;
	}
    }
}

/*****************************************************************************/
/**
 * \brief  DEBUG: show dataspace page area list
 *
 * \param  list          Area list head
 */
/*****************************************************************************/
void
dmphys_pages_list(page_area_t * list)
{
  page_area_t * area = list;

  /* show list dataspace page area list */
  while (area != NULL)
    {
      LOG_printf("    0x%08lx - 0x%08lx (%uKB)\n",
             area->addr, area->addr + area->size, area->size / 1024);
      area = area->ds_next;
    }
}
