/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__region_alloc.h
 * \brief  L4RM region descriptor allocation
 *
 * \date   02/13/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___REGION_ALLOC_H
#define _L4RM___REGION_ALLOC_H

/* L4/L4Env include */
#include <l4/env/cdefs.h>
#include <l4/slab/slab.h>

/* L4RM includes */
#include "__region.h"

/*****************************************************************************
 *** global symbols
 *****************************************************************************/

/* region descriptor slab cache */
extern l4slab_cache_t l4rm_region_cache;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init descriptor allocation */
int
l4rm_region_alloc_init(void);

/* allocate region descriptor */
L4_INLINE l4rm_region_desc_t *
l4rm_region_desc_alloc(void);

/* release region descriptor */
L4_INLINE void
l4rm_region_desc_free(l4rm_region_desc_t * r);

/*****************************************************************************
 *** implementaions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate region descriptor
 * 
 * \return Pointer to region descriptor, NULL if allocation failed.
 */
/*****************************************************************************/ 
L4_INLINE l4rm_region_desc_t *
l4rm_region_desc_alloc(void)
{
  l4rm_region_desc_t * r = l4slab_alloc(&l4rm_region_cache);

  if (r == NULL)
    return NULL;

  /* setup descriptor */
  r->flags = REGION_INITIALIZER;
  r->userptr = 0;
  r->next = NULL;
  r->prev = NULL;

  /* done */
  return r;
}

/*****************************************************************************/
/**
 * \brief  Release region descriptor
 * 
 * \param  r             Region descriptor
 */
/*****************************************************************************/ 
L4_INLINE void
l4rm_region_desc_free(l4rm_region_desc_t * r)
{
  l4slab_free(&l4rm_region_cache,r);
}

#endif
