/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/size.c
 * \brief  DMphys, return dataspace size
 * 
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/env/errno.h>
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
 * \brief  Return dataspace size
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval size          Dataspace size
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (\a size contains the dataspace size), 
 *         error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   Caller is not a client of the dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_mem_size_component(CORBA_Object _dice_corba_obj,
                           l4_uint32_t ds_id,
                           l4_uint32_t *size,
                           CORBA_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;

  /* get dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id,*_dice_corba_obj,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id, id %u, caller %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	ERROR("DMphys: caller %x.%x is not a client of dataspace %d!",
	      _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,ds_id);
#endif
      return ret;
    }
  
  /* set dataspace size */
  *size = dmphys_ds_get_size(ds);

  /* done */
  return 0;
}
