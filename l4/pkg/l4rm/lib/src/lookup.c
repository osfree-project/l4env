/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/lookup.c
 * \brief  Address lookup.
 *
 * \date   02/21/2001
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

#if DEBUG_LOOKUP
  INFO("lookup addr 0x%08x\n",a);
#endif

  /* lookup address */
  ret = l4rm_tree_lookup_region(a,&r);
  if (ret < 0)
    {
      /* not found */
      l4rm_unlock_region_list();
      return ret;
    }

#if DEBUG_LOOKUP
  INFO("found region\n");
  DMSG("  ds %u at %x.%x, attached to 0x%08x-0x%08x, offs 0x%08x\n",
       r->ds.id,r->ds.manager.id.task,r->ds.manager.id.lthread,
       r->start,r->end,r->offs);
#endif

  memcpy(ds,&r->ds,sizeof(l4dm_dataspace_t));
  *offset = (a - r->start) + r->offs;
  *map_addr = r->start;
  *map_size = r->end - r->start + 1;	

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}

  
