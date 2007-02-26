/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/region.c
 * \brief  Region list handling.
 *
 * \date   06/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* L4RM includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/* VM size */
static l4_addr_t vm_start;  ///< VM start address
static l4_addr_t vm_end;    ///< VM end address

/**
 * pointer to region list 
 */
static l4rm_region_desc_t * head = NULL;

/**
 * region list lock 
 */
l4lock_t region_list_lock = L4LOCK_UNLOCKED_INITIALIZER;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Insert region into region list.
 * 
 * \param  region        new region descriptor 
 * \param  rp            free region in which the new region should be 
 *                       inserted
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_NOMEM  out of memory allocating region descriptor
 *
 * Insert region into region list at the place described by rp.
 */
/*****************************************************************************/ 
static int
__insert_region(l4rm_region_desc_t * region, 
		l4rm_region_desc_t * rp)
{
  l4rm_region_desc_t * tmp;

  Assert(!FLAGS_EQUAL(region,rp));

  /* insert */
  if (rp->start == region->start) 
    {
      /* new region starts at beginning of the free region */
      if (rp->end == region->end)
	{
	  /* replace descriptor of free region with new descriptor */
	  if (rp == head)
	    {
	      region->prev = NULL;
	      region->next = rp->next;
	      if (rp->next)
		rp->next->prev = region;
	      head = region;
	    }
	  else
	    {
	      region->prev = rp->prev;
	      region->prev->next = region;
	      region->next = rp->next;
	      if (rp->next)
		rp->next->prev = region;
	    }

	  /* release region descriptor */
	  l4rm_region_desc_free(rp);
	}
      else
	{
	  /* shrink free region and insert new region */
	  rp->start = region->end + 1;
	  if (rp == head)
	    {
	      region->prev = NULL;
	      region->next = rp;
	      rp->prev = region;
	      head = region;
	    }
	  else
	    {
	      region->prev = rp->prev;
	      rp->prev->next = region;
	      region->next = rp;
	      rp->prev = region;
	    }
	}
    }
  else if (rp->end == region->end)
    {
      /* new region ends at the end of the free region, shrink free region */
      rp->end = region->start - 1;
      region->next = rp->next;
      region->prev = rp;
      if (region->next)
	region->next->prev = region;
      rp->next = region;
    }
  else
    {
      /* split free region */
      tmp =  l4rm_region_desc_alloc();
      if (tmp == NULL)
	/* out of memory */
	return -L4_ENOMEM;

      /* new free region behind new region */
      tmp->start = region->end + 1;
      tmp->end = rp->end;
      tmp->flags = rp->flags;
      tmp->prev = region;
      tmp->next = rp->next;
      if (tmp->next)
	tmp->next->prev = tmp;

      rp->end = region->start - 1;
      rp->next = region;
      
      region->prev = rp;
      region->next = tmp;
    } 

  /* done */
  return 0;
} 

/*****************************************************************************/
/**
 * \brief Change the flags of region.
 * 
 * \param  region        region descriptor
 * \param  new_flags     new flags for region
 *	
 * Change the flags of region to new_flags. Check if the previous/next
 * region then have the same flags and join.
 * 
 * \note After __modify_region returned the pointer region might be invalid
 *       because the region was joined with with the previous/next region.
 */
