/* $Id$ */
/*****************************************************************************/
/**
 * \file    slab/include/slab.h
 * \brief   Slab allocator API.
 *
 * \date    07/27/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#ifndef _L4SLAB_SLAB_H
#define _L4SLAB_SLAB_H

/* L4env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Slab descriptor type 
 */
typedef struct l4slab_slab l4slab_slab_t;

/**
 * \brief   Slab cache descriptor type 
 * \ingroup api_init
 */
typedef struct l4slab_cache l4slab_cache_t;

/**
 * \brief   Cache grow callback function 
 * \ingroup api_init
 * 
 * \param   cache        Descriptor of the slab cache which requests 
 *                       the memory
 * \retval  data         Page user data pointer, the contents is returned 
 *                       with the page to the release callback function.
 * 
 * \return  Pointer to the allocated page.
 *
 * This function is called by a slab cache to allocate a new page for the 
 * cache. It must return a pointer to a memory area with the size of one page 
 * (L4_PAGESIZE) and which is alligned to a page boundary.
 */
typedef void * (* l4slab_grow_fn_t) (l4slab_cache_t * cache, 
				     void ** data);

/**
 * \brief   Cache release callback function
 * \ingroup api_init
 *
 * \param   cache        Slab cache descriptor
 * \param   page         Page address
 * \param   data         Page user data pointer
 * 
 * Cache release callback function. It is called by a slab cache to release 
 * pages which are no longer needed by the cache. 
 */
typedef void (* l4slab_release_fn_t) (l4slab_cache_t * cache, 
				      void * page, void * data);

/**
 * Slab cache descriptor 
 */
struct l4slab_cache
{
  l4_size_t            obj_size;   ///< size of cache objects
  int                  num_objs;   ///< number of objects per slab
  int                  num_slabs;  ///< number of slabs in cache
  int                  num_free;   ///< number of completely free slabs
  unsigned int         max_free;   ///< max. allowed free slabs

  l4slab_slab_t *      slabs;      ///< slab list
  l4slab_slab_t *      slabs_free; ///< list of slabs with free objects

  l4slab_grow_fn_t     grow_fn;    ///< cache grow callback
  l4slab_release_fn_t  release_fn; ///< slab release callback

  void *               data;       ///< application data pointer
};

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Initialize slab cache.
 * \ingroup api_init
 * 
 * \param   cache        Slab cache descriptor
 * \param   size         Size of the cache objects
 * \param   max_free     Maximum number of free pages allowed in the cache. 
 *                       If more pages in the slab cache are freed, they are 
 *                       released  (if a release callback function is 
 *                       specified).
 * \param   grow_fn      Cache grow callback function, called by the slab cache
 *                       to allocate new pages for the cache. If no function is
 *                       specified the cache cannot allocate pages on demand.
 * \param   release_fn   Slab release callback function, called by the cache to 
 *                       release unused pages. If no function is specified 
 *                       unused pages are not released. 
 *	
 * \return  0 on success (initialized cache descriptor), error code otherwise:
 *          - -L4_EINVAL  size too big / invalid cache descriptor
 *
 * Setup cache descriptor. The function initializes the internal cache 
 * descriptor structures, but does not allocate any memory. Memory can be 
 * added using the l4slab_add_page() function or memory is allocated 
 * on demand by the cache if the grow callback function is specified. 
 */
/*****************************************************************************/ 
int
l4slab_cache_init(l4slab_cache_t * cache, l4_size_t size,
		  unsigned int max_free, l4slab_grow_fn_t grow_fn,
		  l4slab_release_fn_t release_fn);

/*****************************************************************************/
/**
 * \brief   Destroy slab cache
 * \ingroup api_init
 * 
 * \param   cache        Cache descriptor
 *
 * Release slab descriptor and free all allocated memory. This function is 
 * only useful if a release callback function is specified for the cache, 
 * otherwise it has no effect.
 */
/*****************************************************************************/ 
void
l4slab_destroy(l4slab_cache_t * cache);

/*****************************************************************************/
/**
 * \brief   Allocate object
 * \ingroup api_alloc
 * 
 * \param   cache        Cache descriptor
 *	
 * \return pointer to object, NULL if allocation failed.
 */
/*****************************************************************************/ 
void *
l4slab_alloc(l4slab_cache_t * cache);

/*****************************************************************************/
/**
 * \brief   Release object
 * \ingroup api_alloc
 * 
 * \param   cache        Cache descriptor
 * \param   objp         Pointer to object
 */
/*****************************************************************************/ 
void
l4slab_free(l4slab_cache_t * cache, void * objp);

/*****************************************************************************/
/**
 * \brief   Add a page to the slab cache
 * \ingroup api_init
 * 
 * \param   cache        Cache descriptor
 * \param   page         Pointer to new page
 * \param   data         Application data
 *
 * Add the page to the slab cache. 
 */
/*****************************************************************************/ 
void
l4slab_add_page(l4slab_cache_t * cache, void * page, void * data);

/*****************************************************************************/
/**
 * \brief   Set cache application data pointer.
 * \ingroup api_init
 * 
 * \param   cache        Cache descriptor
 * \param   data         Application data pointer
 */
/*****************************************************************************/ 
void 
l4slab_set_data(l4slab_cache_t * cache, void * data);

/*****************************************************************************/
/**
 * \brief   Get cache application data.
 * \ingroup api_init
 * 
 * \param   cache        Cache descriptor
 *	
 * \return Application data pointer, NULL if invalid cache descriptor or no
 *         data pointer set.
 */
/*****************************************************************************/ 
void * 
l4slab_get_data(l4slab_cache_t * cache);

/*****************************************************************************/
/**
 * \brief   Dump cache slab list
 * \ingroup api_debug
 * 
 * \param   cache        Cache descriptor
 * \param   dump_free    Dump free list of slabs
 */
/*****************************************************************************/ 
void 
l4slab_dump_cache(l4slab_cache_t * cache, int dump_free);

/*****************************************************************************/
/**
 * \brief   Dump cache free slab list
 * \ingroup api_debug
 * 
 * \param   cache        Cache descriptor
 */
/*****************************************************************************/ 
void 
l4slab_dump_cache_free(l4slab_cache_t * cache);

__END_DECLS;

#endif /* !_L4SLAB_SLAB_H */
