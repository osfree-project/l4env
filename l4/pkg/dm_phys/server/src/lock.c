/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/lock.c
 * \brief  DMphys, lock/unlock dataspace region
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
#include <l4/sys/types.h>
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
 * \brief  Lock dataspace region
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  offset             Offset in dataspace
 * \param  size               Region size
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  invalid dataspace region
 */
/*****************************************************************************/ 
long
if_l4dm_mem_lock_component (CORBA_Object _dice_corba_obj,
                            unsigned long ds_id,
                            unsigned long offset,
                            unsigned long size,
                            CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_size_t ds_size;

  /* we do not need to do anything, just check dataspace and region */
  ret = dmphys_ds_get_check_client(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	     ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: caller "l4util_idfmt" is not a client of dataspace %ld!",
	      l4util_idstr(*_dice_corba_obj), ds_id);
#endif
      return ret;
    }

  ds_size = dmphys_ds_get_size(ds);
  if (offset + size > ds_size)
    return -L4_EINVAL_OFFS;

  LOGdL(DEBUG_LOCK, "ds %lu, caller "l4util_idfmt",  offset 0x%08lx, " \
        "size 0x%08lx, ds size 0x%08zx", ds_id,
        l4util_idstr(*_dice_corba_obj), offset, size, ds_size);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Unlock dataspace region
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  offset             Offset in dataspace
 * \param  size               Region size
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  invalid dataspace region
 */
/*****************************************************************************/ 
long
if_l4dm_mem_unlock_component (CORBA_Object _dice_corba_obj,
                              unsigned long ds_id,
                              unsigned long offset,
                              unsigned long size,
                              CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_size_t ds_size;

  /* we do not need to do anything, just check dataspace and region */
  ret = dmphys_ds_get_check_client(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	      ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: caller "l4util_idfmt" is not a client of dataspace %ld!",
	      l4util_idstr(*_dice_corba_obj), ds_id);
#endif
      return ret;
    }

  ds_size = dmphys_ds_get_size(ds);
  if (offset + size > ds_size)
    return -L4_EINVAL_OFFS;

  LOGdL(DEBUG_LOCK, "ds %lu, caller "l4util_idfmt" offset 0x%08lx, " \
        "size 0x%08lx, ds size 0x%08zx", ds_id, l4util_idstr(*_dice_corba_obj),
        offset, size, ds_size);
  
  /* done */
  return 0;
}
