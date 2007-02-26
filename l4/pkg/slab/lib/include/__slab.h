/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/lib/include/__slab.h
 * \brief  Slab allocator internal types.
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
#ifndef _SLAB___SLAB_H
#define _SLAB___SLAB_H

/* slab includes */
#include <l4/slab/slab.h>

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Slab descriptor
 */
struct l4slab_slab
{
  int                    num_free;   ///< number of free objects in slab
 
  void *                 free_objs;  ///< free list 

  l4slab_cache_t *       cache;      ///< pointer to slab cache descriptor
 
  struct l4slab_slab *   free_prev;  ///< previous slab with free entries
  struct l4slab_slab *   free_next;  ///< next slab with free entries

  struct l4slab_slab *   slab_prev;  ///< previous slab 
  struct l4slab_slab *   slab_next;  ///< next slab

  void *                 data;       ///< client data
};

/// Max. slab object size 
#define L4SLAB_MAX_SIZE (L4_PAGESIZE - sizeof(l4slab_slab_t))

/**
 * Slab page descriptor 
 */
typedef struct l4slab_page
{
  unsigned char          objs[L4SLAB_MAX_SIZE];  ///< object area
  l4slab_slab_t          slab;       ///< slab descriptor 
} l4slab_page_t;

#endif /* !_SLAB___SLAB_H */
