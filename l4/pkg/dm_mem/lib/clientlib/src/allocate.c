/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/allocate.c
 * \brief  Generic dataspace manager client library, memory allocation 
 *         convenience functions
 *
 * \date   01/31/2002
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
#include <l4/sys/consts.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate and attach dataspace
 * 
 * \param  size          Dataspace size
 * \param  flags         Flags:
 *                       - \c L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                               memory
 *                       - \c L4DM_PINNED        allocate pinned ("locked")
 *                                               memory
 *                       - \c L4RM_MAP           map memory immediately
 *                       - \c L4RM_LOG2_ALIGNED  find a 2^(log2(size) + 1) 
 *                                               aligned VM region 
 *                       - \c L4RM_LOG2_ALLOC    allocate the whole 
 *                                               2^(log2(size) + 1) sized 
 *                                               VM region
 * \param  name          Dataspace name
 * \retval ds            Dataspace id                         
 *	
 * \return Pointer to attached dataspace, NULL if allocation / attach failed.
 */
/*****************************************************************************/ 
static void *
__allocate(l4_size_t size, 
	   l4_uint32_t flags, 
	   const char * name, 
	   l4dm_dataspace_t * ds)
{
  l4_threadid_t dsm_id;
  int ret;
  void * ptr;
  
  /* get dataspace descriptor */
  dsm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dsm_id))
    {
      ERROR("libdm_mem: no dataspace manager found!");
      return NULL;
    }

  /* round size to pagesize */
  size = l4_round_page(size);

#if DEBUG_ALLOCATE
  INFO("allocate 0x%x at %x.%x\n",size,dsm_id.id.task,dsm_id.id.lthread);
#endif

  /* allocate memory */
  ret = l4dm_mem_open(dsm_id,size,DMMEM_ALLOCATE_ALIGN,flags,name,ds);
  if (ret < 0)
    {
      ERROR("libdm_mem: dataspace allocation at %x.%x failed: %d!",
	    dsm_id.id.task,dsm_id.id.lthread,ret);
      return NULL;
    }

#if DEBUG_ALLOCATE
  INFO("got ds %u at %x.%x\n",ds->id,ds->manager.id.task,
       ds->manager.id.lthread);
#endif

  /* attach dataspace, set access rights to L4DM_RW */
  ret = l4rm_attach(ds,size,0,flags | L4DM_RW,&ptr);
  if (ret < 0)
    {
      ERROR("libdm_mem: attach dataspace failed: %d!",ret);
      l4dm_close(ds);
      return NULL;
    }

#if DEBUG_ALLOCATE
  INFO("attached to 0x%08x\n",(l4_addr_t)ptr);
#endif

  /* done */
  return ptr;
}

/*****************************************************************************/
/**
 * \brief  Detach and release dataspace
 * 
 * \param  ptr           Memory address
 */
/*****************************************************************************/ 
static void
__release(void * ptr)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t ds_map_size;

#if DEBUG_RELEASE
  INFO("free at 0x%08x\n",(l4_addr_t)ptr);
#endif

  /* lookup dataspace */
  ret = l4rm_lookup(ptr,&ds,&ds_offs,&ds_map_addr,&ds_map_size);
  if (ret < 0)
    {
      ERROR("libdm_mem: no dataspace attached at 0x%08x (%d)!",
	    (l4_addr_t)ptr,ret);
      return;
    }

#if DEBUG_RELEASE
  INFO("ds %u at %x.%x\n",ds.id,ds.manager.id.task,
       ds.manager.id.lthread);
#endif

  /* detach dataspace */
  ret = l4rm_detach(ptr);
  if (ret < 0)
    {
      ERROR("libdm_mem: detach failed: %d!",ret);
      return;
    }

  /* close dataspace */
  ret = l4dm_close(&ds);
  if (ret < 0)
    {
      ERROR("libdm_mem: close dataspace %u at %x.%x failed: %d!",
	    ds.id,ds.manager.id.task,ds.manager.id.lthread,ret);
      return;
    }

  /* done */
}

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate memory
 * 
 * \param  size          Memory size
 * \param  flags         Flags
 *	
 * \return Pointer to allocated memory, NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager and attach to the
 * address space. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_allocate(l4_size_t size, 
		  l4_uint32_t flags)
{
  l4dm_dataspace_t ds;

  /* allocate */
  return __allocate(size,flags,"",&ds);
}

/*****************************************************************************/
/**
 * \brief  Allocate memory, name dataspace
 * 
 * \param  size          Memory size
 * \param  flags         Flags
 * \param  name          Dataspace name
 *	
 * \return Pointer to allocated memory, NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager and attach to the
 * address space. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_allocate_named(l4_size_t size, 
			l4_uint32_t flags, 
			const char * name)
{
  l4dm_dataspace_t ds;

  /* allocate */
  return __allocate(size,flags,name,&ds);
}
  
/*****************************************************************************/
/**
 * \brief  Allocate memory
 * 
 * \param  size          Memory size
 * \param  flags         Flags
 * \retval ds            Dataspace descriptor
 *	
 * \return Pointer to allocated memory, NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager and attach to the
 * address space. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_ds_allocate(l4_size_t size, 
		     l4_uint32_t flags, 
		     l4dm_dataspace_t * ds)
{
  /* allocate */
  return __allocate(size,flags,"",ds);
}

/*****************************************************************************/
/**
 * \brief  Allocate memory, name dataspace
 * 
 * \param  size          Memory size
 * \param  flags         Flags
 * \param  name          Dataspace name
 * \retval ds            Dataspace descriptor
 *	
 * \return Pointer to allocated memory, NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager and attach to the
 * address space. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_ds_allocate_named(l4_size_t size, 
			   l4_uint32_t flags, 
			   const char * name,
			   l4dm_dataspace_t * ds)
{
  /* allocate */
  return __allocate(size,flags,name,ds);
}
  
/*****************************************************************************/
/**
 * \brief  Release memory
 * 
 * \param  ptr           Memory area address
 *
 * Release memory attached to \a ptr.
 */
/*****************************************************************************/ 
void
l4dm_mem_release(void * ptr)
{
  /* release */
  __release(ptr);
}
