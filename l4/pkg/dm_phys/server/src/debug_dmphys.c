/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/debug_dmphys.c
 * \brief  DMphys, internal debug functions
 *
 * \date   02/03/2002
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
#include <l4/slab/slab.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include <l4/dm_phys/consts.h>
#include "__memmap.h"
#include "__pages.h"
#include "__dataspace.h"

/*****************************************************************************
 *** some external data structures
 *****************************************************************************/

/**
 * memmap descriptor slab cache
 */
extern l4slab_cache_t memmap_cache;

/**
 * Page area descriptor slab cache
 */
extern l4slab_cache_t area_cache;

/**
 * Dataspace descriptor slab cache 
 */
extern l4slab_cache_t dataspace_cache;

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Show debug information
 * 
 * \param  request       Flick request structure
 * \param  key           Debug key (see include/l4/dm_phys/consts.h)
 * \param  data          Debug data
 * \retval _ev           Flick exception structure, unused
 */
/*****************************************************************************/ 
void 
if_l4dm_memphys_dmphys_debug_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t key,
    l4_uint32_t data,
    CORBA_Environment *_dice_corba_env)
{
  page_pool_t * pool;

  /* show debug information */
  switch (key)
    {
    case L4DM_MEMPHYS_SHOW_MEMMAP:
      /* show memory map */
      dmphys_memmap_show();
      break;

    case L4DM_MEMPHYS_SHOW_POOLS:
      /* show all used memory pools */
      dmphys_pages_dump_used_pools();
      break;

    case L4DM_MEMPHYS_SHOW_POOL_AREAS:
      /* show memory areas of page pool */
      pool = dmphys_get_page_pool(data);
      if (pool != NULL)
	dmphys_pages_dump_areas(pool);
      else
	Error("DMphys: invalid page pool %u",data);
      break;

    case L4DM_MEMPHYS_SHOW_POOL_FREE:
      /* show free lists of a page pool */
      pool = dmphys_get_page_pool(data);
      if (pool != NULL)
	dmphys_pages_dump_free(pool);
      else
	Error("DMphys: invalid page pool %u",data);
      break;

    case L4DM_MEMPHYS_SHOW_SLABS:
      /* show descriptor slab cache information */
      printf("Memmap descriptor slab cache:\n");
      l4slab_dump_cache_free(&memmap_cache);
      printf("\n");
      printf("Page area slab cache:\n");
      l4slab_dump_cache_free(&area_cache);
      printf("\n");
      printf("Dataspace descriptor slab cache:\n");
      l4slab_dump_cache_free(&dataspace_cache);
      break;

    default:
      Error("DMphys: invalid debug key: 0x%08x",key);
    }
}

