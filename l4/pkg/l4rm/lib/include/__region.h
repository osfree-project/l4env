/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__region.h
 * \brief  Region types
 *
 * \date   06/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___REGION_H
#define _L4RM___REGION_H

/* L4/L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/lock/lock.h>
#include <l4/dm_generic/types.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__config.h"
#include "__avl_tree.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * region list lock 
 */
extern l4lock_t region_list_lock;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init region handling */
int
l4rm_init_regions(void);

/* region handling*/
int
l4rm_new_region(l4rm_region_desc_t * region, l4_addr_t addr, l4_size_t size, 
                l4_uint32_t area, l4_uint32_t flags);

void
l4rm_free_region(l4rm_region_desc_t * region);

L4_INLINE l4rm_region_desc_t *
l4rm_find_used_region(l4_addr_t addr);

l4rm_region_desc_t *
l4rm_find_region(l4_addr_t addr);

void
l4rm_unmap_region(l4rm_region_desc_t * region);

/* region tree handling */
int
l4rm_tree_insert_region(l4rm_region_desc_t * region, l4_uint32_t flags);

int
l4rm_tree_remove_region(l4_addr_t addr, l4_uint32_t type, l4_uint32_t flags, 
			l4rm_region_desc_t ** region);

/* vm areas */
void 
l4rm_reset_area(l4_uint32_t area);

void
l4rm_show_region_list(void);

/* region list lock */
L4_INLINE void
l4rm_lock_region_list(void);

L4_INLINE void 
l4rm_unlock_region_list(void);

L4_INLINE void
l4rm_lock_region_list_direct(l4_uint32_t flags);

L4_INLINE void 
l4rm_unlock_region_list_direct(l4_uint32_t flags);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Find VM region addr belongs to.
 * 
 * \param  addr          Address
 *	
 * \return Pointer to region descriptor, NULL if not found.
 */
/*****************************************************************************/ 
L4_INLINE l4rm_region_desc_t *
l4rm_find_used_region(l4_addr_t addr)
{
  avlt_key_t k;
  avlt_data_t r;
  int ret;
  
  /* search region in AVL tree */
  k.start = k.end = addr;
  ret = avlt_find(k,&r);
  if (ret)
    return NULL;

  /* done */
  return (l4rm_region_desc_t *)r;
}

/*****************************************************************************/
/**
 * \brief Lock region list
 */
/*****************************************************************************/ 
L4_INLINE void
l4rm_lock_region_list(void)
{
  /* lock */
  l4lock_lock(&region_list_lock);
}

/*****************************************************************************/
/**
 * \brief Unlock region list
 */
/*****************************************************************************/ 
L4_INLINE void
l4rm_unlock_region_list(void)
{
  /* unlock */
  l4lock_unlock(&region_list_lock);
}

/*****************************************************************************/
/**
 * \brief Conditional lock region list
 *
 * \param  flags         Flags: 
 *                       - \c MODIFY_DIRECT  do not lock region list (required
 *                                           for direct attach/reserve
 */
/*****************************************************************************/ 
L4_INLINE void
l4rm_lock_region_list_direct(l4_uint32_t flags)
{
  if (!(flags & L4RM_MODIFY_DIRECT))
      /* lock */
      l4lock_lock(&region_list_lock);
}

/*****************************************************************************/
/**
 * \brief Conditional Unlock region list
 * 
 * \param  flags         Flags: 
 *                       - \c MODIFY_DIRECT  do not lock region list (required
 *                                           for direct attach/reserve
 */
/*****************************************************************************/ 
L4_INLINE void
l4rm_unlock_region_list_direct(l4_uint32_t flags)
{
  if (!(flags & L4RM_MODIFY_DIRECT))
    /* unlock */
    l4lock_unlock(&region_list_lock);
}

#endif /* !_L4RM___REGION_H */
