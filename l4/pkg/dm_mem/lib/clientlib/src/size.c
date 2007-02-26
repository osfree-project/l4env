/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/size.c
 * \brief  Memory dataspace manager client library, return dataspace size
 *
 * \date   01/29/2002
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

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_mem/dm_mem-client.h>

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return dataspace size
 * 
 * \param  ds            Dataspace descriptor
 * \retval size          Dataspace size
 *	
 * \return 0 on success (\a size contains the dataspace size), 
 *         error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   Caller is not a client of the dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_size(l4dm_dataspace_t * ds, 
	      l4_size_t * size)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_mem_size(ds->manager,ds->id,size,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_mem: get size of dataspace %u at %x.%x failed "
	    "(ret %d, exc %d)!",ds->id,ds->manager.id.task,
	    ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;
}
