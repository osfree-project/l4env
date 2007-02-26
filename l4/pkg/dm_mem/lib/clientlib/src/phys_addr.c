/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/phys_addr.c
 * \brief  Memory dataspace manager client library, get phys. address
 *
 * \date   01/29/2002
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
 * \brief  Call dataspace manager to request the phys. address of a dataspace 
 *         region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 * \retval paddr         Phys. address
 * \retval psize         Size of phys. contiguous dataspace region at offset
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__get_phys_addr(l4dm_dataspace_t * ds, 
		l4_offs_t offset, 
		l4_size_t size,
		l4_addr_t * paddr, 
		l4_size_t * psize)
{
  int ret;
  sm_exc_t _exc;

  /* call dataspace manager */
  ret = if_l4dm_mem_phys_addr(ds->manager,ds->id,offset,size,paddr,psize,
			      &_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_mem: get address of dataspace %u at %x.%x failed "
	    "(ret %d, exc %d)!",ds->id,ds->manager.id.task,
	    ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Get phys. address of dataspace region, IDL call wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size, \c L4DM_WHOLE_DS to get the size of the
 *                       contiguous area at offset
 * \retval paddr         Phys. address
 * \retval psize         Size of phys. contiguous dataspace region at offset
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_phys_addr(l4dm_dataspace_t * ds, 
		      l4_offs_t offset, 
		      l4_size_t size,
		      l4_addr_t * paddr, 
		      l4_size_t * psize)
{
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  return __get_phys_addr(ds,offset,size,paddr,psize);
}

/*****************************************************************************/
/**
 * \brief  Get phys. address of a of VM region
 * 
 * \param  ptr           VM region address
 * \param  size          VM region size
 * \param  addrs         Phys. address regions destination buffer
 * \param  num           Number of elements in \a addrs.
 * \retval psize         Total size of the address regions in \a addrs
 *	
 * \return On success number of found phys. address regions, 
 *         error code otherwise:
 *         - \c -L4_ENOTFOUND  dataspace not found for a VM address
 *         - \c -L4_EPERM      caller is not a client of the dataspace
 *         - \c -L4_EIPC       error calling region mapper / dataspace manager
 *         
 * Lookup the phys. addresses for the specified virtual memory region. 
 */
/*****************************************************************************/ 
int
l4dm_mem_phys_addr(void * ptr, 
		   l4_size_t size, 
		   l4dm_mem_addr_t addrs[], 
		   int num, 
		   l4_size_t * psize)
{
  int ret,i;
  l4_addr_t addr = (l4_addr_t)ptr; 
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t left,ds_map_size,map_remain;
  
  if (size <= 0)
    /* nothing to do */
    return 0;

  /* lookup phys. addresses for VM region */
  left = size;
  i = 0;
  while ((left > 0) && (i < num))
    {
#if DEBUG_PHYS_ADDR
      INFO("\n");
      DMSG("  addr 0x%08x, left 0x%08x\n",addr,left);
#endif

      /* lookup addr */
      ret = l4rm_lookup((void *)addr,&ds,&ds_offs,&ds_map_addr,&ds_map_size);
      if (ret < 0)
	{
	  ERROR("libdm_mem: lookup VM addr 0x%08x failed: %d!",addr,ret);
	  return ret;
	}
      map_remain = ds_map_size - ds_offs;
      if (map_remain > left)
	map_remain = left;

#if DEBUG_PHYS_ADDR
      INFO("ds %u at %x.%x\n",ds.id,ds.manager.id.task,ds.manager.id.lthread);
      DMSG("  ds map area 0x%08x-0x%08x, offs 0x%08x\n",
	   ds_map_addr,ds_map_addr + ds_map_size,ds_offs);
      DMSG("  request region size 0x%08x\n",map_remain);
#endif

      /* get phys. addr */
      ret = __get_phys_addr(&ds,ds_offs,map_remain,
			    &addrs[i].addr,&addrs[i].size);
      if (ret < 0)
	{
#if DEBUG_ERRORS
	  DMSG("ds %u at %x.%x, offset 0x%08x, size 0x%08x\n",
	       ds.id,ds.manager.id.task,ds.manager.id.lthread,ds_offs,
	       map_remain);
	  ERROR("get phys. address failed: %d!",ret);
#endif
	  if ((ret == -L4_EINVAL) || (ret == -L4_EINVAL_OFFS))
	    /* invalid offset, this can result from weird VM layouts */
	    return -L4_ENOTFOUND;
	  else
	    return ret;
	}
      
#if DEBUG_PHYS_ADDR
      INFO("addr index %d:\n",i);
      DMSG("  addr 0x%08x, size 0x%08x\n",addrs[i].addr,addrs[i].size);
#endif

      left -= addrs[i].size;
      addr += addrs[i].size;
      i++;
    }

  /* set phys. region size */
  *psize = size - left;

  /* done */
  return i;
}

/*****************************************************************************/
/**
 * \brief  Test if dataspace is allocated on contiguous memory
 * 
 * \param  ds            Dataspace id
 *	
 * \return 1 if dataspace is allocated on contiguous memory, 0 if not.
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_is_contiguous(l4dm_dataspace_t * ds)
{
  int ret,is_cont;
  sm_exc_t _exc;

  if (ds == NULL)
    return 0;

  /* call dataspace manager */
  ret = if_l4dm_mem_is_contiguous(ds->manager,ds->id,&is_cont,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_mem: check dataspace %u at %x.%x failed "
	    "(ret %d, exc %d)!",ds->id,ds->manager.id.task,
	    ds->manager.id.lthread,ret,_exc._type);
      return 0;
    }
  
  /* done */
  return is_cont;
}
