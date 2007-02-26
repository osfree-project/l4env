/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/transfer.c
 * \brief  Generic dataspace manager client library, 
 *         transfer dataspace ownership
 *
 * \date   01/23/2002
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

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>

/*****************************************************************************
 *** libdm_generic API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Transfer dataspace ownership
 * 
 * \param  ds            Dataspace descriptor
 * \param  new_owner     New dataspace owner
 *	
 * \return 0 on success (set owner to \a new_owner), error code otherwise:
 *         - \c -L4_EIPC   IPC error calling dataspace manager
 *         - \c -L4_EINVAL Invalid dataspace descriptor
 *         - \c -L4_EPERM  Permission denied, only the current owner can 
 *                         transfer the ownership
 */
/*****************************************************************************/ 
int
l4dm_transfer(l4dm_dataspace_t * ds, 
	      l4_threadid_t new_owner)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_transfer(ds->manager,ds->id,
				 (if_l4dm_threadid_t *)&new_owner,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: transfer ownership failed, ds %u at %x.%x, "
	    "new owner %x.%x (ret %d, exc %d)!",ds->id, ds->manager.id.task,
	    ds->manager.id.lthread,new_owner.id.task,new_owner.id.lthread,
	    ret,_exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}
