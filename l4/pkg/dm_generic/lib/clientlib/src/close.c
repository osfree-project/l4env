/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/close.c
 * \brief  Generic dataspace manager client library, close dataspace
 *
 * \date   11/23/2001
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
 * \brief Close dataspace.
 * 
 * \param  ds            Dataspace id
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   operation not permitted, only the owner can 
 *                          close a dataspace
 */
/*****************************************************************************/ 
int
l4dm_close(l4dm_dataspace_t * ds)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_close(ds->manager,ds->id,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: close dataspace %u at %x.%x failed "
	    "(ret %d, exc %d)!",ds->id,
	    ds->manager.id.task,ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Close all dataspaces of a client
 * 
 * \param  dsm_id        Dataspace manager thread id
 * \param  client        Client thread id
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  close all dataspaces owned by
 *                                          threads of the task specified by
 *                                          \a client.
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid client thread id
 *         - \c -L4_EPERM   permission denied
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_close_all(l4_threadid_t dsm_id, 
	       l4_threadid_t client, 
	       l4_uint32_t flags)
{
  int ret;
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_generic_close_all(dsm_id,(if_l4dm_threadid_t *)&client,flags,
				  &_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: close dataspaces failed (ret %d, exc %d)!",
	    ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}
