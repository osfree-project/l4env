/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/transfer.c
 * \brief  DMphys, transfer dataspace ownership
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
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include "__dataspace.h"
#include "__debug.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Transfer the ownership of a dataspace
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  new_owner     New owner of the dataspace
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (set the owner to \a new_owner), error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id  
 *         - \c -L4_EPERM   permission denied, only the current owner can 
 *                          transfer the ownership.
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_transfer(sm_request_t * request, 
				l4_uint32_t ds_id, 
				const if_l4dm_threadid_t * new_owner, 
				sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_threadid_t caller = request->client_tid;
  l4_threadid_t * owner = (l4_threadid_t *)new_owner;

#if DEBUG_TRANSFER
  INFO("ds %u\n",ds_id);
  DMSG("  caller %x.%x, new owner %x.%x\n",
       caller.id.task,caller.id.lthread,owner->id.task,owner->id.lthread);
#endif

  /* get dataspace descriptor, check if caller owns the dataspace */
  ret = dmphys_ds_get_check_owner(ds_id,caller,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id,id %u, caller %x.%x",
	      ds_id,caller.id.task,caller.id.lthread); 
      else
	ERROR("DMphys: caller is not the current owner "
	      "(ds %u, caller %x.%x)!",ds_id,caller.id.task,
	      caller.id.lthread);
#endif	
      return ret;
    }

  /* set new owner */
  dmphys_ds_set_owner(ds,*owner);

  /* done */
  return 0;
}

