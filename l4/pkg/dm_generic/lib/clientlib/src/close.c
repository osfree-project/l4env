/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/close.c
 * \brief  Generic dataspace manager client library, close dataspace
 *
 * \date   11/23/2001
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
#include "__debug.h"

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
 *         - -#L4_EIPC    IPC error calling dataspace manager
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   operation not permitted, only the owner can 
 *                        close a dataspace
 */
/*****************************************************************************/ 
int
l4dm_close(const l4dm_dataspace_t * ds)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_close_call(&(ds->manager), ds->id, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: close dataspace %u at "l4util_idfmt \
            " failed (ret %d, exc %d)!",ds->id, l4util_idstr(ds->manager), 
            ret, _env.major);
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
 *         - -#L4_EINVAL  invalid client thread id
 *         - -#L4_EPERM   permission denied
 *         - -#L4_EIPC    IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_close_all(l4_threadid_t dsm_id, l4_threadid_t client, l4_uint32_t flags)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_generic_close_all_call(&dsm_id, &client, flags, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: close dataspaces of "l4util_idfmt \
            " failed (ret %d, exc %d)!", l4util_idstr(client), ret, _env.major);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}
