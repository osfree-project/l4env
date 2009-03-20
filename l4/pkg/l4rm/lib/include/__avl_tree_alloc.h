/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__avl_tree_alloc.h
 * \brief  L4RM AVL tree node allocation
 *
 * \date   02/13/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
*/
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___AVL_TREE_ALLOC_H
#define _L4RM___AVL_TREE_ALLOC_H

/* L4/L4Env include */
#include <l4/env/env.h>
#include <l4/slab/slab.h>
#include <l4/lock/lock.h>

/* L4RM includes */
#include "__avl_tree.h"

/*****************************************************************************
 *** global symbols
 *****************************************************************************/

/* AVL tree node slab cache and lock */
extern l4slab_cache_t l4rm_avl_node_cache;
extern l4lock_t avl_node_cache_lock;

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
l4_addr_t
avlt_node_index(avlt_t * node);

/*****************************************************************************
 *** implementaions
 *****************************************************************************/

/**
 * \brief Lock region slab cache.
 */
L4_INLINE void
l4rm_avl_node_cache_lock(void);
L4_INLINE void
l4rm_avl_node_cache_lock(void)
{
  if (l4env_startup_done())
    l4lock_lock(&avl_node_cache_lock);
}

/**
 * \brief Unlock region slab cache.
 */
L4_INLINE void
l4rm_avl_node_cache_unlock(void);
L4_INLINE void
l4rm_avl_node_cache_unlock(void)
{
  if (l4env_startup_done())
    l4lock_unlock(&avl_node_cache_lock);
}

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
  avlt_t *r;
  l4rm_avl_node_cache_lock();
  r = l4slab_alloc(&l4rm_avl_node_cache);
  l4rm_avl_node_cache_unlock();
  return r;
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
  l4rm_avl_node_cache_lock();
  l4slab_free(&l4rm_avl_node_cache,node);
  l4rm_avl_node_cache_unlock();
}

#endif /* !_L4RM___AVL_TREE_ALLOC_H */
