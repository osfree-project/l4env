/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/region_tree.c
 * \brief  Region mapper library, region tree handling.
 *
 * \date   01/23/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 * 
 * This file contains the implementation of the Region Mapper thread IPC
 * interface. To avoid pagefaults in the IPC-call, all arguments are passed 
 * to the RM thread in global variables. Synchronizing the accesses to these
 * variables is done using the region list lock, it is done at a higher level
 * (add / remove / lookup region).
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

/* L4RM includes */
#include "l4rm-server.h"
#include "l4rm-client.h"
#include "__avl_tree.h"
#include "__region.h"
#include "__alloc.h"
#include "__debug.h"

// was in __libl4rm.h
extern l4_threadid_t l4rm_service_id;

/*****************************************************************************
 *** Global data
 *****************************************************************************/

/**
 * IPC arguments
 */
static avlt_key_t    arg_key;    ///< in:     entry key
static l4_addr_t     arg_addr;   ///< in:     VM address
static l4_uint32_t   arg_type;   ///< in:     region type 
static avlt_data_t   arg_data;   ///< in/out: entry data
 
/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add entry to region tree
 *	
 * \return 0 on success (inserted entry in tree), error code otherwise:
 *         - -#L4_EEXISTS  key already exists
 *         - -#L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
static int
__add_region(void)
{
  int ret;

  LOGdL(DEBUG_REGION_TREE,"adding region 0x%08x-0x%08x, desc at 0x%08x",
        arg_key.start, arg_key.end, (unsigned)arg_data);
  
  /* insert entry */
  ret = avlt_insert(arg_key, arg_data);
  if (ret < 0)
    {
      if (ret == -AVLT_NO_MEM)
	return -L4_ENOMEM;
      else
	return -L4_EEXISTS;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Remove entry from region tree
 * 
 * \return 0 on success (removed entry from tree, stored entry in arg_data),
 *         error code otherwise:
 *         - -#L4_ENOTFOUND  key not found
 *         - -#L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
static int
__remove_region(void)
{
  int ret;
  avlt_key_t key;
  l4rm_region_desc_t *r;

  LOGdL(DEBUG_REGION_TREE, "removing region at addr 0x%08x", arg_addr);

  /* find entry */
  key.start = key.end = arg_addr;
  ret = avlt_find(key, &arg_data);
  if (ret < 0)
    return -L4_ENOTFOUND;
  r = (l4rm_region_desc_t *)arg_data;

  LOGd(DEBUG_REGION_TREE, "region 0x%08x-%#08x, flags 0x%08x\n", 
       r->start, r->end, r->flags);

  if (REGION_TYPE(r) == arg_type)
    {
      /* remove entry */
      key.start = r->start;
      key.end = r->end;
      ret = avlt_remove(key);
      if (ret < 0)
        return -L4_EINVAL;
    }
  else
    {
      LOG_Error("invalid region type for region 0x%08x-0x%08x,\n " \
                "requested type 0x%08x, got 0x%08x",
                r->start, r->end, arg_type, REGION_TYPE(r));
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add entry to region tree
 * 
 * \param  request       Flick request structure
 * \param  _ev           Flick exception structure, unused	
 * 
 * \return 0 on success (inserted entry in tree), error code otherwise:
 *         - -#L4_EEXISTS  key already exists
 *         - -#L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_add_component(CORBA_Object _dice_corba_obj,
                    CORBA_Server_Environment *_dice_corba_env)
{
  if (!l4_task_equal(*_dice_corba_obj, l4rm_service_id))
    {
      LOG_Error("L4RM: blocked message from outside ("l4util_idfmt")!",
                l4util_idstr(*_dice_corba_obj));
      return DICE_NO_REPLY;
    }

  /* add region */
  return __add_region();
}

/*****************************************************************************/
/**
 * \brief Find and remove entry from region tree.
 * 
 * \param  request       Flick request structure
 * \param  _ev           Flick exception structure, unused
 *	
 * \return 0 on success (removed entry from tree, stored entry in arg_data),
 *         error code otherwise:
 *         - -#L4_ENOTFOUND  key not found
 *         - -#L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_remove_component(CORBA_Object _dice_corba_obj,
                       CORBA_Server_Environment *_dice_corba_env)
{
  if (!l4_task_equal(*_dice_corba_obj, l4rm_service_id))
    {
      LOG_Error("L4RM: blocked message from outside ("l4util_idfmt")!",
                l4util_idstr(*_dice_corba_obj));
      return DICE_NO_REPLY;
    }

  /* remove region */
  return __remove_region();
}

/*****************************************************************************
 *** L4RM library internal API
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Call region mapper thread to add a region into the region tree.
 * 
 * \param  region        Region descriptor
 * \param  flags         Flags:
 *                       - #L4RM_MODIFY_DIRECT do not call region mapper thread,
 *                                             add region directly instead
 *	
 * \return 0 on success (added region), error code otherwise:
 *         - -#L4_EIPC     IPC error calling region mapper thread
 *         - -#L4_EEXISTS  key already exists
 *         - -#L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
int
l4rm_tree_insert_region(l4rm_region_desc_t * region, l4_uint32_t flags)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* set argument buffers */
  arg_key.start = region->start;
  arg_key.end = region->end;
  arg_data = region;

  if (flags & L4RM_MODIFY_DIRECT)
    /* add region directly */
    return __add_region();
  else
    {
      /* call region mapper thread */
      ret = l4_rm_add_call(&l4rm_service_id, &env);
      if (ret || (env.major != CORBA_NO_EXCEPTION))
	{
	  LOG_Error("L4RM: call region mapper failed: ret %d, exc %d!",
                    ret, env.major);
	  if (ret)
	    return ret;
	  else
	    return -L4_EIPC;
	}
      
      /* done */
      return 0;
    }
}

/*****************************************************************************/
/**
 * \brief Call region mapper thread to find and remove a region from the 
 *        region tree.
 * 
 * \param  addr          VM area address
 * \param  type          Region type, check if region is of that type
 * \param  flags         Flags:
 *                       - #L4RM_MODIFY_DIRECT  do not call region mapper
 *                                              thread, add region directly 
 *                                              instead
 * \retval region        Region descriptor
 *	
 * \return 0 on success (removed region, \a region contains the descriptor),
 *         error code otherwise:
 *         - -#L4_EIPC       IPC error calling region mapper thread
 *         - -#L4_ENOTFOUND  key not found
 *         - -#L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
int
l4rm_tree_remove_region(l4_addr_t addr, l4_uint32_t type, l4_uint32_t flags, 
			l4rm_region_desc_t ** region)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* set argument buffer */
  arg_addr = addr;
  arg_type = type;

  if (flags & L4RM_MODIFY_DIRECT)
    /* remove region directly */
    return __remove_region();
  else
    {
      /* call region mapper thread */
      ret = l4_rm_remove_call(&l4rm_service_id, &env);
      if (ret || (env.major != CORBA_NO_EXCEPTION))
	{
	  *region = NULL;
	  LOG_Error("L4RM: call region mapper failed: ret %d, exc %d!",
                    ret, env.major);
	  if (ret)
	    return ret;
	  else
	    return -L4_EIPC;
	}
    }

  *region = (l4rm_region_desc_t *)arg_data;

  /* done */
  return 0;
}
