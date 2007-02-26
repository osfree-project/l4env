/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/info.c
 * \brief  Memory dataspace manager client library, return dataspace size
 *
 * \date   09/21/2005
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_mem/dm_mem-client.h>
#include "__debug.h"

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return dataspace size
 * 
 * \param  ds            Dataspace descriptor
 * \retval size          Dataspace size
 * \retval owner         Dataspace owner
 * \retval name          Dataspace name
 * \retval next_id       Next dataspace id
 *	
 * \return 0 on success (\a size contains the dataspace size), 
 *         error code otherwise:
 *         - -#L4_EIPC    IPC error calling dataspace manager
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   Caller is not a client of the dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_info(const l4dm_dataspace_t * ds, l4_size_t * size,
	      l4_threadid_t *owner, char * name, l4_uint32_t *next_id)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_mem_info_call(&(ds->manager), ds->id, size, owner,
			        &name, next_id, &_env);
  if (ret)
    return ret;
  if (_env.major != CORBA_NO_EXCEPTION)
    return -L4_EIPC;
  return 0;
}