/*****************************************************************************/ 
static void
__modify_region(l4rm_region_desc_t * region, 
		l4_uint32_t new_flags)
{
  l4rm_region_desc_t * tmp;

  /* set flags */
  region->flags = new_flags;

  if (IS_ATTACHED_REGION(region))
    /* we can't join attached regions */
    return;

#if DEBUG_REGION_MODIFY
  if (region->prev)
    printf("prev   0x%08x-0x%08x, flags 0x%08x\n",
	   region->prev->start,region->prev->end,region->prev->flags);
  printf("region 0x%08x-0x%08x, flags 0x%08x\n",
	 region->start,region->end,region->flags);
  if (region->next)
    printf("next   0x%08x-0x%08x, flags 0x%08x\n",
	   region->next->start,region->next->end,region->next->flags);
#endif

  /* check previous/next region */
  if (region->prev && (FLAGS_EQUAL(region->prev,region)))
    {
      /* join with previous region */
      tmp = region->prev;
      tmp->end = region->end;
      tmp->next = region->next;
      if (tmp->next)
	tmp->next->prev = tmp;

      /* release region descriptor */
      l4rm_region_desc_free(region);

      region = tmp;
    }

  if (region->next && (FLAGS_EQUAL(region,region->next)))
    {
      /* join with next region */
      tmp = region->next;
      tmp->start = region->start;
      if (region == head)
	{
	  tmp->prev = NULL;
	  head = tmp;
	}
      else
	{
	  tmp->prev = region->prev;
	  tmp->prev->next = tmp;
	}

      /* release region descriptor */
      l4rm_region_desc_free(region);
    }

  /* done */
}

/*****************************************************************************/
/**
 * \brief Search region.
 * 
 * \param  rp            region descriptor
 *	
 * \return region \a rp fits into, NULL if \a rp describes invalid address 
 *         range or overlaps serveral existing regions.
 *
 * Search region which contains address range described by rp. 
 */
/*****************************************************************************/ 
static l4rm_region_desc_t *
__find_region(l4rm_region_desc_t * rp)
{
  l4rm_region_desc_t * tmp = head;

  /* sanity checks */
  if ((rp->start > rp->end) ||
      (rp->start < vm_start) || (rp->start > vm_end) ||
      (rp->end < vm_start) || (rp->end > vm_end))
    /* invalid address region */
    return NULL;

#if DEBUG_REGION_FIND
  INFO("L4RM: find <0x%08x-0x%08x>\n",rp->start,rp->end);
#endif

  /* find region descriptor the new region fits into */
  while (tmp && (tmp->end < rp->start))
    tmp = tmp->next;

  if (tmp == NULL)
    {
      Panic("L4RM: corrupted region list!");
      return NULL;
    }

#if DEBUG_REGION_FIND
  DMSG("  found <0x%08x-0x%08x>\n",tmp->start,tmp->end);
#endif

  if (rp->end > tmp->end)
    /* overlapping areas */
    return NULL;

  /* found */
  return tmp;
}

/*****************************************************************************
 * L4RM internal library functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Setup region table.
 * 
 * \retval 0 on success, -1 if initialization failed.
 */
