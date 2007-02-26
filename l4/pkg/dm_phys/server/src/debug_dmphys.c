/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/debug_dmphys.c
 * \brief  DMphys, internal debug functions
 *
 * \date   02/03/2002
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
if_l4dm_memphys_server_dmphys_debug(sm_request_t * request, 
				    l4_uint32_t key, 
				    l4_uint32_t data, 
				    sm_exc_t * _ev)
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
      Msg("Memmap descriptor slab cache:\n");
      l4slab_dump_cache_free(&memmap_cache);
      Msg("\n");
      Msg("Page area slab cache:\n");
      l4slab_dump_cache_free(&area_cache);
      Msg("\n");
      Msg("Dataspace descriptor slab cache:\n");
      l4slab_dump_cache_free(&dataspace_cache);
      break;

    default:
      Error("DMphys: invalid debug key: 0x%08x",key);
    }
}

