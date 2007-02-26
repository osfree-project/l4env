/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/info.c
 * \brief  DMphys, return dataspace information for debugging purposes
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
 * \brief  Return dataspace information.
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id.
 *                            the first dataspace id.
 * \retval size               Dataspace size
 * \retval owner              Dataspace owner
 * \retval name               Dataspace name
 * \retval next_id            Next dataspace id for iteration if
 *                            ds_id != L4DM_INVALID_DATASPACE.id else first
 *                            dataspace id for iteration.
 * \retval _dice_corba_env    Server environment
 *	
 * \return 0 on success (\a size contains the dataspace size), 
 *         error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   Caller is not a client of the dataspace
 *
 * XXX This function is a security risk! It should only be used for debugging
 * purposes.
 */
/*****************************************************************************/ 
l4_int32_t
if_l4dm_mem_info_component (CORBA_Object _dice_corba_obj,
                            l4_uint32_t ds_id,
                            l4_uint32_t *size,
                            l4_threadid_t *owner,
                            char **name,
                            l4_uint32_t *next_id,
                            CORBA_Server_Environment *_dice_corba_env)
{
  dsmlib_ds_desc_t *desc;
  dmphys_dataspace_t *ds;

  if (ds_id == L4DM_INVALID_DATASPACE.id)
    {
      /* return first dataspace id */
      if (!(desc = dsmlib_get_dataspace_list()))
	return -L4_ENOTFOUND;

      *size    = 0;
      *owner   = L4_INVALID_ID;
      *name    = "";
      *next_id = desc->id;
      return 0;
    }

  desc = dsmlib_get_dataspace(ds_id);

  if (!desc || !(ds = dsmlib_get_dsm_ptr(desc)))
    return -L4_ENOTFOUND;

  *size    = dmphys_ds_get_size(ds);
  *owner   = dsmlib_get_owner(ds->desc);
  *name    = dmphys_ds_get_name(ds);
  *next_id = desc->ds_next ? desc->ds_next->id : L4DM_INVALID_DATASPACE.id;
  return 0;
}
