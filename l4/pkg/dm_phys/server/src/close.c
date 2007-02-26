/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/close.c
 * \brief  DMphys, close dataspace
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
#include "__pages.h"
#include "__internal_alloc.h"
#include "__dm_phys.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Close dataspace
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Iterator functio data, ignored
 */
/*****************************************************************************/ 
static void
__close(dmphys_dataspace_t * ds, 
	void * data)
{
  page_area_t * pages;
  page_pool_t * pool;

  /* get page area list / page pool */
  pages = dmphys_ds_get_pages(ds);
  pool = dmphys_ds_get_pool(ds);

  /* unmap pages */
  dmphys_unmap_areas(pages);
  
  /* release pages */
  dmphys_pages_release(pool,pages);

  /* release dataspace descriptor */
  dmphys_ds_release(ds);

  /* update internal memory allocation */
  dmphys_internal_alloc_update();
}

/*****************************************************************************
 *** DMphys internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Close dataspace
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return 0 on success.
 */
/*****************************************************************************/ 
int
dmphys_close(dmphys_dataspace_t * ds)
{
  /* close */
  __close(ds,NULL);

  return 0;
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Close dataspace
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (closed dataspace), error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   operation not permitted, only the owner can 
 *                          close a dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_close(sm_request_t * request, 
			     l4_uint32_t ds_id, 
			     sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;

#if DEBUG_CLOSE
  INFO("close ds %u\n",ds_id);
#endif

  /* get dataspace descriptor, check if caller owns the dataspace */
  ret = dmphys_ds_get_check_owner(ds_id,request->client_tid,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id %u, caller %x.%x",ds_id,
	      request->client_tid.id.task,request->client_tid.id.lthread);
      else
	ERROR("DMphys: client %x.%x does not own dataspace %u",
	      request->client_tid.id.task,request->client_tid.id.lthread,
	      ds_id);
#endif      
      return ret;
    }

  /* close */
  __close(ds,NULL);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Close all dataspaces of a client.
 * 
 * \param  request       Flick request structure
 * \param  client        Client thread id
 * \param  flags         Flags
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid client id
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_close_all(sm_request_t * request, 
				 const if_l4dm_threadid_t * client, 
				 l4_uint32_t flags, 
				 sm_exc_t * _ev)
{
  if (l4_is_invalid_id(*(l4_threadid_t *)client))
    return -L4_EINVAL;

  /* We should check whether the caller is allowd to close all dataspaces.
   * close_all can be used e.g. by a loader/task-server to release all 
   * dataspaces created by a client after the client has exited. 
   * Therefore we should allow close_all for non-owners of dataspaces, but
   * only for those we trust. */
  Msg("DMphys: close all dataspaces owned by %x.%x, caller %x.%x\n",
      ((l4_threadid_t *)client)->id.task,((l4_threadid_t *)client)->id.lthread,
      request->client_tid.id.task,request->client_tid.id.lthread);

  /* close dataspaces */
  dmphys_ds_iterate(__close,NULL,*(l4_threadid_t *)client,flags);

  /* done */
  return 0;
}

