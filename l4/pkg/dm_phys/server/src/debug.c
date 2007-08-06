/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/debug.c
 * \brief  DMphys, debug functions
 *
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include <l4/dm_phys/consts.h>
#include "__dataspace.h"
#include "__dm_phys.h"
#include "__pages.h"
#include "__debug.h"

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Dump iterator argument 
 */
typedef struct dump_arg
{
  l4_size_t   sum;
  l4_size_t   size;
  char *      str_buf;
} dump_arg_t;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  List dataspaces iterator function
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Data pointer, total size of all dataspaces
 */
/*****************************************************************************/ 
static void
__ds_list_iterator(dmphys_dataspace_t * ds, void * data)
{
  l4_size_t * sum = data;
  l4_threadid_t ds_owner = dsmlib_get_owner(ds->desc);

  LOG_printf("%4d: size=%08zx  owner="l4util_idfmt"  name=\"%s\"\n",
         dmphys_ds_get_id(ds), ds->size, l4util_idstr(ds_owner),
         dmphys_ds_get_name(ds));
  *sum += ds->size;
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Set dataspace name
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  name               Dataspace name
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   caller is not the owner of the dataspace
 */
/*****************************************************************************/ 
long
if_l4dm_generic_set_name_component (CORBA_Object _dice_corba_obj,
                                    unsigned long ds_id,
                                    const char* name,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;

  /* get dataspace descriptor */
  ret = dmphys_ds_get_check_owner(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id %lu, caller "l4util_idfmt,
             ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: client "l4util_idfmt" does not own dataspace %lu",
	     l4util_idstr(*_dice_corba_obj), ds_id);
#endif
      return ret;
    }

  /* set name */
  dmphys_ds_set_name(ds, name);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Return dataspace name
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  _dice_corba_env    Server environment
 * \retval name               Dataspace name
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 */
/*****************************************************************************/ 
long
if_l4dm_generic_get_name_component (CORBA_Object _dice_corba_obj,
                                    unsigned long ds_id,
                                    char **name,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;

  /* get dataspace descriptor */
  ds = dmphys_ds_get(ds_id);
  if (ds == NULL)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: invalid dataspace %lu, caller "l4util_idfmt,
            ds_id, l4util_idstr(*_dice_corba_obj));
      return -L4_EINVAL;
    }

  /* return name */
  *name = dmphys_ds_get_name(ds);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Show dataspace information on debugging console
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid dataspace id
 */
/*****************************************************************************/ 
long
if_l4dm_generic_show_ds_component (CORBA_Object _dice_corba_obj,
                                   unsigned long ds_id,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;

  /* get dataspace descriptor */
  ds = dmphys_ds_get(ds_id);
  if (ds == NULL)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: invalid dataspace %lu, caller "l4util_idfmt,
            ds_id, l4util_idstr(*_dice_corba_obj));
      return -L4_EINVAL;
    }

  /* show dataspace info */
  dmphys_ds_show(ds);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  List dataspaces on debugging console
 * 
 * \param  _dice_corba_obj    Request source
 * \param  owner              Dataspace owner, if set to #L4_INVALID_ID list
 *                            all dataspaces
 * \param  flags              Flags:
 *                            - #L4DM_SAME_TASK  list dataspaces owned by task
 * \param  _dice_corba_env    Server environment
 */
/*****************************************************************************/ 
void
if_l4dm_generic_list_component (CORBA_Object _dice_corba_obj,
                                const l4_threadid_t *owner,
                                unsigned long flags,
                                CORBA_Server_Environment *_dice_corba_env)
{
  l4_size_t sum = 0;

  if (l4_is_invalid_id(*owner))
    LOG_printf("DMphys dataspace list:\n");
  else
    LOG_printf("DMphys dataspace list for client "l4util_idfmt":\n",
           l4util_idstr(*owner));

  /* iterate dataspace list */
  dmphys_ds_iterate(__ds_list_iterator, &sum, *owner, flags);

  if (sum > 0)
    LOG_printf("===========================================================\n"
           "total size %08zx (%zdkB, %zdMB)\n", 
	   sum, (sum+(1<<9)-1)/(1<<10), (sum+(1<<19)-1)/(1<<20));
  else
    LOG_printf("no suitable dataspace found\n");

  /* done */
}
