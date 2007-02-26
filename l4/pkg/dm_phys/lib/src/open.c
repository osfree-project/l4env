/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/open.c
 * \brief  DMphys client library, create new dataspace
 *
 * \date   01/09/2002
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

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include <l4/dm_phys/consts.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_phys/dm_phys-client.h>
#include "__debug.h"

/*****************************************************************************
 *** DMphys client lib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create new dataspace (extended version for DMphys)
 * 
 * \param  pool          Memory pool
 * \param  addr          Start address of memory area
 *                         set to \c L4DM_MEMPHYS_ANY_ADDR to find a suitable 
 *                         memory area
 * \param  size          Dataspace size
 * \param  align         Memory area alignment 
 * \param  flags         Flags:
 *                       - \c L4DM_CONTIGUOUS  allocate contiguous memory area
 * \param  name          Dataspace name
 * \retval ds            Dataspace id
 *	
 * \return 0 on success (created dataspace, ds contains a valid dataspace id),
 *         error code otherwise:
 *         - \c -L4_ENODM   DMphys not found
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_ENOMEM  out of memory
 *
 * Call DMphys to create a new dataspace.
 */
/*****************************************************************************/ 
int
l4dm_memphys_open(int pool, 
		  l4_addr_t addr, 
		  l4_size_t size, 
		  l4_addr_t align, 
		  l4_uint32_t flags, 
		  const char * name, 
		  l4dm_dataspace_t * ds)
{
  l4_threadid_t dsm_id;
  int ret;
  sm_exc_t _exc;

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return -L4_ENODM;
  
#if DEBUG_OPEN
  INFO("DMphys at %x.%x\n",dsm_id.id.task,dsm_id.id.lthread);
#endif

  /* call DMphys */
  if (name != NULL)
    ret = if_l4dm_memphys_dmphys_open(dsm_id,pool,addr,size,align,flags,name,
				    (if_l4dm_dataspace_t *)ds,&_exc);
  else
    ret = if_l4dm_memphys_dmphys_open(dsm_id,pool,addr,size,align,flags,"",
				    (if_l4dm_dataspace_t *)ds,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_phys: open dataspace at DMphys (%x.%x) failed "
	    "(ret %d, exc %d)",dsm_id.id.task,dsm_id.id.lthread,
	    ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return ret;
}

