/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/clients.c
 * \brief  DMphys, dataspace client handling
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
#include <l4/sys/types.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include "__internal_alloc.h"
#include "__dataspace.h"
#include "__debug.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Grant / extend dataspace access rights
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  client             Client thread id
 * \param  flags              Flags => access rights for client
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success (added \a client to dataspace client list), 
 *         error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   the requested rights for the new client exceed 
 *                        the rights of the caller for the dataspace
 *         - -#L4_ENOMEM  out of memory allocating client descriptor
 */
/*****************************************************************************/ 
long
if_l4dm_generic_share_component (CORBA_Object _dice_corba_obj,
                                 unsigned long ds_id,
                                 const l4_threadid_t *client,
                                 unsigned long flags,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;
  l4_uint32_t rights = flags & L4DM_RIGHTS_MASK;
  int ret;

  LOGdL(DEBUG_SHARE,
        "ds %lu, caller "l4util_idfmt", client "l4util_idfmt", rights 0x%02x",
        ds_id, l4util_idstr(*_dice_corba_obj), l4util_idstr(*client), rights);

  /* get dataspace descriptor, caller must have also the rights */
  ret = dmphys_ds_get_check_rights(ds_id, *_dice_corba_obj, rights, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	     ds_id, l4util_idstr(*_dice_corba_obj));
      else
	{
	  LOG_printf("ds %lu, owner "l4util_idfmt", caller "l4util_idfmt
	         ", rights 0x%02x, client "l4util_idfmt", rights 0x%02x\n",
                 ds_id, l4util_idstr(dsmlib_get_owner(ds->desc)),
		 l4util_idstr(*_dice_corba_obj),
                 dmphys_ds_get_rights(ds, *_dice_corba_obj),
                 l4util_idstr(*client), rights);
	  LOGL("DMphys: bad permissions!");
	}
#endif
      return ret;
    }

  /* add client, if the client already exists this will add the rights 
   * to the already existing rights of the client */
  ret = dmphys_ds_add_client(ds, *client,rights);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      LOG_printf("ds %lu, caller "l4util_idfmt", client "l4util_idfmt \
             ", rights 0x%02x\n", ds_id, l4util_idstr(*_dice_corba_obj),
             l4util_idstr(*client), rights);
      LOGL("DMphys: add client failed: %d!", ret);
#endif
      return ret;
    }

  /* we might have allocated internal memory, update memory pool */
  dmphys_internal_alloc_update();

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Revoke / restrict dataspace access rights
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  client             Client thread id
 * \param  flags              Rights to revoke
 * \param  _dice_corba_env    Server environment
 * 	
 * \return 0 on success (revoked access rights), error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   requested operations not allowed
 * 
 * Revoke the specifed access rights. If the resulting access rights are 0, 
 * remove the client from dataspace client list. 
 */
/*****************************************************************************/ 
long
if_l4dm_generic_revoke_component (CORBA_Object _dice_corba_obj,
                                  unsigned long ds_id,
                                  const l4_threadid_t *client,
                                  unsigned long flags,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;
  l4_uint32_t rights = flags & L4DM_RIGHTS_MASK;
  l4_uint32_t old_rights;
  int ret;

  /* get dataspace descriptor, caller must have the rights he wants 
   * to revoke */
  ret = dmphys_ds_get_check_rights(ds_id, *_dice_corba_obj, rights, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace %ld, caller "l4util_idfmt,
             ds_id, l4util_idstr(*_dice_corba_obj));
      else
	{
	  LOG_printf("ds %lu, caller "l4util_idfmt", rights 0x%02x, " \
                 "client "l4util_idfmt", rights 0x%02x\n",
                 ds_id, l4util_idstr(*_dice_corba_obj),
                 dmphys_ds_get_rights(ds, *_dice_corba_obj),
                 l4util_idstr(*client), rights);
	  LOGL("DMphys: bad permissions!");
	}
#endif
      return -L4_EINVAL;
    }

  /* get old rights */
  old_rights = dmphys_ds_get_rights(ds,*client);

  LOGdL(DEBUG_REVOKE, "ds %lu, caller "l4util_idfmt", client "l4util_idfmt", " \
        "revoke 0x%02x, has 0x%02x", ds_id, l4util_idstr(*_dice_corba_obj), 
        l4util_idstr(*client), rights, old_rights);
  
  if (old_rights == 0)
    /* nothing to do */
    return 0;

  rights = old_rights & ~rights;
  if (rights == 0)
    {
      /* remove client */
#if DEBUG_REVOKE
      LOG_printf(" new rights 0x%02x, remove client\n", rights);
#endif
      ret = dmphys_ds_remove_client(ds,*client);
      if (ret < 0)
	{
	  LOGdL(DEBUG_ERRORS, "DMphys: remove client failed: %d", ret);
	  return -L4_EINVAL;
	}
    }
  else
    {
      /* restrict access rights */
#if DEBUG_REVOKE
      LOG_printf(" new rights 0x%02x\n", rights);
#endif
      ret = dmphys_ds_set_rights(ds, *client, rights);
      if (ret < 0)
	{
	  LOGdL(DEBUG_ERRORS, "DMphys: set access rights failed: %d", ret);
	  return -L4_EINVAL;
	}
    }

  /* TODO: unmap ds at client */

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Check access rights for calling client
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  flags              Flags => access rights
 * \param  _dice_corba_env    Server environment
 *
 * \return 0 if caller has the requested rights, error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   requested operations not allowed
 */
/*****************************************************************************/
long
if_l4dm_generic_check_rights_component (CORBA_Object _dice_corba_obj,
                                        unsigned long ds_id,
                                        unsigned long flags,
                                        CORBA_Server_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;
  l4_uint32_t rights = flags & L4DM_RIGHTS_MASK;
  int ret;

  /* check rights */
  ret = dmphys_ds_get_check_rights(ds_id, *_dice_corba_obj, rights, &ds);
  if (ret == -L4_EINVAL)
    LOGdL(DEBUG_ERRORS,
          "DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	  ds_id, l4util_idstr(*_dice_corba_obj));

  /* done */
  return ret;
}