/*****************************************************************************/ 
int
l4rm_init_regions(void)
{
  int ret;

  /* init region descriptor allocation */
  ret = l4rm_region_alloc_init();
  if (ret < 0)
    return -1;

  /* setup first region which describes the whole user address space */
  vm_start = l4rm_get_vm_start();
  vm_end = l4rm_get_vm_end();

  head = l4rm_region_desc_alloc();
  if (head == NULL)
    return -1;

  head->start = vm_start;
  head->end = vm_end;
  head->prev = NULL;
  head->next = NULL;
  SET_USED(head);
  SET_FREE(head);
  SET_AREA(head,L4RM_DEFAULT_REGION_AREA);

#if DEBUG_REGION_INIT
  INFO("L4RM: vm 0x%08x-0x%08x\n",head->start,head->end + 1);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Insert region into region list.
 * 
 * \param  region        region descriptor
 *	
 * \return 0 on success (region inserted into region list), error code 
 *         otherwise:
 *         - \c -L4_EUSED   address region already used 
 *         - \c -L4_EINVAL  invalid region descriptor 
 *         - \c -L4_ENOMEM  out of memory allocating region descriptor
 *
 * Insert region into region list. The region list ist sorted, starting
 * with the lowest address.
 */
/*****************************************************************************/ 
int 
l4rm_insert_region(l4rm_region_desc_t * region)
{
  l4rm_region_desc_t * rp;

#if DEBUG_REGION_INSERT
  INFO("\n");
  DMSG("  region <0x%08x-0x%08x>\n",region->start,region->end);
  DMSG("  area 0x%05x\n",REGION_AREA(region));
#endif

  rp = __find_region(region);
  if (rp == NULL)
    /* invalid region */
    return -L4_EUSED;

#if DEBUG_REGION_INSERT
  DMSG("  in region <0x%08x-0x%08x>\n",rp->start,rp->end);
#endif

  if (IS_ATTACHED_REGION(rp))
    /* region already used */
    return -L4_EUSED;

  if (REGION_AREA(rp) != REGION_AREA(region))
    /* region reserved by someone else */
    return -L4_EUSED;

  /* insert region */
  return __insert_region(region,rp);
}
  
/*****************************************************************************/
/**
 * \brief Mark region free.
 * 
 * \param  region        region descriptor
 */
/*****************************************************************************/ 
void
l4rm_free_region(l4rm_region_desc_t * region)
{
  /* sanity checks */
  if (IS_UNUSED_REGION(region) || IS_FREE_REGION(region))
    return;

  /* mark region free */
  SET_FREE(region);

  /* modify region list */
  return __modify_region(region,region->flags);
}

/*****************************************************************************/
/**
 * \brief Insert area into region list.
 * 
 * \param  region        region descriptor 
 *	
 * \return 0 on success (area inserted into region list), error code 
 *         otherwise:
 *         - \c -L4_EUSED   address region already used 
 *         - \c -L4_EINVAL  invalid region descriptor
 *         - \c -L4_ENOMEM  out of memory allocating region descriptor
 */
/*****************************************************************************/ 
int
l4rm_insert_area(l4rm_region_desc_t * region)
{
  l4rm_region_desc_t * rp;

#if DEBUG_REGION_INSERT
  INFO("\n");
  DMSG("  region <0x%08x-0x%08x>\n",region->start,region->end);
  DMSG("  area 0x%05x\n",REGION_AREA(region));
#endif

  rp = __find_region(region);
  if (rp == NULL)
    /* invalid region */
    return -L4_EINVAL;

#if DEBUG_REGION_INSERT
  DMSG("  in region <0x%08x-0x%08x>\n",rp->start,rp->end);
#endif

  if (IS_ATTACHED_REGION(rp))
    /* region already used */
    return -L4_EUSED;

  Assert(REGION_AREA(rp) != REGION_AREA(region));

  /* insert region */
  return __insert_region(region,rp);
}

/*****************************************************************************/
/**
 * \brief Reset the area id.
 * 
 * \param  area          area id
 *
 * Reset the area id of all regions which belong to area \a area to the 
 * default area id.
 */
/*****************************************************************************/ 
void 
l4rm_reset_area(l4_uint32_t area)
{
  l4rm_region_desc_t * rp;
  int done;

  /* reset */
  done = 0;
  do
    {
      rp = head;

      while (rp && (REGION_AREA(rp) != area))
	rp = rp->next;

      if (!rp)
	/* done */
	done = 1;
      else
	{
	  /* reset area id */
	  SET_AREA(rp,L4RM_DEFAULT_REGION_AREA);
	  CLEAR_REGION_FLAG(rp,REGION_RESERVED);
	  __modify_region(rp,rp->flags);
	}

      /* rp might be invalid after l4rm_modify_region, start from the 
       * list head again :( */
    }
  while (!done);
}  

/*****************************************************************************/
/**
 * \brief Find free region.
 * 
 * \param  size          size of requested region
 * \param  area          area id 
 * \param  flags         flags:
 *                       - \c L4RM_LOG2_ALIGNED find a 2^(log2(size) + 1)
 *                                              aligned region
 *                       - \c L4RM_LOG2_ALLOC   allocate the whole 
 *                                              2^(log2(size) + 1) sized area
 * \retval addr          start address of region
 *	
 * \return 0 on success, -L4_ENOTFOUND if no region found.
 *
 *  Find free region of size \a size in area \a area.
 */
/*****************************************************************************/ 
int
l4rm_find_free_region(l4_size_t size, 
		      l4_uint32_t area, 
		      l4_uint32_t flags,
		      l4_addr_t * addr)
{
  l4rm_region_desc_t * rp = head;
  int align;
  l4_uint32_t a_size = 0;
  l4_addr_t a_addr,offs;

  if (flags & L4RM_LOG2_ALIGNED)
    {
      /* calculate alignment */
      align = bsr(size);
      if (size > (1UL << align))
	align++;

      a_size = 1UL << align; 

      if (flags & L4RM_LOG2_ALLOC)
	size = a_size;
    }

  while (rp)
    {
      if (IS_FREE_REGION(rp) && (REGION_AREA(rp) == area))
	{
	  if (flags & L4RM_LOG2_ALIGNED)
	    {
	      a_addr = (rp->start + a_size - 1) & ~(a_size - 1);
	      offs = a_addr - rp->start;
	      if ((rp->end - rp->start + 1) >= (offs + size))
		{
		  /* found big enough region */
		  *addr = a_addr;
		  return 0;
		}
	    }
	  else
	    {
	      if ((rp->end - rp->start + 1) >= size)
		{
		  /* found big enough region */
		  *addr = rp->start;
		  return 0;
		}
	    }
	}
      rp = rp->next;
    }

  /* nothing found */
  return -L4_ENOTFOUND;
}

/*****************************************************************************/
/**
 * \brief  Find region which given address belongs to
 * 
 * \param  addr          VM address
 *	
 * \return pointer to region NULL if not found
 */
/*****************************************************************************/ 
l4rm_region_desc_t *
l4rm_find_region(l4_addr_t addr)
{
  l4rm_region_desc_t rp;

  /* Argh: we must manually search the given address in the region list, 
   *       reserved areas are not inserted into the region tree
   */
  rp.start = rp.end = addr;
  return __find_region(&rp);
}

/*****************************************************************************/
/**
 * \brief Get access rights for a dataspace
 * 
 * \param  ds            Dataspace id
 *	
 * \return Access right bit mask, 0 if dataspace not attached.
 *
 * Walk the region list to get the access rights to the given dataspace. 
 */
/*****************************************************************************/ 
l4_uint32_t
l4rm_get_access_rights(l4dm_dataspace_t * ds)
{
  l4_uint32_t rights = 0;
  l4rm_region_desc_t * rp = head;

  /* walk region list */
  while(rp)
    {
      if (IS_ATTACHED_REGION(rp) && (l4dm_dataspace_equal(*ds,rp->ds)))
	rights |= rp->rights;

      rp = rp->next;
    }

  /* done */
  return rights;
}

/*****************************************************************************/
/**
 * \brief DEBUG: show region list
 */
/*****************************************************************************/ 
void
l4rm_show_region_list(void)
{
  l4rm_region_desc_t * rp = head;

  Msg("region list:\n");
  while (rp)
    {
      Msg("  area 0x%05x: 0x%08x - 0x%08x [%7dKiB]:",
	  REGION_AREA(rp),rp->start,rp->end, (rp->end - rp->start + 1) >> 10);
      if (IS_ATTACHED_REGION(rp))
	Msg(" %2d at %2x.%x\n",
	    rp->ds.id,rp->ds.manager.id.task,rp->ds.manager.id.lthread);
      else if (IS_FREE_REGION(rp) || IS_RESERVED_AREA(rp))
	{
	  if (IS_RESERVED_AREA(rp))
	    Msg(" reserved");

	  if (IS_FREE_REGION(rp))
	    Msg(" free");

	  Msg("\n");
	}
      else
	Msg(" unknown\n");

      rp = rp->next;
    }
  Msg("\n");
}
