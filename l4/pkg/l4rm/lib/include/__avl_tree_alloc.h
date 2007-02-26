/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__avl_tree_alloc.h
 * \brief  L4RM AVL tree node allocation
 *
 * \date   02/13/2002
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
#ifndef _L4RM___AVL_TREE_ALLOC_H
#define _L4RM___AVL_TREE_ALLOC_H

/* L4/L4Env include */
#include <l4/env/cdefs.h>
#include <l4/slab/slab.h>

/* L4RM includes */
#include "__avl_tree.h"

/*****************************************************************************
 *** global symbols
 *****************************************************************************/

/* AVL tree node slab cache */
extern l4slab_cache_t l4rm_avl_node_cache;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init allocation */
int
avlt_alloc_init(void);

/* allocate node */
L4_INLINE avlt_t *
avlt_new_node(void);

/* release node */
L4_INLINE void 
avlt_free_node(avlt_t * node);

/* DEBUG */
unsigned int
avlt_node_index(avlt_t * node);

/*****************************************************************************
 *** implementaions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate new tree node.
 * 
 * \return pointer to new tree node, NULL if allocation failed.
 */
/*****************************************************************************/ 
L4_INLINE avlt_t *
avlt_new_node(void)
{
  return l4slab_alloc(&l4rm_avl_node_cache);
}

/*****************************************************************************/
/**
 * \brief  Release node. 
 * 
 * \param  node          tree node
 */
/*****************************************************************************/ 
L4_INLINE void 
avlt_free_node(avlt_t * node)
{
  l4slab_free(&l4rm_avl_node_cache,node);
}

#endif /* !_L4RM___AVL_TREE_ALLOC_H */
