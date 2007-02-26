/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/lock.c
 * \brief  Memory dataspace manager client library, lock/unlock dataspaces
 *
 * \date   01/30/2002
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
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_mem/dm_mem-client.h>
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to lock dataspace region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__lock(l4dm_dataspace_t * ds, 
       l4_offs_t offset, 
       l4_size_t size)
{
  int ret;
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_mem_lock(ds->manager,ds->id,offset,size,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_mem: lock dataspace %u at %x.%x failed (ret %d, exc %d)!",
	    ds->id,ds->manager.id.task,ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;  
}

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to unlock dataspace region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__unlock(l4dm_dataspace_t * ds, 
	 l4_offs_t offset, 
	 l4_size_t size)
{
  int ret;
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_mem_unlock(ds->manager,ds->id,offset,size,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_mem: unlock dataspace %u at %x.%x failed (ret %d, exc %d)!",
	    ds->id,ds->manager.id.task,ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Walk through VM region to lock/unlock attached dataspaces
 * 
 * \param  addr          VM region start address
 * \param  size          VM region size
 * \param  lock          1 .. lock, 0 .. unlock atached dataspaces
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_ENOTFOUND  dataspace not found for a VM address
 *         - \c -L4_EIPC       error calling region mapper / dataspace manager
 *         - \c -L4_EPERM      caller is not a client of a dataspace attached
 *                             to the VM region
 */
/*****************************************************************************/ 
static int
__walk_vm(l4_addr_t addr, 
	  l4_size_t size, 
	  int lock)
{
  int ret,failed;
  l4_addr_t a,ds_map_addr;
  l4_size_t left,ds_map_size,map_remain;
  l4_offs_t ds_offs;
  l4dm_dataspace_t ds;
  
  a = addr;
  left = size;
  failed = 0;
  while ((left > 0) && !failed)
    {
#if DEBUG_LOCK
      INFO("addr 0x%08x, left 0x%08x\n",a,left);
#endif

      /* lookup addr */
      ret = l4rm_lookup((void *)a,&ds,&ds_offs,&ds_map_addr,&ds_map_size);
      if (ret < 0)
	{
	  ERROR("libdm_mem: lookup VM addr 0x%08x failed: %d!",addr,ret);
	  failed = ret;
	}
      
      if (!failed)
	{
	  /* lock / unlock dataspace region */
	  map_remain = ds_map_size - ds_offs;
	  if (map_remain > left)
	    map_remain = left;

#if DEBUG_LOCK
	  INFO("ds %u at %x.%x\n",ds.id,ds.manager.id.task,
	       ds.manager.id.lthread);
	  DMSG("  ds map area 0x%08x-0x%08x, offs 0x%08x\n",
	       ds_map_addr,ds_map_addr + ds_map_size,ds_offs);
	  DMSG("  %s request size 0x%08x\n",
	       (lock) ? "lock" : "unlock",map_remain);
#endif	  

	  if (lock)
	    ret = __lock(&ds,ds_offs,map_remain);
	  else
	    ret = __unlock(&ds,ds_offs,map_remain);
	  if (ret < 0)
	    {
#if DEBUG_ERRORS
	      DMSG("ds %u at %x.%x, offset 0x%08x, size 0x%08x\n",
		   ds.id,ds.manager.id.task,ds.manager.id.lthread,ds_offs,
		   map_remain);
	      ERROR("libdm_mem: %s failed: %d!",
		    (lock) ? "lock" : "unlock",ret);
#endif
	      if ((ret == -L4_EINVAL) || (ret == -L4_EINVAL_OFFS))
		/* invalid offset, this can result from weird VM layouts */
		failed = -L4_ENOTFOUND;
	      else
		failed = ret;
	    }
	  else
	    {
	      left -= map_remain;
	      a += map_remain;
	    }
	}
    }

  if (failed && lock)
    {
      /* unlock already locked dataspace regions */
      if ((size - left) > 0)
	__walk_vm(addr,size - left,0);
    }

  /* done */
  return failed;
}

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Lock dataspace region, IDL call wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_lock(l4dm_dataspace_t * ds, 
		 l4_offs_t offset, 
		 l4_size_t size)
{
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  return __lock(ds,offset,size);
}

/*****************************************************************************/
/**
 * \brief  Unlock dataspace region, IDL call wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_unlock(l4dm_dataspace_t * ds, 
		   l4_offs_t offset, 
		   l4_size_t size)
{
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  return __unlock(ds,offset,size);
}

/*****************************************************************************/
/**
 * \brief  Lock virtual memory region
 * 
 * \param  ptr           VM region start address
 * \param  size          VM region size
 *	
 * \return 0 on success (locked VM region), error code otherwise:
 *         - \c -L4_ENOTFOUND  dataspace not found for a VM address
 *         - \c -L4_EIPC       error calling region mapper / dataspace manager
 *         - \c -L4_EPERM      caller is not a client of a dataspace attached
 *                             to the VM region
 */
/*****************************************************************************/ 
int
l4dm_mem_lock(void * ptr, 
	      l4_size_t size)
{
  /* lock VM region */
  return __walk_vm((l4_addr_t)ptr,size,1);
}

/*****************************************************************************/
/**
 * \brief  Unlock virtual memory region
 * 
 * \param  ptr           VM region start address
 * \param  size          VM region size
 *	
 * \return 0 on success (unlocked VM region), error code otherwise:
 *         - \c -L4_ENOTFOUND  dataspace not found for a VM address
 *         - \c -L4_EIPC       error calling region mapper / dataspace manager
 *         - \c -L4_EPERM      caller is not a client of a dataspace attached
 *                             to the VM region
 */
/*****************************************************************************/ 
int
l4dm_mem_unlock(void * ptr, 
		l4_size_t size)
{
  /* lock VM region */
  return __walk_vm((l4_addr_t)ptr,size,0);
}
