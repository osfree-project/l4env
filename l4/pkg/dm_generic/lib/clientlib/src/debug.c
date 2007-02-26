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
#include "__debug.h"

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
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   caller is not the owner of the dataspace
 *         - -#L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_set_name(const l4dm_dataspace_t * ds, const char * name)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_set_name_call(&(ds->manager), ds->id, name, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: set name for ds %u at "l4util_idfmt \
            "failed (ret %d, exc %d)", ds->id, l4util_idstr(ds->manager),
            ret, _env.major);
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
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_get_name(const l4dm_dataspace_t * ds, char * name)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_get_name_call(&(ds->manager), ds->id, &name, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: get name for da %u at "l4util_idfmt \
            "failed (ret %d, exc %d)", ds->id, l4util_idstr(ds->manager),
	    ret, _env.major);
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
l4dm_ds_show(const l4dm_dataspace_t * ds)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_show_ds_call(&(ds->manager), ds->id, &_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
    LOGdL(DEBUG_ERRORS, "libdm_generic: show ds %u at "l4util_idfmt \
          " failed (ret %d, exc %d)", ds->id, l4util_idstr(ds->manager), 
	  ret, _env.major);
}

/*****************************************************************************/
/**
 * \brief  List dataspaces
 * 
 * \param  dsm_id        Dataspace manager thread id
 * \param  owner         Dataspace owner, if set to #L4_INVALID_ID list
 *                       all dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  list dataspaces owned by task
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list(l4_threadid_t dsm_id, l4_threadid_t owner, l4_uint32_t flags)
{
  CORBA_Environment _env = dice_default_environment;
 
  if (l4_thread_equal(dsm_id, L4DM_DEFAULT_DSM))
    {
      /* request dataspace manager id from L4 environment */
      dsm_id = l4env_get_default_dsm();
      if (l4_is_invalid_id(dsm_id))
	{
	  LOGdL(DEBUG_ERRORS, "libdm_generic: no dataspace manager found!");
	  return -L4_ENODM;
	}
    }

  /* call dataspace manager */
  if_l4dm_generic_list_call(&dsm_id, &owner, flags, &_env);
  if (_env.major != CORBA_NO_EXCEPTION)
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: list dataspaces of "l4util_idfmt \
            "at "l4util_idfmt" failed (exc %d)",
            l4util_idstr(owner), l4util_idstr(dsm_id), _env.major);
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
 *         - -#L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list_all(l4_threadid_t dsm_id)
{
  /* list all dataspaces at dataspace manager */
  return l4dm_ds_list(dsm_id, L4_INVALID_ID, 0);
}
