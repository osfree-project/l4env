/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/open.c
 * \brief  DMphys, open new dataspace
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
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* DMphys/private include */
#include <l4/dm_phys/consts.h>
#include "dm_phys-server.h"
#include "__dataspace.h"
#include "__pages.h"
#include "__internal_alloc.h"
#include "__dm_phys.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create new dataspace
 * 
 * \param  owner         Dataspace owner
 * \param  pool          Page pool descriptor
 * \param  addr          Memory area start address 
 *                       (#L4DM_MEMPHYS_ANY_ADDR ... find suitable area)
 * \param  size          Memory area size
 * \param  align         Alignment
 * \param  flags         Flags:
 *                       - #L4DM_CONTIGUOUS allocate contiguous area, default
 *                                          is to assemble pages from smaller
 *                                          areas
 * \param  name          Dataspace name
 * \retval ds            Dataspace descriptor
 *	
 * \return 0 on success (\a ds contains a valid dataspace descriptor), 
 *         error code otherwise:
 *         - -#L4_ENOHANDLE  could not create dataspace descriptor
 *         - -#L4_ENOMEM     no memory available
 */
/*****************************************************************************/ 
static int
__create_ds(l4_threadid_t owner, page_pool_t * pool, l4_addr_t addr, 
	    l4_size_t size, l4_addr_t align, l4_uint32_t flags, 
	    const char * name, l4dm_dataspace_t * ds)
{
  dmphys_dataspace_t * desc;
  page_area_t * pages;
  int ret;

  /* round size to multiple of pagesize */
  size = (size + DMPHYS_PAGESIZE - 1) & ~(DMPHYS_PAGESIZE - 1);

  /* set alignment to 2^n */
  if (align < DMPHYS_PAGESIZE)
    align = DMPHYS_PAGESIZE;
  else
    align = 1UL << (l4util_bsr(align));

  LOGdL(DEBUG_OPEN, "size %u, pool %u, alignment 0x%08x, flags 0x%08x",
        size, pool->pool, align, flags);

  /* create dataspace descriptor */
  desc = dmphys_ds_create(owner, name, flags);
  if (desc == NULL)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: dataspace descriptor allocation failed!");
      return -L4_ENOHANDLE;
    }

  /* allocate memory */
  if (addr == L4DM_MEMPHYS_ANY_ADDR)
    ret = dmphys_pages_allocate(pool, size, align, flags, PAGES_USER, &pages);
  else
    ret = dmphys_pages_allocate_area(pool, addr, size, PAGES_USER, &pages);
  if (ret < 0)
    {
      /* allocation failed */
      LOGdL(DEBUG_ERRORS, "DMphys: memory allocation failed!");
      dmphys_ds_release(desc);
      return -L4_ENOMEM;
    }

  /* add pages to dataspace descriptor, this will also set the size of the 
   * dataspace which is calculated from the page area list. This size might 
   * differ from the requested size, e.g. if the L4DM_MEMPHYS_SUPERPAGES flag
   * is set and dmphys_pages_allocate therefore aligned the size to a
   * superpage */
  dmphys_ds_add_pages(desc, pages, pool);

#if DEBUG_OPEN
  LOGL("id %u, memory areas:", dmphys_ds_get_id(desc));
  dmphys_pages_list(pages);
#endif

  /* setup dataspace id */
  ds->id = dmphys_ds_get_id(desc);
  ds->manager = dmphys_service_id;

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
 * \brief  Open new dataspace
 * 
 * \param  owner         Dataspace owner
 * \param  pool          Page pool descriptor
 * \param  addr          Memory area start address 
 *                       (#L4DM_MEMPHYS_ANY_ADDR ... find suitable area)
 * \param  size          Memory area size
 * \param  align         Alignment
 * \param  flags         Flags:
 *                       - #L4DM_CONTIGUOUS allocate contiguous area, default
 *                                          is to assemble pages from smaller
 *                                          areas
 * \param  name          Dataspace name
 * \retval ds            Dataspace descriptor
 *	
 * \return 0 on success (\a ds contains a valid dataspace descriptor), 
 *         error code otherwise:
 *         - -#L4_ENOHANDLE  could not create dataspace descriptor
 *         - -#L4_ENOMEM     no memory available
 *
 * Internal version, used. e.g. in copy.c to create a copy of a dataspace.
 */
/*****************************************************************************/ 
int
dmphys_open(l4_threadid_t owner, page_pool_t * pool, l4_addr_t addr, 
	    l4_size_t size, l4_addr_t align, l4_uint32_t flags, 
	    const char * name, l4dm_dataspace_t * ds)
{
  /* create dataspace */
  return __create_ds(owner, pool, addr, size, align, flags, name, ds);
}

/*****************************************************************************
 *** DMphys IDL  server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open new dataspace (generic memory dataspace manager version)
 * 
 * \param  _dice_corba_obj    Request source
 * \param  size               Dataspace size
 * \param  align              Alignment
 * \param  flags              Flags
 *                            - #L4DM_CONTIGUOUS allocate contiguous memory area
 * \param  name               Dataspace name
 * \param  _dice_corba_env    Server environment
 * \retval ds                 Dataspace id
 *	
 * \return 0 on success (\a ds contains a valid dataspace descriptor),
 *         error code otherwise:
 *         - -#L4_ENOHANDLE  no dataspace descriptor available
 *         - -#L4_ENOMEM     no memory available
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_mem_open_component(CORBA_Object _dice_corba_obj, l4_uint32_t size,
                           l4_uint32_t align, l4_uint32_t flags,
                           const char* name, l4dm_dataspace_t *ds,
                           CORBA_Server_Environment *_dice_corba_env)
{
  page_pool_t * p = dmphys_get_default_pool();

#if DEBUG_OPEN
  LOGL("owner "l4util_idfmt, l4util_idstr(*_dice_corba_obj));
  if (name != NULL)
    LOG_printf(" name \'%s\', size %u, align 0x%08x, flags 0x%08x\n",
           name, size, align, flags);
  else
    LOG_printf(" size %u, align 0x%08x, flags 0x%08x\n", size, align, flags);
#endif

  /* create dataspace */
  return __create_ds(*_dice_corba_obj, p, L4DM_MEMPHYS_ANY_ADDR,
		     size, align, flags, name, ds);
}

/*****************************************************************************/
/**
 * \brief Open new dataspace (extended DMphys version)
 * 
 * \param  _dice_corba_obj    Request source
 * \param  pool               Memory pool number
 * \param  addr               Memory area start address
 *                            (#L4DM_MEMPHYS_ANY_ADDR ... find suitable area)
 * \param  size               Memory area size
 * \param  align              Memory area alignment
 * \param  flags              Flags
 *                            - #L4DM_CONTIGUOUS allocate contiguous memory area
 * \param  name               Dataspace name
 * \param  _dice_corba_env    Server environment
 * \retval ds                 Dataspace id
 *	
 * \return 0 on success (\a ds contains a valid dataspace descriptor),
 *         error code otherwise:
 *         - -#L4_EINVAL     invalid page pool number
 *         - -#L4_ENOHANDLE  no dataspace descriptor available
 *         - -#L4_ENOMEM     no memory available
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_dmphys_open_component(CORBA_Object _dice_corba_obj,
                                      l4_uint32_t pool, l4_uint32_t addr,
                                      l4_uint32_t size, l4_uint32_t align,
                                      l4_uint32_t flags, const char* name,
                                      l4dm_dataspace_t *ds,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  page_pool_t * p = dmphys_get_page_pool(pool);

  /* sanity checks */
  if (p == NULL)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: invalid page pool (%d)", pool);
      return -L4_EINVAL;
    }

  /* create dataspace */
  return __create_ds(*_dice_corba_obj, p, addr, size, align, flags, name, ds);
}

