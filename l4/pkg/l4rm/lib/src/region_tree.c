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
 * variables is not done, it must be done at a higher level.
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* L4RM includes */
#include "l4rm-server.h"
#include "l4rm-client.h"
#include "__libl4rm.h"
#include "__avl_tree.h"
#include "__region.h"
#include "__alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** Global data
 *****************************************************************************/

/**
 * IPC arguments
 */
static avlt_key_t    arg_key;    ///< in:     entry key
static l4_addr_t     arg_addr;   ///< in:     VM address
static avlt_data_t   arg_data;   ///< in/out: entry data
static l4_threadid_t arg_client; ///< in:     region mapper client
 
/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add entry to region tree
 *	
 * \return 0 on success (inserted entry in tree), error code otherwise:
 *         - \c -L4_EEXISTS  key already exists
 *         - \c -L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
static inline int
__add_region(void)
{
  int ret;

#if DEBUG_REGION_TREE
  INFO("\n");
  DMSG("  adding region 0x%08x-0x%08x, desc at 0x%08x\n",
       arg_key.start,arg_key.end,(unsigned)arg_data);
#endif

  /* insert entry */
  ret = avlt_insert(arg_key,arg_data);
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
 *         - \c -L4_ENOTFOUND  key not found
 *         - \c -L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
static inline int
__remove_region(void)
{
  int ret;
  avlt_key_t key;

#if DEBUG_REGION_TREE
  INFO("\n");
  DMSG("  removing region at addr 0x%08x\n",arg_addr);
#endif

  /* find entry */
  key.start = key.end = arg_addr;
  ret = avlt_find(key,&arg_data);
  if (ret < 0)
    return -L4_ENOTFOUND;

#if DEBUG_REGION_TREE
  DMSG("  region 0x%08x-0x%08x\n",((l4rm_region_desc_t *)arg_data)->start,
       ((l4rm_region_desc_t *)arg_data)->end);
#endif

  /* remove entry */
  key.start = ((l4rm_region_desc_t *)arg_data)->start;
  key.end = ((l4rm_region_desc_t *)arg_data)->end;
  ret = avlt_remove(key);
  if (ret < 0)
    return -L4_EINVAL;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Add region mapper client
 *	
 * \return 0 on success
 */
/*****************************************************************************/ 
static inline int
__add_client(void)
{
  /* Right now we have no real client handling, access control is based 
   * on task ids. Just allow access to the L4RM heap for the client */
  l4rm_heap_add_client(arg_client);

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
 *         - \c -L4_EEXISTS  key already exists
 *         - \c -L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_server_add(sm_request_t * request, 
		 sm_exc_t * _ev)
{
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
 *         - \c -L4_ENOTFOUND  key not found
 *         - \c -L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_server_remove(sm_request_t * request, 
		    sm_exc_t * _ev)
{
  /* remove region */
  return __remove_region();
}

/*****************************************************************************/
/**
 * \brief Lookup key in region tree.
 * 
 * \param  request       Flick request structure
 * \param  _ev           Flick exception structure, unused
 *	
 * \return 0 on success (found entry, stored data in arg_data), 
 *         error code otherwise:
 *         - \c -L4_ENOTFOUND  key not found
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_server_lookup(sm_request_t * request, 
		    sm_exc_t * _ev)
{
  int ret;
  avlt_key_t key;

#if DEBUG_REGION_TREE
  INFO("\n");
  DMSG("  searching region at addr 0x%08x\n",arg_addr);
#endif

  /* find entry */
  key.start = key.end = arg_addr;
  ret = avlt_find(key,&arg_data);
  if (ret < 0)
    return -L4_ENOTFOUND;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Add region mapper client
 * 
 * \param  request       Flick request structure
 * \param  _ev           Flick exception structure, unused
 *	
 * \return 0 on success
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_server_add_client(sm_request_t * request, 
			sm_exc_t * _ev)
{
  /* add client */
  return __add_client();
}

/*****************************************************************************/
/**
 * \brief  Remove region mapper client
 * 
 * \param  request       Flick request structure
 * \param  _ev           Flick exception structure, unused
 *	
 * \return 0 on success
 */
/*****************************************************************************/ 
l4_int32_t 
l4_rm_server_remove_client(sm_request_t * request, 
			   sm_exc_t * _ev)
{
  l4rm_heap_remove_client(arg_client);

  return 0;
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
 *                       - \c MODIFY_DIRECT  do not call region mapper thread,
 *                                           add region directly instead
 *	
 * \return 0 on success (added region), error code otherwise:
 *         - \c -L4_EIPC     IPC error calling region mapper thread
 *         - \c -L4_EEXISTS  key already exists
 *         - \c -L4_ENOMEM   out of memory allocating tree node
 */
/*****************************************************************************/ 
int
l4rm_tree_insert_region(l4rm_region_desc_t * region, 
			l4_uint32_t flags)
{
  int ret;
  sm_exc_t exc;

  /* set argument buffers */
  arg_key.start = region->start;
  arg_key.end = region->end;
  arg_data = region;

  if (flags & MODIFY_DIRECT)
    /* add region directly */
    return __add_region();
  else
    {
      /* call region mapper thread */
      ret = l4_rm_add(l4rm_service_id,&exc);
      if (ret || (exc._type != exc_l4_no_exception))
	{
	  ERROR("L4RM: call region mapper failed: ret %d, exc %d!",
		ret,exc._type);
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
 * \param  flags         Flags:
 *                       - \c MODIFY_DIRECT  do not call region mapper thread,
 *                                           add region directly instead
 * \retval region        Region descriptor
 *	
 * \return 0 on success (removed region, \a region contains the descriptor),
 *         error code otherwise:
 *         - \c -L4_EIPC       IPC error calling region mapper thread
 *         - \c -L4_ENOTFOUND  key not found
 *         - \c -L4_EINVAL     invalid AVL tree
 */
/*****************************************************************************/ 
int
l4rm_tree_remove_region(l4_addr_t addr, 
			l4_uint32_t flags, 
			l4rm_region_desc_t ** region)
{
  int ret;
  sm_exc_t exc;

  /* set argument buffer */
  arg_addr = addr;
  
  if (flags & MODIFY_DIRECT)
    /* remove region directly */
    return __remove_region();
  else
    {
      /* call region mapper thread */
      ret = l4_rm_remove(l4rm_service_id,&exc);
      if (ret || (exc._type != exc_l4_no_exception))
	{
	  *region = NULL;
	  ERROR("L4RM: call region mapper failed: ret %d, exc %d!",
		ret,exc._type);
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

/*****************************************************************************/
/**
 * \brief Call region mapper thread to find an entry in the region tree.
 * 
 * \param  addr          VM area address
 * \retval region        Region descriptor
 *	
 * \return 0 on success (\a region contains the descriptor),
 *         error code otherwise:
 *         - \c -L4_EIPC       IPC error calling region mapper thread
 *         - \c -L4_ENOTFOUND  key not found
 */
/*****************************************************************************/ 
int
l4rm_tree_lookup_region(l4_addr_t addr, 
			l4rm_region_desc_t ** region)
{
  int ret;
  sm_exc_t exc;

  /* set argument buffer */
  arg_addr = addr;

  /* call region mapper thread */
  ret = l4_rm_lookup(l4rm_service_id,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      *region = NULL;
      ERROR("L4RM: call region mapper failed: ret %d, exc %d!",ret,exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }

  *region = (l4rm_region_desc_t *)arg_data;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Call region mapper thread to add a new client
 * 
 * \param  client        Client thread id
 * \param  flags         Flags:
 *                       - \c MODIFY_DIRECT  do not call region mapper thread,
 *                                           add client directly instead
 *	
 * \return 0 on success, \c -L4_EIPC if call failed
 */
/*****************************************************************************/ 
int
l4rm_tree_add_client(l4_threadid_t client, 
		     l4_uint32_t flags)
{
  int ret;
  sm_exc_t exc;

  /* grab region list lock, it also protects the IPC argument buffers */
  l4rm_lock_region_list_direct(flags);

  /* set argument buffer */
  arg_client = client;

  if (flags & MODIFY_DIRECT)
    /* add region directly */
    ret = __add_client();
  else
    {
      /* call region mapper thread */
      ret = l4_rm_add_client(l4rm_service_id,&exc);
      if (ret || (exc._type != exc_l4_no_exception))
	{
	  ERROR("L4RM: call region mapper failed: ret %d, exc %d!",ret,exc._type);
	  if (!ret)
	    ret = -L4_EIPC;
	}
    }

  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);

  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Call region mapper thread to remove a client
 * 
 * \param  client        Client thread id
 *	 
 * \return 0 on success, \c -L4_EIPC if call failed
 */
/*****************************************************************************/ 
int
l4rm_tree_remove_client(l4_threadid_t client)
{
  int ret;
  sm_exc_t exc;

  /* grab region list lock, it also protects the IPC argument buffers */
  l4rm_lock_region_list();

  /* set argument buffer */
  arg_client = client;

  /* call region mapper thread */
  ret = l4_rm_remove_client(l4rm_service_id,&exc);
  if (ret || (exc._type != exc_l4_no_exception))
    {
      ERROR("L4RM: call region mapper failed: ret %d, exc %d!",ret,exc._type);
      if (!ret)
	ret = -L4_EIPC;
    }

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return ret;
}
