/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/reserve.c
 * \brief  VM area reservation.
 *
 * \date   08/22/2000
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
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Mark address area reserved.
 * 
 * \param  addr          region start address (ADDR_FIND .. find suitable area)
 * \param  size          region size
 * \param  flags         flags:
 *                       - #L4RM_LOG2_ALIGNED 
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_LOG2_ALLOC   
 *                         reserve the whole 2^(log2(size) + 1) sized region
 *                       - #L4RM_RESERVE_USED mark reserved area as used
 * \retval addr          start address used
 * \retval area          area id 
 *	
 * \return 0 on success (reserved area), error code otherwise:
 *         - \c -L4_ENOTFOUND  no suitable address area found
 *         - \c -L4_ENOMEM     out of memory allocation region descriptor
 *         - \c -L4_EUSED      address region already used 
 *         - \c -L4_EINVAL     invalid area
 *
 * Mark address area reserved. If addr == ADDR_FIND, find a suitable area.
 */
/*****************************************************************************/ 
static int
__reserve(l4_addr_t * addr, 
	  l4_size_t size, 
	  l4_uint32_t flags, 
	  l4_uint32_t * area)
{
  int ret;
  l4rm_region_desc_t *r;
  l4_uint32_t align;

#if DEBUG_REGION_RESERVE
  INFO("addr 0x%08x, size 0x%08x\n",*addr,size);
#endif

  /* lock region list */
  l4rm_lock_region_list_direct(flags);

  if (*addr == ADDR_FIND)
    {
      /* find free region of specified size */
      ret = l4rm_find_free_region(size,L4RM_DEFAULT_REGION_AREA,flags,addr);
      if (ret < 0)
	{
	  /* nothing found */
	  l4rm_unlock_region_list_direct(flags);
	  return -L4_ENOTFOUND;	  
	}
    }
  else
    {
      /* check if specified region is available */
      r = l4rm_find_region(*addr);
      if (r == NULL)
	{
	  l4rm_unlock_region_list_direct(flags);
	  return -L4_EINVAL;
	}

#if DEBUG_REGION_RESERVE
      DMSG("  in region <0x%08x-0x%08x>, area 0x%05x\n",
	   r->start,r->end,REGION_AREA(r));
#endif

      if (!IS_FREE_REGION(r) || IS_RESERVED_AREA(r) || 
	  ((*addr + size - 1) > r->end))
	{
	  /* region not available */
	  l4rm_unlock_region_list_direct(flags);
	  return -L4_EUSED;
	}
   }
      
  /* allocate new region descriptor */
  r = l4rm_region_desc_alloc();
  if (r == NULL)
    {
      /* out of memory */
      l4rm_unlock_region_list_direct(flags);
      return -L4_ENOMEM;
    }

  /* the area id is the first page number of that area */
  *area = *addr >> L4_PAGESHIFT;

#if DEBUG_REGION_RESERVE
  INFO("area 0x%05x\n",*area);
#endif

  /* setup new region */
  if ((flags & L4RM_LOG2_ALIGNED) && (flags & L4RM_LOG2_ALLOC))
    {
      /* round size to next log2 size */
      align = bsr(size);
      if (size > (1UL << align))
	align++;

      size = 1UL << align;
    }
  r->start = *addr;
  r->end = *addr + size - 1;
  SET_AREA(r,*area);
  SET_RESERVED(r);
  if (!(flags & L4RM_RESERVE_USED))
    SET_FREE(r);
  
  /* insert region */
  ret = l4rm_insert_area(r);
  if (ret < 0)
    {
      /* insert failed, region probably already used  */
      l4rm_region_desc_free(r);
      l4rm_unlock_region_list_direct(flags);
      return ret;
    }
  
  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);
  
  /* done */
  return 0;
}

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Reserve area.
 * 
 * \param  size          Area size
 * \param  flags         Flags:
 *                       - #L4RM_LOG2_ALIGNED 
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_LOG2_ALLOC   
 *                         reserve the whole 2^(log2(size) + 1) sized region
 *                       - #L4RM_RESERVE_USED mark reserved area as used
 * \retval addr          Start address 
 * \retval area          Area id
 *	
 * \return 0 on success (reserved area at \a addr), error code otherwise:
 *         - \c -L4_ENOTFOUND  no free area of size \a size found
 *         - \c -L4_ENOMEM     out of memory allocating descriptors
 *
 * Reserve area of size \a size. The reserved area will not be used 
 * in l4rm_attach or l4rm_attach_to_region, dataspace can only be attached 
 * to this area calling l4rm_area_attach or l4rm_area_attach_to region with 
 * the appropriate area id. 
 */
/*****************************************************************************/ 
int
l4rm_area_reserve(l4_size_t size, 
		  l4_uint32_t flags, 
		  l4_addr_t * addr, 
		  l4_uint32_t * area)
{

  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* reserve */
  *addr = ADDR_FIND;
  return __reserve(addr,size,flags,area);
}

/*****************************************************************************/
/**
 * \brief Reserve specified area.
 * 
 * \param  addr          Address
 * \param  size          Area Size
 * \param  flags         Flags:
 *                       - #L4RM_RESERVE_USED mark reserved area as used
 * \retval area          Area id
 *	
 * \return 0 on success (reserved area at \a addr), error code otherwise:
 *         - \c -L4_EUSED   specified are aalready used
 *         - \c -L4_ENOMEM  out of memory allocating descriptors
 *
 * Reserve area at \a addr.
 */
/*****************************************************************************/ 
int
l4rm_area_reserve_region(l4_addr_t addr, 
			 l4_size_t size, 
			 l4_uint32_t flags,
			 l4_uint32_t * area)
{
  l4_offs_t offs;
  l4_addr_t a = addr;

  /* align address */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      Msg("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
          a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* reserve */
  return __reserve(&a,size,flags,area);
}

/*****************************************************************************
 * Call __reserve without locking the region list. It is necessary during the
 * task startup, where the thread and lock libraries are not initialized yet.
 * DO NOT USE AT OTHER PLACES!
 *****************************************************************************/

/*****************************************************************************
 * l4rm_direct_area_reserve
 *****************************************************************************/
int
l4rm_direct_area_reserve(l4_size_t size, 
			 l4_uint32_t flags, 
			 l4_addr_t * addr, 
                         l4_uint32_t * area)
{
  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* reserve */
  *addr = ADDR_FIND;
  return __reserve(addr,size,flags | MODIFY_DIRECT,area);
}

/*****************************************************************************
 * l4rm_direct_area_reserve_region
 *****************************************************************************/
int
l4rm_direct_area_reserve_region(l4_addr_t addr, 
				l4_size_t size, 
                                l4_uint32_t flags, 
				l4_uint32_t * area)
{
  l4_offs_t offs;
  l4_addr_t a = addr;

  /* align address */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      Msg("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
          a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* reserve */
  return __reserve(&a,size,flags | MODIFY_DIRECT,area);
}
 
