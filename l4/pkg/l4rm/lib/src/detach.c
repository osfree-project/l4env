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
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/dm_generic/dm_generic.h>

/* L4RM includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
//#include "__libl4rm.h"
#include "__debug.h"

// was in __libl4rm.h
extern l4_threadid_t l4rm_service_id;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Detach region.
 * 
 * \param  addr          VM area address
 *	
 * \return 0 on success (region detached), error code otherwise:
 *         - \c -L4_EINVAL  invalid address (no attached dataspace found)
 *
 * Detach region, remove it from region list and region tree. 
 */
/*****************************************************************************/ 
static int
__detach(l4_addr_t addr)
{
  int ret;
  l4rm_region_desc_t * r;
  l4dm_dataspace_t ds;
  l4_uint32_t rights,region_rights;
  l4_size_t size;
  int addr_align,log2_size,fpage_size;

  LOGdL(DEBUG_DETACH,"detach region at 0x%08x",addr);

  /* lock region list */
  l4rm_lock_region_list();

  /* remove region from region tree */
  ret = l4rm_tree_remove_region(addr,0,&r);
  if (ret < 0)
    {
      /* region not found */
      l4rm_unlock_region_list();
      return ret;
    }

  ds = r->ds;
  region_rights = r->rights;
  addr = r->start;
  size = r->end - r->start + 1;

  LOGdL(DEBUG_DETACH,"DS %u at %x.%x, offset 0x%x",r->ds.id,
        r->ds.manager.id.task,r->ds.manager.id.lthread,r->offs);
  LOGdL(DEBUG_DETACH,"region <%08x,%08x>, area 0x%05x",
        r->start,r->end,REGION_AREA(r));

  /* remove regeion from region list */
  l4rm_free_region(r);

  /* unmap pages
   * we try to use as few l4_fpage_unmap calls as possible to minimize the 
   * detach overhead for large regions.
   */
  LOGdL(DEBUG_DETACH_UNMAP,"unmap addr 0x%08x, size %u",addr,size);

  while (size > 0)
    {
      LOGdL(DEBUG_DETACH_UNMAP,"0x%08x, size %u (0x%08x)",addr,size,size);

      /* calculate the largest fpage we can unmap at address addr, 
       * it depends on the alignment of addr and the size */
      addr_align = (addr == 0) ? 32 : l4util_bsf(addr);
      log2_size = l4util_log2(size);
      fpage_size = (addr_align < log2_size) ? addr_align : log2_size;

      LOGdL(DEBUG_DETACH_UNMAP,"addr %d, size %d, log2 %d",
            addr_align,log2_size,fpage_size);

      /* unmap page */
      l4_fpage_unmap(l4_fpage(addr,fpage_size,0,0),
		     L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

      addr += (1UL << fpage_size);
      size -= (1UL << fpage_size);
    }

  /* adapt access rights of the region mapper thread if necessary */
  rights = l4rm_get_access_rights(&ds);
  if (region_rights & ~rights)
    {
      LOGdL(DEBUG_DETACH,"shrink rights\n" \
            "  old 0x%04x, new 0x%04x, revoke 0x%04x\n",
            region_rights | rights,rights,region_rights & ~rights);

      /* revoke access rights */
      l4dm_revoke(&ds,l4rm_service_id,region_rights & ~rights);
    }

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}

/*****************************************************************************
 *** Client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Detach dataspace.
 * 
 * \param  addr          VM region address
 *	
 * \return 0 on success (dataspace detached from region \a id), error code
 *         otherwise:
 *         - \c -L4_EINVAL  invalid region id
 *         - \c -L4_EIPC    error calling region mapper
 *
 * Detach dataspace from region \a id.
 */
/*****************************************************************************/ 
int
l4rm_detach(void * addr)
{
  /* detach */
  return __detach((l4_addr_t)addr);
}

