/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/clients.c
 * \brief  Generic dataspace manager client library, client handling
 *
 * \date   01/22/2002
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
 * \brief Grant dataspace access rights to a client
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Access rights:
 *                       - \c L4DM_RO  read-only
 *                       - \c L4DM_RW  read/write
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   the requested rights for the new client exceed 
 *                          the rights of the caller for the dataspace
 * 
 * Grant / extend dataspace access rights to a client. If the client already 
 * has access to the dataspace, the new rights are added to the existing 
 * rights.
 */
/*****************************************************************************/ 
int
l4dm_share(l4dm_dataspace_t * ds, 
	   l4_threadid_t client, 
	   l4_uint32_t rights)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_share(ds->manager,ds->id,(if_l4dm_threadid_t *)&client,
			      rights,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: share failed, ds %u at %x.%x, client %x.%x "
	    "(ret %d, exc %d)!",
	    ds->id, ds->manager.id.task,ds->manager.id.lthread,
	    client.id.task,client.id.lthread,ret,_exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Revoke dataspace access rights.
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Access rights:
 *                       - \c L4DM_WRITE       revoke write access
 *                       - \c L4DM_ALL_RIGHTS  revoke all rights, the client
 *                                             is removed from the client list
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller has not the right to revoke access rights
 */
/*****************************************************************************/ 
int
l4dm_revoke(l4dm_dataspace_t * ds, 
	    l4_threadid_t client, 
	    l4_uint32_t rights)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_revoke(ds->manager,ds->id,
			       (if_l4dm_threadid_t *)&client,
			       rights,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: revoke failed, ds %u at %x.%x, client %x.%x "
	    "(ret %d, exc %d)!",
	    ds->id, ds->manager.id.task,ds->manager.id.lthread,
	    client.id.task,client.id.lthread,ret,_exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Check dataspace access rights
 * 
 * \param  ds            Dataspace descriptor
 * \param  rights        Access rights
 *	
 * \return 0 if caller has the access rights, error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   requested operations not allowed
 */
/*****************************************************************************/ 
int
l4dm_check_rights(l4dm_dataspace_t * ds, 
		  l4_uint32_t rights)
{
  int ret;
  sm_exc_t _exc;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_check_rights(ds->manager,ds->id,rights,&_exc);
  if (((ret < 0) && (ret != -L4_EPERM)) || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: check rights failed, ds %u at %x.%x "
	    "(ret %d, exc %d)!",ds->id, ds->manager.id.task,
	    ds->manager.id.lthread,ret,_exc._type);
      if (ret) 
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}
