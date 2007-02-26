/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/setup.c
 * \brief  Setup vm area 
 *
 * \date   08/12/2004
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/** the pager of the region mapper, set in libl4rm.c */
extern l4_threadid_t l4rm_task_pager_id;

/*****************************************************************************/
/**
 * \brief  Setup VM area
 * 
 * \param  addr          Region start address 
 *                       (#L4RM_ADDR_FIND ... find suitable region)
 * \param  size          Region size
 * \param  area          Area id
 * \param  type          Region type:
 *                       - #L4RM_REGION_PAGER      region with external pager,
 *                                                 \a pager must contain the id
 *                                                 of the external pager
 *                       - #L4RM_REGION_EXCEPTION  region with exception forward
 *                       - #L4RM_REGION_BLOCKED    blocked (unavailable) region
 * \param  flags         Flags:
 *                       - #L4RM_LOG2_ALIGNED 
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED
 *                         reserve a superpage aligned region
 *                       - #L4RM_LOG2_ALLOC   
 *                         reserve the whole 2^(log2(size) + 1) sized region
 * \param  pager         External pager (if type is #L4RM_REGION_PAGER), if set
 *                       to L4_INVALID_ID pager of the region mapper thread 
 *                       is used
 *	
 * \return 0 on success (setup region), error code otherwise:
 *         - -#L4_ENOTFOUND  no suitable address area found
 *         - -#L4_ENOMEM     out of memory allocation region descriptor
 *         - -#L4_EUSED      address region already used 
 *         - -#L4_EINVAL     invalid area / type
 */
/*****************************************************************************/ 
int
l4rm_do_area_setup(l4_addr_t * addr, l4_size_t size, l4_uint32_t area,
                   int type, l4_uint32_t flags, l4_threadid_t pager)
{
  int ret;
  l4rm_region_desc_t * r;

  LOGdL(DEBUG_REGION_SETUP, "addr 0x"l4_addr_fmt", size 0x%lx, type %d, " \
        "flags 0x%08x", *addr, (l4_addr_t)size, type, flags);

  if ((type != L4RM_REGION_PAGER) && (type != L4RM_REGION_EXCEPTION) && 
      (type != L4RM_REGION_BLOCKED))
    return -L4_EINVAL;

  /* allocate and setup new region descriptor */
  r = l4rm_region_desc_alloc();
  if (r == NULL)
    return -L4_ENOMEM;

  switch (type)
    {
    case L4RM_REGION_PAGER:
      LOGd(DEBUG_REGION_SETUP, "forward to "l4util_idfmt, l4util_idstr(pager));

      SET_REGION_PAGER(r);
      r->data.pager.pager =
        (l4_is_invalid_id(pager)) ? l4rm_task_pager_id : pager;
      break;

    case L4RM_REGION_EXCEPTION:
      LOGd(DEBUG_REGION_SETUP, "forward exception");

      SET_REGION_EXCEPTION(r);
      break;

    case L4RM_REGION_BLOCKED:
      LOGd(DEBUG_REGION_SETUP, "blocked");

      SET_REGION_BLOCKED(r);
      break;
    }

  /* lock region list */
  l4rm_lock_region_list_direct(flags);

  /* create new region */
  ret = l4rm_new_region(r, *addr, size, area, flags | L4RM_TREE_INSERT);

  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);

  if (ret < 0)
    {
      /* insert failed, region probably already used  */
      l4rm_region_desc_free(r);
      return ret;
    }

  LOGd(DEBUG_REGION_SETUP, "using region at 0x"l4_addr_fmt"-0x"l4_addr_fmt,
       r->start, r->end);
  *addr = r->start;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Clear VM area
 * 
 * \param  addr          Address of VM area
 *	
 * \return 0 on success, error code otherwise
 *         - -#L4_ENOTFOUND  no region found at address \a addr
 *         - -#L4_EINVAL     invalid region type
 *         - -#L4_EIPC       IPC error calling region mapper thread
 *
 * Clear region which was set up with l4rm_area_setup_region().
 */
/*****************************************************************************/ 
int
l4rm_area_clear_region(l4_addr_t addr)
{
  l4rm_region_desc_t * rp;
  int ret;

  /* lock region list */
  l4rm_lock_region_list();

  /* lookup address */
  rp = l4rm_find_used_region(addr);
  if (rp == NULL)
    {
      /* not found */
      l4rm_unlock_region_list();
      return -L4_ENOTFOUND;
    }

  /* check region type */
  if (REGION_TYPE(rp) == REGION_DATASPACE)
    {
      /* dataspace regions must be released with l4rm_detach() */
      l4rm_unlock_region_list();
      return -L4_EINVAL;
    }

  /* remove region from region tree */
  ret = l4rm_tree_remove_region(addr, REGION_TYPE(rp), 0, &rp);
  if (ret < 0)
    {
      /* region not found */
      l4rm_unlock_region_list();
      return ret;
    }

  /* unmap region */
  l4rm_unmap_region(rp);

  /* remove region from region list */
  l4rm_free_region(rp);

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}

