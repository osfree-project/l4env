/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/memory.c
 * \brief  Memory allocation.
 *
 * \date   08/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
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
 *
 * Stack memory allocation. The memory is requested at the default dataspace
 * manager definied by the L4 environment (see l4th_env_dm()).
 *
 * \todo Allow user specified dataspace managers.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__memory.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate memory.
 * 
 * \param  size          Requested memory size (bytes)
 * \param  map_addr      Map address (#VM_FIND_REGION find suitable address)
 * \param  vm_area       Map in reserved vm area (#VM_DEFAULT_AREA use 
 *                       default area)
 * \param  owner         Owner of dataspace, no owner is set if #L4_INVALID_ID
 * \param  name          Dataspace name 
 * \param  flags         Flags:
 *                       - #L4THREAD_CREATE_SETUP use direct attach
 *                       - #L4THREAD_CREATE_MAP   immediately map memory
 * \retval desc          Memory descriptor
 *
 * \return 0 on success (allocated pages), error code otherwise:
 *         - -#L4_ENODM  no dataspace manager found
 *         - -#L4_ENOMEM out of memory
 *         - -#L4_EIPC   IPC error calling dataspace manager / region mapper
 *         - -#L4_ENOMAP no map area found
 *
 * Allocate and map pages. If \a map_addr is specified, try to map to this
 * address. If \a vm_area is specified, map pages in reserved vm area.
 */
/*****************************************************************************/ 
int
l4th_pages_allocate(l4_size_t size, l4_addr_t map_addr, l4_uint32_t vm_area,
		    l4_threadid_t owner, const char * name, l4_uint32_t flags, 
		    l4th_mem_desc_t * desc)
{
  int ret;
  l4_uint32_t attach_flags;

  /* allocate memory */
  if (flags & L4THREAD_CREATE_PINNED)
    ret = l4dm_mem_open(L4DM_DEFAULT_DSM,size,0,L4DM_PINNED,name,&desc->ds);
  else
    ret = l4dm_mem_open(L4DM_DEFAULT_DSM,size,0,0,name,&desc->ds);
  if (ret < 0)
    {
      ERROR("l4thread: memory allocation failed: %s (%d)!",
	    l4env_errstr(ret),ret);
      return ret;
    }
  desc->size = size;

#if DEBUG_MEM_ALLOC
  INFO("ds %d at "IdFmt"\n",desc->ds.id,IdStr(desc->ds.manager));
  DMSG("  flags 0x%08x, owner "IdFmt"\n",flags,IdStr(owner));
#endif

  /* attach dataspace */
  attach_flags = L4DM_RW;
  if (flags & L4THREAD_CREATE_MAP)
    attach_flags |= L4RM_MAP;

  if (map_addr == VM_FIND_REGION)
    {
      /* find suitable region */
      if (vm_area == VM_DEFAULT_AREA)
	{
	  if (flags & L4THREAD_CREATE_SETUP)
	    ret = l4rm_direct_attach(&desc->ds,size,0,attach_flags,
				     (void **)&desc->map_addr);
	  else
	    ret = l4rm_attach(&desc->ds,size,0,attach_flags,
			      (void **)&desc->map_addr);
	}
      else
	{
	  if (flags & L4THREAD_CREATE_SETUP)
	    ret = l4rm_direct_area_attach(&desc->ds,vm_area,size,0,
					  attach_flags,
					  (void **)&desc->map_addr);
	  else	    
	    ret = l4rm_area_attach(&desc->ds,vm_area,size,0,
				   attach_flags,(void **)&desc->map_addr);
	}
    }
  else
    {
      /* map to specified address */
      desc->map_addr = map_addr;
      if (vm_area == VM_DEFAULT_AREA)
	{
	  if (flags & L4THREAD_CREATE_SETUP)
	    ret = l4rm_direct_attach_to_region(&desc->ds,(void *)map_addr,
					       size,0,attach_flags);
	  else
	    ret = l4rm_attach_to_region(&desc->ds,(void *)map_addr,size,0,
					attach_flags);
	}
      else
	{
	  if (flags & L4THREAD_CREATE_SETUP)
	    ret = l4rm_direct_area_attach_to_region(&desc->ds,vm_area,
						    (void *)map_addr,size,0,
						    attach_flags);
	  else
	    ret = l4rm_area_attach_to_region(&desc->ds,vm_area,
					     (void *)map_addr,size,0,
					     attach_flags);
	}
    }

  if (ret)
    {
      ERROR("l4thread: attach dataspace failed: %s (%d)!",
	    l4env_errstr(ret),ret);

      l4dm_close(&desc->ds);
      return ret;
    }

#if DEBUG_MEM_ALLOC
  DMSG("  attached to region at 0x%08x\n",desc->map_addr);
#endif

  if (!l4_is_invalid_id(owner))
    {
      /* set dataspace owner */
      ret = l4dm_transfer(&desc->ds,owner);
      if (ret < 0)
	{
	  ERROR("l4thread: set dataspace owner failed: %s (%d)!",
		l4env_errstr(ret),ret);

	  l4rm_detach((void *)desc->map_addr);
	  l4dm_close(&desc->ds);
	  return ret;
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Detach and release pages.
 * 
 * \param  desc          Memory descriptor
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid memory descriptor
 *         - -#L4_EIPC   IPC error calling region mapper / dataspace manager
 *
 * Detach and release memory described by \a desc.
 */
/*****************************************************************************/ 
int 
l4th_pages_free(l4th_mem_desc_t * desc)
{
  int ret;
  
#if DEBUG_MEM_FREE
  INFO("free ds %d at "IdFmt"\n",desc->ds.id,IdStr(desc->ds.manager));
  DMSG("  mapped to 0x%08x, size %u\n",desc->map_addr,desc->size);
#endif

  /* detach vm region */
  ret = l4rm_detach((void *)desc->map_addr);

#if DEBUG_MEM_FREE
  INFO("detach done (ret %d)\n",ret);
#endif

  /* close dataspace */
  ret = l4dm_close(&desc->ds);

#if DEBUG_MEM_FREE
  INFO("close done (ret %d)\n",ret);
#endif

  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Map memory page
 * 
 * \param  desc          Memory descriptor
 * \param  offs          Page offset
 *	
 * \return 0 on success (page mapped), error code otherwise:
 *         - -#L4_EINVAL_OFFS invalid offset
 *         - -#L4_EIPC        IPC error calling dataspace manager
 *
 * Map memory page directly. It's required during startup where L4RM is not
 * yet running.
 */
/*****************************************************************************/ 
int
l4th_pages_map(l4th_mem_desc_t * desc, l4_offs_t offs)
{
  int ret;
  l4_addr_t fp_addr;
  l4_size_t fp_size;

  offs = l4_trunc_page(offs);
  if (offs >= desc->size)
    return -L4_EINVAL_OFFS;

  /* map page */
  ret = l4dm_map_pages(&desc->ds,offs,L4_PAGESIZE,desc->map_addr + offs,
		       L4_LOG2_PAGESIZE,0,L4DM_RW,&fp_addr,&fp_size);
  return ret;
}
