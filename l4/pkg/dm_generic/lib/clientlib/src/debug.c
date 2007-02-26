/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/debug.c
 * \brief  Generic dataspace manager client library, misc. debug functions
 *
 * \date   01/31/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/env.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>

/*****************************************************************************
 *** libdm_generic API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Set dataspace name
 * 
 * \param  ds            Dataspace id
 * \param  name          Dataspace name
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller is not the owner of the dataspace
 *         - \c -L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_set_name(l4dm_dataspace_t * ds, 
		 const char * name)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_set_name_call(&(ds->manager),ds->id,name,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: set dataspace name failed (ret %d, exc %d)",
	    ret,_env.major);
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
 * \brief  Get dataspace name
 * 
 * \param  ds            Dataspace id
 * \retval name          Dataspace name
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_get_name(l4dm_dataspace_t * ds, 
		 char * name)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_get_name_call(&(ds->manager),ds->id,&name,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: get dataspace name failed (ret %d, exc %d)",
	    ret,_env.major);
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
 * \brief  Show information about dataspace
 * 
 * \param  ds            Dataspace id
 */
/*****************************************************************************/ 
void
l4dm_ds_show(l4dm_dataspace_t * ds)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_show_ds_call(&(ds->manager),ds->id,&_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
    ERROR("libdm_generic: show dataspace failed (ret %d, exc %d)",
	  ret,_env.amjor);
}

/*****************************************************************************/
/**
 * \brief  Dump dataspaces
 * 
 * \param  dsm_id        Dataspace manager thread id
 * \param  owner         Dataspace owner, if set to L4_INVALID_ID dump all
 *                       dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  dump dataspaces owned by task
 * \retval ds            Dataspace id
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC       IPC error calling dataspace manager
 *         - \c -L4_ENOHANDLE  could not create dataspace descriptor
 *         - \c -L4_ENOMEM     out of memory
 */
/*****************************************************************************/ 
int
l4dm_ds_dump(l4_threadid_t dsm_id, 
	     l4_threadid_t owner, 
	     l4_uint32_t flags,
	     l4dm_dataspace_t * ds)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  if (l4_thread_equal(dsm_id,L4DM_DEFAULT_DSM))
    {
      /* request dataspace manager id from L4 environment */
      dsm_id = l4env_get_default_dsm();
      if (l4_is_invalid_id(dsm_id))
	{
	  ERROR("libdm_generic: no dataspace manager found!");
	  return -L4_ENODM;
	}
    }

  /* call dataspace manager */
  ret = if_l4dm_generic_dump_call(&dsm_id,&owner,flags,
                                  ds,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: dump dataspaces failed (ret %d, exc %d)",
	    ret,_env.major);
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
 * \brief  List dataspaces
 * 
 * \param  dsm_id        Dataspace manager thread id
 * \param  owner         Dataspace owner, if set to L4_INVALID_ID list
 *                       all dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  list dataspaces owned by task
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list(l4_threadid_t dsm_id, 
	     l4_threadid_t owner, 
	     l4_uint32_t flags)
{
  CORBA_Environment _env = dice_default_environment;
 
  if (l4_thread_equal(dsm_id,L4DM_DEFAULT_DSM))
    {
      /* request dataspace manager id from L4 environment */
      dsm_id = l4env_get_default_dsm();
      if (l4_is_invalid_id(dsm_id))
	{
	  ERROR("libdm_generic: no dataspace manager found!");
	  return -L4_ENODM;
	}
    }

  /* call dataspace manager */
  if_l4dm_generic_list_call(&dsm_id,&owner,flags,&_env);
  if (_env.major != CORBA_NO_EXCEPTION)
    {
      ERROR("libdm_generic: list dataspaces failed (exc %d)",_env.major);
      return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  List all dataspaces
 * 
 * \param  dsm_id        Dataspace manager id
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list_all(l4_threadid_t dsm_id)
{
  /* list all dataspaces at dataspace manager */
  return l4dm_ds_list(dsm_id,L4_INVALID_ID,0);
}
