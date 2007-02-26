/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/lock.c
 * \brief  DMphys, lock/unlock dataspace region
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
#include <l4/sys/types.h>
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
 * \brief  Lock dataspace region
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  invalid dataspace region
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_lock(sm_request_t * request, 
			    l4_uint32_t ds_id, 
			    l4_uint32_t offset, 
			    l4_uint32_t size, 
			    sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_threadid_t caller = request->client_tid;
  l4_size_t ds_size;

  /* we do not need to do anything, just check dataspace and region */
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

  ds_size = dmphys_ds_get_size(ds);
  if (offset + size > ds_size)
    return -L4_EINVAL_OFFS;

#if DEBUG_LOCK
  INFO("ds %u, caller %x.%x\n",ds_id,caller.id.task,caller.id.lthread);
  DMSG("  offset 0x%08x, size 0x%08x, ds size 0x%08x\n",offset,size,ds_size);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Unlock dataspace region
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  invalid dataspace region
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_unlock(sm_request_t * request, 
			      l4_uint32_t ds_id, 
			      l4_uint32_t offset, 
			      l4_uint32_t size, 
			      sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_threadid_t caller = request->client_tid;
  l4_size_t ds_size;

  /* we do not need to do anything, just check dataspace and region */
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

  ds_size = dmphys_ds_get_size(ds);
  if (offset + size > ds_size)
    return -L4_EINVAL_OFFS;

#if DEBUG_LOCK
  INFO("ds %u, caller %x.%x\n",ds_id,caller.id.task,caller.id.lthread);
  DMSG("  offset 0x%08x, size 0x%08x, ds size 0x%08x\n",offset,size,ds_size);
#endif

  /* done */
  return 0;
}
