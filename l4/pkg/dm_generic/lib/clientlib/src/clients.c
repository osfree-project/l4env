/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/clients.c
 * \brief  Generic dataspace manager client library, client handling
 *
 * \date   01/22/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_share_call(&(ds->manager),ds->id,&client,
			      rights,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: share failed, ds %u at %x.%x, client %x.%x "
	    "(ret %d, exc %d)!",
	    ds->id, ds->manager.id.task,ds->manager.id.lthread,
	    client.id.task,client.id.lthread,ret,_env.major);
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
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_revoke_call(&(ds->manager),ds->id,
			       &client,
			       rights,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: revoke failed, ds %u at %x.%x, client %x.%x "
	    "(ret %d, exc %d)!",
	    ds->id, ds->manager.id.task,ds->manager.id.lthread,
	    client.id.task,client.id.lthread,ret,_env.major);
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
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_check_rights_call(&(ds->manager),ds->id,rights,&_env);
  if (((ret < 0) && (ret != -L4_EPERM)) || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: check rights failed, ds %u at %x.%x "
	    "(ret %d, exc %d)!",ds->id, ds->manager.id.task,
	    ds->manager.id.lthread,ret,_env.major);
      if (ret) 
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}
