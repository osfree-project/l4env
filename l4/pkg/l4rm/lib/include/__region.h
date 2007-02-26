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
 *** data types
 *****************************************************************************/

/**
 * internal region descriptor 
 */
typedef struct l4rm_region_desc
{
  /* region description */
  l4_addr_t                 start;    ///< start address 
  l4_addr_t                 end;      ///< end address
  l4_uint32_t               flags;    ///< region flags

  /* region data */
  union
  {
    /* dataspace region */
    struct
    {
      l4dm_dataspace_t      ds;       ///< attached dataspace
      l4_offs_t             offs;     ///< start offset
      l4_uint32_t           rights;   /**< access rights to the attached 
				       **  dataspace */
    } ds;

    /* region with external pager */
    struct
    {
      l4_threadid_t         pager;    ///< external pager
    } pager;
  } data;

  void *                    userptr;  ///< user pointer for area

  struct l4rm_region_desc * next;     ///< next region
  struct l4rm_region_desc * prev;     ///< previous region
} l4rm_region_desc_t;

/*****************************************************************************
 * Flags: 
 * 31                                      0
 *  +--------+--------+--------------------+
 *  |  type  | unused |     area id        |
 *  +--------+--------+--------------------+
 *      4        8             20
 *
 *  type    ... region type:
 *              0 .. free region
 *              1 .. dataspace region
 *              2 .. region with external pager
 *              3 .. forward exception
 *              4 .. blocked region
 *  area id ... area whicht contains the region
 *****************************************************************************/

#define REGION_FREE        0x00000000
#define REGION_DATASPACE   0x10000000
#define REGION_PAGER       0x20000000
#define REGION_EXCEPTION   0x30000000
#define REGION_BLOCKED     0x40000000

#define AREA_MASK          0x000FFFFF
#define TYPE_MASK          0xF0000000

#define REGION_TYPE(r)          ((r)->flags & TYPE_MASK)
#define REGION_AREA(r)          ((r)->flags & AREA_MASK)

#define IS_FREE_REGION(r)       (REGION_TYPE(r)== REGION_FREE)
#define IS_USED_REGION(r)       (!IS_FREE_REGION(r))
#define IS_DATASPACE_REGION(r)  (REGION_TYPE(r)== REGION_DATASPACE)
#define IS_PAGER_REGION(r)      (REGION_TYPE(r)== REGION_PAGER)
#define IS_EXCEPTION_REGION(r)  (REGION_TYPE(r)== REGION_EXCEPTION)
#define IS_BLOCKED_REGION(r)    (REGION_TYPE(r)== REGION_BLOCKED)

#define SET_REGION_FREE(r)      (r)->flags = \
                                  ((r)->flags & AREA_MASK) | REGION_FREE
#define SET_REGION_DATASPACE(r) (r)->flags = \
                                  ((r)->flags & AREA_MASK) | REGION_DATASPACE
#define SET_REGION_PAGER(r)     (r)->flags = \
                                  ((r)->flags & AREA_MASK) | REGION_PAGER
#define SET_REGION_EXCEPTION(r) (r)->flags = \
                                  ((r)->flags & AREA_MASK) | REGION_EXCEPTION
#define SET_REGION_BLOCKED(r)   (r)->flags = \
                                  ((r)->flags & AREA_MASK) | REGION_BLOCKED
#define SET_AREA(r,a)           (r)->flags = \
                                  (a & AREA_MASK) | ((r)->flags & ~AREA_MASK)

#define FLAGS_EQUAL(r1,r2)      ((r1)->flags == (r2)->flags)
#define AREA_EQUAL(r1, r2)      (((r1)->flags & AREA_MASK) == \
                                  ((r2)->flags & AREA_MASK))

#define REGION_INITIALIZER      0

/* flags for l4rm_new_region, see also l4rm.h */
#define L4RM_TREE_INSERT   0x80000000   /* insert new region in region tree */
#define L4RM_SET_AREA      0x40000000   /* set area id in region descriptor */

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
