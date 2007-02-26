/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/region_alloc.c
 * \brief  Allocation/deallocation of region descriptors
 *
 * \date   07/26/2001
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

/* L4env includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/slab/slab.h>
#include <l4/util/macros.h>

/* private includes */
#include "__region_alloc.h"
#include "__region.h"
#include "__alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Region descriptor slab cache
 */
l4slab_cache_t l4rm_region_cache;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Grow slab callback function
 * 
 * \param  cache         Slab cache descriptor
 * \param  data          Slab data pointer, unused
 */
/*****************************************************************************/ 
static void *
__grow(l4slab_cache_t * cache, 
       void ** data)
{
  *data = NULL;
  return l4rm_heap_alloc();
}

/*****************************************************************************
 *** L4RM internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init region descriptor allocation
 *	
 * \return 0 on success, -1 if initialization failed.
 */
/*****************************************************************************/ 
int
l4rm_region_alloc_init(void)
{
  int ret;

  /* initialize slab cache */
  ret = l4slab_cache_init(&l4rm_region_cache,sizeof(l4rm_region_desc_t),
			  1,__grow,NULL);
  if (ret < 0)
    {
      Panic("L4RM: region descriptor slab cache initialization failed!");
      return -1;
    }

  /* done */
  return 0;
}
