/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/reserve.c
 * \brief  VM area reservation.
 *
 * \date   08/22/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Mark address area reserved.
 * 
 * \param  addr          region start address, must be page aligned
 *                       (L4RM_ADDR_FIND .. find suitable area)
 * \param  size          region size
 * \param  flags         flags:
 *                       - #L4RM_LOG2_ALIGNED
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED
 *                         reserve a superpage aligned region
 *                       - #L4RM_LOG2_ALLOC
 *                         reserve the whole 2^(log2(size) + 1) sized region
 * \retval addr          start address used
 * \retval area          area id 
 *	
 * \return 0 on success (reserved area), error code otherwise:
 *         - -#L4_ENOTFOUND  no suitable address area found
 *         - -#L4_ENOMEM     out of memory allocation region descriptor
 *         - -#L4_EUSED      address region already used 
 *         - -#L4_EINVAL     invalid area
 *
 * Mark address area reserved. If addr == L4RM_ADDR_FIND, find a suitable area.
 */
/*****************************************************************************/ 
int
l4rm_do_reserve(l4_addr_t * addr, l4_size_t size, l4_uint32_t flags, 
                l4_uint32_t * area)
{
  int ret;
  l4rm_region_desc_t * r;

  LOGdL(DEBUG_REGION_RESERVE, "addr 0x%08x, size 0x%08x", *addr, size);

  /* allocate and setup new region descriptor */
  r = l4rm_region_desc_alloc();
  if (r == NULL)
    return -L4_ENOMEM;
  
  SET_REGION_FREE(r);
  
  /* lock region list */
  l4rm_lock_region_list_direct(flags);

  /* create new region */
  ret = l4rm_new_region(r, *addr, size, L4RM_DEFAULT_REGION_AREA, 
                        flags | L4RM_SET_AREA);

  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);

  if (ret < 0)
    {
      /* insert failed, region probably already used  */
      l4rm_region_desc_free(r);
      return ret;
    }

  *addr = r->start;
  *area = REGION_AREA(r);
  
  LOGdL(DEBUG_REGION_RESERVE, "got area 0x%08x-0x%08x, area id 0x%05x",
        r->start, r->end, *area);

  /* done */
  return 0;
}

