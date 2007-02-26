/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/detach.c
 * \brief  Detach dataspace.
 *
 * \date   08/09/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/util/macros.h>
#include <l4/dm_generic/dm_generic.h>

/* L4RM includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__debug.h"

/*****************************************************************************
 *** Client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Detach dataspace. Also unmap the complete memory of that region.
 * 
 * \param  addr          VM region address
 *	
 * \return 0 on success (dataspace detached from region \a id), error code
 *         otherwise:
 *         - -#L4_EINVAL  invalid region id
 *         - -#L4_EIPC    error calling region mapper
 *
 * Detach dataspace from region \a id.
 */
/*****************************************************************************/ 
int
l4rm_detach(const void * map_addr)
{
  l4_addr_t addr = (l4_addr_t)map_addr;
  int ret;
  l4rm_region_desc_t * r;
  l4dm_dataspace_t ds;

  LOGdL(DEBUG_DETACH, "detach region at 0x"l4_addr_fmt, addr);

  /* lock region list */
  l4rm_lock_region_list();

  /* remove region from region tree */
  ret = l4rm_tree_remove_region(addr, REGION_DATASPACE, 0, &r);
  if (ret < 0)
    {
      /* region not found / invalid region type */
      l4rm_unlock_region_list();
      return ret;
    }
  ds = r->data.ds.ds;

  LOGdL(DEBUG_DETACH, "DS %u at "l4util_idfmt", offset 0x%lx",
        ds.id, l4util_idstr(ds.manager), r->data.ds.offs);
  LOGdL(DEBUG_DETACH, "region <"l4_addr_fmt","l4_addr_fmt">, area 0x%05x",
        r->start, r->end, REGION_AREA(r));

  /* unmap region */
  l4rm_unmap_region(r);

  /* remove region from region list */
  l4rm_free_region(r);

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}
