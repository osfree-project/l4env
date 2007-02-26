/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/allocate.c
 * \brief  Generic dataspace manager client library, memory allocation 
 *         convenience functions
 *
 * \date   01/31/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
 *                       - #L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                             memory
 *                       - #L4DM_PINNED        allocate pinned ("locked")
 *                                             memory
 *                       - #L4RM_MAP           map memory immediately
 *                       - #L4RM_LOG2_ALIGNED  find a 2^(log2(size) + 1) 
 *                                             aligned VM region 
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             2^(log2(size) + 1) sized 
 *                                             VM region
 * \param  name          Dataspace name
 * \param  dsm_id        Dataspace manager id
 * \retval ds            Dataspace id                         
 *	
 * \return Pointer to attached dataspace, NULL if allocation / attach failed.
 */
/*****************************************************************************/ 
static void *
__allocate(l4_size_t size, l4_uint32_t flags, const char * name, 
	   l4_threadid_t dsm_id, l4dm_dataspace_t * ds)
{
  int ret;
  void * ptr;
  
  /* get dataspace manager */
  if (l4_is_invalid_id(dsm_id))
    dsm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dsm_id))
    {
      LOGdL(DEBUG_ERRORS, "libdm_mem: no dataspace manager found!");
      return NULL;
    }

  /* round size to pagesize */
  size = l4_round_page(size);

  LOGdL(DEBUG_ALLOCATE, "allocate 0x%x at "l4util_idfmt,
        (unsigned)size, l4util_idstr(dsm_id));

  /* allocate memory */
  ret = l4dm_mem_open(dsm_id, size, DMMEM_ALLOCATE_ALIGN, flags, name, ds);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_mem: dataspace allocation at "l4util_idfmt" failed: %d!",
	    l4util_idstr(dsm_id), ret);
      return NULL;
    }

  LOGdL(DEBUG_ALLOCATE, "got ds %u at "l4util_idfmt,
        ds->id, l4util_idstr(ds->manager));

  /* attach dataspace, set access rights to L4DM_RW */
  ret = l4rm_attach(ds, size, 0, flags | L4DM_RW, &ptr);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, "libdm_mem: attach dataspace failed: %d!", ret);
      l4dm_close(ds);
      return NULL;
    }

  LOGdL(DEBUG_ALLOCATE, "attached to 0x"l4_addr_fmt, (l4_addr_t)ptr);

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
__release(const void * ptr)
{
  int ret;
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t ds_map_size;
  l4_threadid_t dummy;

  LOGdL(DEBUG_RELEASE, "free at 0x"l4_addr_fmt, (l4_addr_t)ptr);

  /* lookup dataspace */
  ret = l4rm_lookup(ptr, &ds_map_addr, &ds_map_size, &ds, &ds_offs, &dummy);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, "libdm_mem: no dataspace attached at 0x"l4_addr_fmt
	    " (%d)!", (l4_addr_t)ptr, ret);
      return;
    }

  if (ret != L4RM_REGION_DATASPACE)
    {
      LOGdL(DEBUG_ERRORS, "trying to free non-dataspace " \
            "region at addr 0x"l4_addr_fmt" (type %d)", (l4_addr_t)ptr, ret);
      return;
    }

  LOGdL(DEBUG_RELEASE,"ds %u at "l4util_idfmt, ds.id, l4util_idstr(ds.manager));

  /* detach dataspace */
  ret = l4rm_detach(ptr);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, "libdm_mem: detach failed: %d!", ret);
      return;
    }

  /* close dataspace */
  ret = l4dm_close(&ds);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, "libdm_mem: close dataspace %u at "l4util_idfmt \
            "failed: %d!", ds.id, l4util_idstr(ds.manager), ret);
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
l4dm_mem_allocate(l4_size_t size, l4_uint32_t flags)
{
  l4dm_dataspace_t ds;

  /* allocate */
  return __allocate(size, flags, "", L4_INVALID_ID, &ds);
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
l4dm_mem_allocate_named(l4_size_t size, l4_uint32_t flags, const char * name)
{
  l4dm_dataspace_t ds;

  /* allocate */
  return __allocate(size, flags, name, L4_INVALID_ID, &ds);
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
l4dm_mem_ds_allocate(l4_size_t size, l4_uint32_t flags, 
		     l4dm_dataspace_t * ds)
{
  /* allocate */
  return __allocate(size, flags, "", L4_INVALID_ID, ds);
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
l4dm_mem_ds_allocate_named(l4_size_t size, l4_uint32_t flags, const char * name,
			   l4dm_dataspace_t * ds)
{
  /* allocate */
  return __allocate(size, flags, name, L4_INVALID_ID, ds);
}
  
/*****************************************************************************/
/**
 * \brief  Allocate memory, name dataspace, from specific dataspace manager
 * 
 * \param  size          Memory size
 * \param  flags         Flags
 * \param  name          Dataspace name
 * \param  dsm_id        Dataspace manager id
 * \retval ds            Dataspace descriptor
 *	
 * \return Pointer to allocated memory, NULL if allocation failed.
 *
 * Allocate memory at the given dataspace manager and attach to the
 * address space. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_ds_allocate_named_dsm(l4_size_t size, l4_uint32_t flags, 
                               const char * name, l4_threadid_t dsm_id, 
			       l4dm_dataspace_t * ds)
{
  /* allocate */
  return __allocate(size, flags, name, dsm_id, ds);
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
l4dm_mem_release(const void * ptr)
{
  /* release */
  __release(ptr);
}
