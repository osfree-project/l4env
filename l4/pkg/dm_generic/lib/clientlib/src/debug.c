/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/debug.c
 * \brief  Generic dataspace manager client library, misc. debug functions
 *
 * \date   01/31/2002
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
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_generic_set_name(ds->manager,ds->id,name,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: set dataspace name failed (ret %d, exc %d)",
	    ret,_exc._type);
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
  char * n;
  int ret;
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_generic_get_name(ds->manager,ds->id,&n,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: get dataspace name failed (ret %d, exc %d)",
	    ret,_exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }
 
  /* copy name */
  strcpy(name,n);

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
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_generic_show_ds(ds->manager,ds->id,&_exc);
  if ((ret < 0) || (_exc._type != exc_l4_no_exception))
    ERROR("libdm_generic: show dataspace failed (ret %d, exc %d)",
	  ret,_exc._type);
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
  sm_exc_t _exc;

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
  ret = if_l4dm_generic_dump(dsm_id,(if_l4dm_threadid_t *)&owner,flags,
			     (if_l4dm_dataspace_t *)ds,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: dump dataspaces failed (ret %d, exc %d)",
	    ret,_exc._type);
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
  sm_exc_t _exc;
 
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
  if_l4dm_generic_list(dsm_id,(if_l4dm_threadid_t *)&owner,flags,&_exc);
  if (_exc._type != exc_l4_no_exception)
    {
      ERROR("libdm_generic: list dataspaces failed (exc %d)",_exc._type);
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
