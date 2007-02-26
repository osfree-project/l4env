/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/transfer.c
 * \brief  Generic dataspace manager client library, 
 *         transfer dataspace ownership
 *
 * \date   01/23/2002
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
 * \brief Transfer dataspace ownership
 * 
 * \param  ds            Dataspace descriptor
 * \param  new_owner     New dataspace owner
 *	
 * \return 0 on success (set owner to \a new_owner), error code otherwise:
 *         - \c -L4_EIPC   IPC error calling dataspace manager
 *         - \c -L4_EINVAL Invalid dataspace descriptor
 *         - \c -L4_EPERM  Permission denied, only the current owner can 
 *                         transfer the ownership
 */
/*****************************************************************************/ 
int
l4dm_transfer(l4dm_dataspace_t * ds, 
	      l4_threadid_t new_owner)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  ret = if_l4dm_generic_transfer_call(&(ds->manager),ds->id,
                                      &new_owner,&_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_generic: transfer ownership failed, ds %u at %x.%x, "
	    "new owner %x.%x (ret %d, exc %d)!",ds->id, ds->manager.id.task,
	    ds->manager.id.lthread,new_owner.id.task,new_owner.id.lthread,
	    ret,_env.major);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return 0;
}
