/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_phys/server/src/resize.c
 * \brief  DMphys, resize dataspace
 *
 * \date   22/11/2001
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
#include "__dm_phys.h"
#include "__dataspace.h"
#include "__internal_alloc.h"
#include "__pages.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Resize
 * 
 * \param  ds            Dataspace descriptor
 * \param  new_size      New dataspace size
 *	
 * \return 0 on success (resized dataspace), error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace
 *         - \c -L4_ENOMEM  memory area not available
 */
/*****************************************************************************/ 
static int
__resize(dmphys_dataspace_t * ds, 
	 l4_size_t new_size)
{
  int ret;
  l4_size_t old_size,add;
  page_area_t * pages;
  page_pool_t * pool;

  if (new_size == 0)
    {
      /* close dataspace */
      LOGdL(DEBUG_RESIZE,"new_size 0, close dataspace");

      ret = dmphys_close(ds);
      if (ret < 0)
	{
	  ERROR("DMphys: close dataspace failed: %d!",ret);
	  return ret;
	}
    }
  else
    {
      /* resize dataspace */
      old_size = dmphys_ds_get_size(ds);
      pages = dmphys_ds_get_pages(ds);
      pool = dmphys_ds_get_pool(ds);

      LOGdL(DEBUG_RESIZE,"size new 0x%x, old 0x%x",new_size,old_size);

      if (new_size > old_size)
	{
	  /* enlarge dataspace */
	  add = new_size - old_size;
	  add = (add + DMPHYS_PAGESIZE - 1) & DMPHYS_PAGEMASK;

	  if (DS_IS_CONTIGUOUS(ds))
	    {
	      /* phys. contiguous dataspace, try to enlarge page area */
#if DEBUG_RESIZE
	      printf("  contiguous ds, enlarge page area by 0x%x\n",add);
#endif
	      ret = dmphys_pages_enlarge(pool,pages,add,PAGES_USER);
	      if (ret < 0)
		{
		  ERROR("DMphys: enlarge page area list failed: %d!",ret);
		  return ret;
		}
	    }
	  else
	    {
#if DEBUG_RESIZE
	      printf("  allocate new page areas, size 0x%x\n",add);
#endif
	      ret = dmphys_pages_add(pool,pages,add,PAGES_USER);
	      if (ret < 0)
		{
		  ERROR("DMphys: add pages failed: %d!",ret);
		  return ret;
		}
	    }
	}
      else if (new_size < old_size)
	{
	  /* shrink dataspace */
#if DEBUG_RESIZE
	  printf("  shrink dataspace page area list\n");
#endif
	  ret = dmphys_pages_shrink(pool,pages,new_size);
	  if (ret < 0)
	    {
	      ERROR("DMphys: shrink page area list failed: %d!",ret);
	      return ret;
	    }
	}
      else
	{
	  /* kepp size, nothing to do */
#if DEBUG_RESIZE
	  DMSG("  keep size\n");
#endif  
	}

      /* set new dataspace size */
      dmphys_ds_set_size(ds);
    }

  /* we might have allocated internal memory, update memory pool */
  dmphys_internal_alloc_update();

  /* done */
  return 0;
}

/*****************************************************************************
 *** DMphys internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Resize dataspace
 * 
 * \param  ds            Dataspace descriptor
 * \param  new_size      New dataspace size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace
 *         - \c -L4_ENOMEM  memory area not available
 */
/*****************************************************************************/ 
int
dmphys_resize(dmphys_dataspace_t * ds, 
	      l4_size_t new_size)
{
  /* round new size to pagesize */
  new_size = (new_size + DMPHYS_PAGESIZE - 1) & DMPHYS_PAGEMASK;

  /* resize dataspace */
  return __resize(ds,new_size);
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Resize dataspace
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  new_size      New dataspace size
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (resized dataspace), error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace
 *         - \c -L4_EPERM   caller is not allowed to resize the dataspace
 *         - \c -L4_ENOMEM  memory area not available
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_mem_resize_component(CORBA_Object _dice_corba_obj,
                             l4_uint32_t ds_id,
                             l4_uint32_t new_size,
                             CORBA_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  
  LOGdL(DEBUG_RESIZE,"ds %u, caller %x.%x",
        ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);

  /* get dataspace descriptor, check if caller owns the dataspace */
  ret = dmphys_ds_get_check_rights(ds_id,*_dice_corba_obj,L4DM_RESIZE,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id %u, caller %x.%x",ds_id,
	      _dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	ERROR("DMphys: client %x.%x  is not allowed to resize ds %u",
	      _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,ds_id);
#endif      
      return ret;
    }

  /* round new size to pagesize */
  new_size = (new_size + DMPHYS_PAGESIZE - 1) & DMPHYS_PAGEMASK;

  /* resize dataspace */
  return __resize(ds,new_size);
}
