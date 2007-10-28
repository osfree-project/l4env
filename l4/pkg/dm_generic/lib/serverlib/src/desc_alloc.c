/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/src/desc_alloc.c
 * \brief  Datapace manager library, dataspace / client descriptor allocation
 *
 * \date   11/21/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/slab/slab.h>
#include <l4/util/macros.h>
#include <l4/util/l4_macros.h>

/* DM_generic includes */
#include <l4/dm_generic/dsmlib.h>
#include "__desc_alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Dataspace descriptor slab cache
 */
static l4slab_cache_t ds_desc_cache;

/**
 * Client descriptor slab cache
 */
static l4slab_cache_t client_desc_cache;

/**
 * Page allocation callback function
 */
static dsmlib_get_page_fn_t get_page = NULL;

/**
 * Page release callback function
 */
static dsmlib_free_page_fn_t free_page = NULL;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Slab cache grow callback
 *
 * \param  cache         Slab cache descriptor
 * \param  data          Page data pointer
 *
 * \return pointer to allocated page
 */
/*****************************************************************************/
static void *
__grow(l4slab_cache_t * cache, void ** data)
{
  void * page;

  page = get_page(data);

  LOGdL(DEBUG_PAGE_ALLOC, "got page at 0x"l4_addr_fmt, (l4_addr_t)page);

  return page;
}

/*****************************************************************************/
/**
 * \brief Slab cache release callback
 *
 * \param  cache         Slab cache descriptor
 * \param  page          Page address
 * \param  data          Page data pointer
 */
/*****************************************************************************/
static void
__release(l4slab_cache_t * cache, void * page, void * data)
{
  LOGdL(DEBUG_PAGE_ALLOC, "release page at 0x"l4_addr_fmt, (l4_addr_t)page);

  free_page(page,data);
}

/*****************************************************************************
 *** DSMlib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Initialize descriptor allocation
 *
 * \param  get_page_fn   Page allocation callback function
 * \param  free_page_fn  Page release callback function
 *
 * \return 0 on succes, -1 if initialization failed.
 */
/*****************************************************************************/
int
dsmlib_init_desc_alloc(dsmlib_get_page_fn_t get_page_fn,
                       dsmlib_free_page_fn_t free_page_fn)
{
  /* we need at least a page allocation function */
  if (get_page_fn == NULL)
    return -1;

  get_page = get_page_fn;
  free_page = free_page_fn;

  /* initialize slab caches */
  if (l4slab_cache_init(&ds_desc_cache, sizeof(dsmlib_ds_desc_t), 0,
                        __grow, __release) < 0)
    {
      LOGdL(DEBUG_ERRORS, "DSMlib: descriptor slab cache init failed!");
      return -1;
    }

  if (l4slab_cache_init(&client_desc_cache, sizeof(dsmlib_client_desc_t),
                        0, __grow, __release) < 0)
    {
      LOGdL(DEBUG_ERRORS, "DSMlib: descriptor slab cache init failed!");
      l4slab_destroy(&ds_desc_cache);
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Allocate dataspace descriptor
 *
 * \return Pointer to new dataspace descriptor, NULL if allocation failed.
 */
/*****************************************************************************/
dsmlib_ds_desc_t *
dsmlib_alloc_ds_desc(void)
{
  return l4slab_alloc(&ds_desc_cache);
}

/*****************************************************************************/
/**
 * \brief Release dataspace descriptor
 *
 * \param  desc          Pointer to dataspace descriptor
 */
/*****************************************************************************/
void
dsmlib_free_ds_desc(dsmlib_ds_desc_t * desc)
{
  l4slab_free(&ds_desc_cache,desc);
}

/*****************************************************************************/
/**
 * \brief Allocate client descriptor
 *
 * \return Pointer to new client descriptor, NULL if allocation failed
 */
/*****************************************************************************/
dsmlib_client_desc_t *
dsmlib_alloc_client_desc(void)
{
  return l4slab_alloc(&client_desc_cache);
}

/*****************************************************************************/
/**
 * \brief Release client descriptor
 *
 * \param  desc          Pointer to client descriptor
 */
/*****************************************************************************/
void
dsmlib_free_client_desc(dsmlib_client_desc_t * desc)
{
  l4slab_free(&client_desc_cache, desc);
}
