/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/size.c
 * \brief  DMphys, return dataspace size
 * 
 * \date   11/22/2001
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
 * \brief  Return dataspace size
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval size          Dataspace size
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (\a size contains the dataspace size), 
 *         error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   Caller is not a client of the dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_size(sm_request_t * request, 
			    l4_uint32_t ds_id, 
			    l4_uint32_t * size, 
			    sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_threadid_t caller = request->client_tid;

  /* get dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id,caller,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id, id %u, caller %x.%x",
	      ds_id,caller.id.task,caller.id.lthread);
      else
	ERROR("DMphys: caller %x.%x is not a client of dataspace %d!",
	      caller.id.task,caller.id.lthread,ds_id);
#endif
      return ret;
    }
  
  /* set dataspace size */
  *size = dmphys_ds_get_size(ds);

  /* done */
  return 0;
}
