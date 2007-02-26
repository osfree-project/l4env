/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/transfer.c
 * \brief  DMphys, transfer dataspace ownership
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
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include "__dataspace.h"
#include "__debug.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Transfer the ownership of a dataspace
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  new_owner     New owner of the dataspace
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (set the owner to \a new_owner), error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id  
 *         - \c -L4_EPERM   permission denied, only the current owner can 
 *                          transfer the ownership.
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_generic_transfer_component(CORBA_Object _dice_corba_obj,
                                   l4_uint32_t ds_id,
                                   const l4_threadid_t *new_owner,
                                   CORBA_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;

  LOGdL(DEBUG_TRANSFER,"ds %u\n  caller %x.%x, new owner %x.%x",ds_id,
        _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,
        new_owner->id.task,new_owner->id.lthread);
  
  /* get dataspace descriptor, check if caller owns the dataspace */
  ret = dmphys_ds_get_check_owner(ds_id,*_dice_corba_obj,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id,id %u, caller %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread); 
      else
	ERROR("DMphys: caller is not the current owner "
	      "(ds %u, caller %x.%x)!",ds_id,_dice_corba_obj->id.task,
	      _dice_corba_obj->id.lthread);
#endif	
      return ret;
    }

  /* set new owner */
  dmphys_ds_set_owner(ds,*new_owner);

  /* done */
  return 0;
}

