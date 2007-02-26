/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/debug_dmphys.c
 * \brief  DMphys client library, show debug information
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
#include <l4/util/macros.h>

/* DMphys includes */
#include <l4/dm_phys/consts.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_phys/dm_phys-client.h>
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Call dataspace manager
 * 
 * \param  key           Debug key
 * \param  data          Debug data
 */
/*****************************************************************************/ 
static void
__debug(l4_uint32_t key, l4_uint32_t data)
{
  l4_threadid_t dsm_id;
  CORBA_Environment _env = dice_default_environment;

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return;

  /* call DMphys */
  if_l4dm_memphys_dmphys_debug_call(&(dsm_id), key, data, &_env);
  if (_env.major != CORBA_NO_EXCEPTION)
    LOG_Error("libdm_phys: IPC erroc calling DMphys (exc %d)!", _env.major);
}

/*****************************************************************************
 *** DMphys client lib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  DEBUG: show DMphys memory map 
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_memmap(void)
{
  /* show memmap */
  __debug(L4DM_MEMPHYS_SHOW_MEMMAP, 0);
}

/*****************************************************************************/
/**
 * \brief  DEBUG: show DMphys memory pools
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pools(void)
{
  /* show pools */
  __debug(L4DM_MEMPHYS_SHOW_POOLS, 0);
}

/*****************************************************************************/
/**
 * \brief  DEBUG: show memory areas of a memory pool
 * 
 * \param  pool          Memory pool number
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pool_areas(int pool)
{
  /* show pool areas */
  __debug(L4DM_MEMPHYS_SHOW_POOL_AREAS, pool);
}

/*****************************************************************************/
/**
 * \brief  DEBUG: show free lists of a memory pool
 * 
 * \param  pool          Memory pool number
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pool_free(int pool)
{
  /* show pool free lists */
  __debug(L4DM_MEMPHYS_SHOW_POOL_FREE, pool);
}

/*****************************************************************************/
/**
 * \brief  DEBUG: show descriptor slab cache information
 *
 * \param  show_free     Show slab cache free lists
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_slabs(int show_free)
{
  /* show descriptor slabs */
  __debug(L4DM_MEMPHYS_SHOW_SLABS, show_free);
}
