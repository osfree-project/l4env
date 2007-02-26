/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/attach.c
 * \brief  Attach new dataspace.
 *
 * \date   06/03/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/sys/consts.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************/
/**
 * \brief Attach dataspace to region.
 * 
 * \param  ds            dataspace id
 * \param  area          area id
 * \param  addr          region start address 
 *                       (#L4RM_ADDR_FIND ... find suitable region)
 * \param  size          region size
 * \param  ds_offs       offset in dataspace
 * \param  flags         flags:
 *                       - #L4DM_RO            attach read-only
 *                       - #L4DM_RW            attach read/write
 *                       - #L4RM_LOG2_ALIGNED  find a 2^(log2(size) + 1)
 *                                             aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED find a 
 *                                             superpage aligned region
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             2^(log2(size) + 1) sized area
 *                       - #L4RM_MAP           immediately map area
 *                       - #L4RM_MODIFY_DIRECT add new region without locking
 *                                             the region list and calling the
 *                                             region mapper thread
 * \retval addr          start address of region
 *
 * \return 0 on success (dataspace attached to region), error code otherwise:
 *         - -#L4_ENOMEM  out of memory allocating region descriptor
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_ENOMAP  no suitable region found
 *         - -#L4_EUSED   region already used
 *         - -#L4_EIPC    error calling region mapper
 *         - -#L4_EPERM   could not set the access rights for the region
 *                        mapper thread at the dataspace manager. 
 */
/*****************************************************************************/ 
int
l4rm_do_attach(const l4dm_dataspace_t * ds, l4_uint32_t area, l4_addr_t * addr,
               l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags)
{
  l4rm_region_desc_t * r;
  l4_uint32_t rights = (flags & L4DM_RIGHTS_MASK);
  l4_offs_t start_offs, map_offs;
  int ret;
  
  /* sanity checks */
  if (l4dm_is_invalid_ds(*ds))
    return -L4_EINVAL;

  LOGdL(DEBUG_ATTACH, 
        "DS %u at "l4util_idfmt", offset 0x%lx, addr 0x"l4_addr_fmt
	", size %lu, area 0x%x", ds->id, l4util_idstr(ds->manager), ds_offs,
	*addr, (l4_addr_t)size, area);
  
  /* Check alignments of ds_offs. If ds_offs is not page aligned, recalculate
   * ds_offs and size to fit page boundaries and attach that larger area of 
   * the dataspace. l4rm_do_attach then returns the address which points to 
   * the original offset.
   * addr and size are aligned in l4rm_new_region.
   */

  /* align offset in dataspace */
  start_offs = l4_trunc_page(ds_offs);
  map_offs = ds_offs - start_offs;
  size += map_offs;

  LOGd(DEBUG_ATTACH, "aligned to offs 0x%lx, map_offs 0x%lx, size %lu",
       start_offs, map_offs, (l4_addr_t)size);

  /* allocate and setup new region descriptor */
  r = l4rm_region_desc_alloc();
  if (r == NULL)
    return -L4_ENOMEM;

  r->data.ds.ds = *ds;
  r->data.ds.offs = start_offs;
  r->data.ds.rights = rights;
  SET_REGION_DATASPACE(r);

  /* lock region list */
  l4rm_lock_region_list_direct(flags);

  /* create new region */
  ret = l4rm_new_region(r, *addr, size, area, flags | L4RM_TREE_INSERT);
  if (ret < 0)
    {
      /* insert failed, region probably already used  */
      l4rm_region_desc_free(r);
      l4rm_unlock_region_list_direct(flags);
      return ret;
    }

  LOGd(DEBUG_ATTACH, "attached to region 0x"l4_addr_fmt"-0x"l4_addr_fmt,
       r->start, r->end);

  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);

  if ((flags & L4RM_MAP) && !(flags & L4RM_MODIFY_DIRECT))
    {
      /* immediately map attached region */
      LOGd(DEBUG_ATTACH,
           "map attached region, region 0x"l4_addr_fmt"-0x"l4_addr_fmt
           ", size 0x%lx, rights 0x%02x", r->start, r->end, (l4_addr_t)size,
           rights);

      /* we don't need to use l4dm_map() here since we already know
       * which dataspace is attached -- saves one l4rm_lookup */
      ret = l4dm_map_ds(ds, start_offs, r->start, size, rights);
      if (ret < 0)
	LOG_printf("map attached region failed (%d), ignored!\n",ret);
    }

  /* return address which points to ds_offs */
  *addr = r->start + map_offs;

  /* done */
  return 0;
}

  
