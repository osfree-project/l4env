/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/close.c
 * \brief  DMphys, close dataspace
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
#include "__pages.h"
#include "__internal_alloc.h"
#include "__dm_phys.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Close dataspace
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Iterator functio data, ignored
 */
/*****************************************************************************/ 
static void
__close(dmphys_dataspace_t * ds, void * data)
{
  page_area_t * pages;
  page_pool_t * pool;

  LOGdL(DEBUG_CLOSE, "close dataspace %d, client "l4util_idfmt,
        dmphys_ds_get_id(ds), l4util_idstr(dsmlib_get_owner(ds->desc)));
 
  /* get page area list / page pool */
  pages = dmphys_ds_get_pages(ds);
  pool = dmphys_ds_get_pool(ds);

  /* unmap pages */
  dmphys_unmap_areas(pages);
  
  /* release pages */
  dmphys_pages_release(pool, pages);

  /* release dataspace descriptor */
  dmphys_ds_release(ds);

  /* update internal memory allocation */
  dmphys_internal_alloc_update();
}

/*****************************************************************************
 *** DMphys internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Close dataspace
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return 0 on success.
 */
/*****************************************************************************/ 
int
dmphys_close(dmphys_dataspace_t * ds)
{
  /* close */
  __close(ds, NULL);

  return 0;
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Close dataspace
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Dataspace id
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success (closed dataspace), error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace id
 *         - -#L4_EPERM   operation not permitted, only the owner can 
 *                        close a dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_generic_close_component(CORBA_Object _dice_corba_obj,
                                l4_uint32_t ds_id,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;

  LOGdL(DEBUG_CLOSE, "close ds %u", ds_id);

  /* get dataspace descriptor, check if caller owns the dataspace */
  ret = dmphys_ds_get_check_owner(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id %u, caller "l4util_idfmt,
             ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: client "l4util_idfmt" does not own dataspace %u "
	     "(owner is "l4util_idfmt")",
	     l4util_idstr(*_dice_corba_obj), ds_id,
	     l4util_idstr(dsmlib_get_owner(ds->desc)));
#endif      
      return ret;
    }

  /* close */
  __close(ds,NULL);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Close all dataspaces of a client.
 * 
 * \param  _dice_corba_obj    Request source
 * \param  client             Client thread id
 * \param  flags              Flags:
 *                            - #L4DM_SAME_TASK  close all dataspaces owned by
 *                                               threads of the task specified 
 *                                               by \a client
 * \param  _dice_corba_env    Server environment
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid client id
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_generic_close_all_component(CORBA_Object _dice_corba_obj,
                                    const l4_threadid_t *client,
                                    l4_uint32_t flags,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  if (l4_is_invalid_id(*client))
    return -L4_EINVAL;

  /* We should check whether the caller is allowd to close all dataspaces.
   * close_all can be used e.g. by a loader/task-server to release all 
   * dataspaces created by a client after the client has exited. 
   * Therefore we should allow close_all for non-owners of dataspaces, but
   * only for those we trust. */
  LOGdL(DEBUG_CLOSE, "close all dataspaces owned by "l4util_idfmt
      		     ", caller "l4util_idfmt"\n",
		     l4util_idstr(*client), l4util_idstr(*_dice_corba_obj));

  /* close dataspaces */
  dmphys_ds_iterate(__close, NULL, *client, flags);

  /* done */
  return 0;
}
