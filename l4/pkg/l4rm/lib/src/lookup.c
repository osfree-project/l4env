/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/lookup.c
 * \brief  Address lookup.
 *
 * \date   02/21/2001
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

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__debug.h"

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Lookup address.
 * 
 * \param  addr          Address
 * \retval ds            Dataspace
 * \retval offset        Offset of ptr in dataspace
 * \retval map_start     Map area start address
 * \retval map_size      Map area size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_ENOTFOUND  dataspace not found for address
 *         - \c -L4_EIPC       error calling region mapper
 *         
 * Return the dataspace and region which is attached to \a addr.
 */
/*****************************************************************************/ 
int
l4rm_lookup(void * addr, 
	    l4dm_dataspace_t * ds, 
	    l4_offs_t * offset, 
	    l4_addr_t * map_addr, 
	    l4_size_t * map_size)
{
  int ret;
  l4rm_region_desc_t * r;
  l4_addr_t a = (l4_addr_t)addr;

  /* lock region list */
  l4rm_lock_region_list();

  LOGdL(DEBUG_LOOKUP,"lookup addr 0x%08x",a);

  /* lookup address */
  ret = l4rm_tree_lookup_region(a,&r);
  if (ret < 0)
    {
      /* not found */
      l4rm_unlock_region_list();
      return ret;
    }
  
  LOGdL(DEBUG_LOOKUP,"found region\n" \
        "  ds %u at %x.%x, attached to 0x%08x-0x%08x, offs 0x%08x",
        r->ds.id,r->ds.manager.id.task,r->ds.manager.id.lthread,
        r->start,r->end,r->offs);

  memcpy(ds,&r->ds,sizeof(l4dm_dataspace_t));
  *offset = (a - r->start) + r->offs;
  *map_addr = r->start;
  *map_size = r->end - r->start + 1;	

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}

  
