/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/avl_tree_alloc.c
 * \brief  Allocation of tree nodes.
 *
 * \date   05/27/2000
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/slab/slab.h>
#include <l4/util/macros.h>

/* private includes */
#include "__avl_tree_alloc.h"
#include "__avl_tree.h"
#include "__alloc.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * AVL tree node slab cache
 */
l4slab_cache_t l4rm_avl_node_cache;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Grow slab cache callback
 * 
 * \param  cache         Cache descriptor
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
 *** internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Setup tree node allocation. 
 *	
 * \return 0 on success, -1 if initialization failed.
 */
/*****************************************************************************/ 
int
avlt_alloc_init(void)
{
  int ret;

  /* setup slab cache */
  ret = l4slab_cache_init(&l4rm_avl_node_cache,sizeof(avlt_t),0,__grow,NULL);
  if (ret < 0)
    {
      Panic("L4RM: init AVL tree slab cache failed!");
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief DEBUG: return index of a tree node in allocator node table.
 * 
 * \param  node          pointer to tree node
 *	
 * \return index of node in node table, -1 if invalid pointer.
 */
/*****************************************************************************/ 
unsigned int
avlt_node_index(avlt_t * node)
{
  /* we don't use a staic table anymore, return address */
  return (unsigned int)node;
}
