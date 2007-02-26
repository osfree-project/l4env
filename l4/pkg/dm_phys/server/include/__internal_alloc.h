/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__internal_alloc.h
 * \brief  DMphys internal memory allocation
 *
 * \date   02/04/2002
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
#ifndef _DMPHYS___INTERNAL_ALLOC_H
#define _DMPHYS___INTERNAL_ALLOC_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/slab/slab.h>

/* DMphys includes */
#include "__pages.h"

/*****************************************************************************
 *** typedefs 
 *****************************************************************************/

/**
 * Descriptor allocation memory pool
 */
typedef struct int_pool
{
  int           available;   ///< page available
  l4_addr_t     map_addr;    ///< page map address, -1 if not allocated
  page_area_t * area;        /**< page area descriptor, NULL if page directly 
			      **  allocated at sigma0 */
} int_pool_t;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init descriptor allocation */
int
dmphys_internal_alloc_init(void);

/* reserve initial pages */
void
dmphys_internal_alloc_init_reserve(void);

/* update internal memory pool */
void
dmphys_internal_alloc_update(void);

/* free unused page areas */
void
dmphys_internal_alloc_update_free(void);

/* generic slab cache grow callback */
void *
dmphys_internal_alloc_grow(l4slab_cache_t * cache, 
			   void ** data);

/* generic slab cache release callback */
void
dmphys_internal_alloc_release(l4slab_cache_t * cache, 
			      void * page, 
			      void * data);

/* allocate page */
void *
dmphys_internal_allocate(void ** data);

/* release page */
void
dmphys_internal_release(void * page, 
			void * data);

#endif /* !_DMPHYS___INTERNAL_ALLOC_H */
