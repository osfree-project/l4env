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
  l4dm_dataspace_t          ds;       ///< backing dataspace
  l4_offs_t                 offs;     ///< start offset

  l4_addr_t                 start;    ///< start address 
  l4_addr_t                 end;      ///< end address

  l4_uint32_t               flags;    ///< region flags
  l4_uint32_t               rights;   /**< access rights to the attached 
				       **  dataspace */

  struct l4rm_region_desc * next;     ///< next region
  struct l4rm_region_desc * prev;     ///< previous region
} l4rm_region_desc_t;

/*****************************************************************************
 * Flags: 
 * 31                                           0
 *  +---+---+---+----------+--------------------+
 *  | f | a | r |  unused  |     area id        |
 *  +---+---+---+----------+--------------------+
 *    1   1   1      9             20
 *
 *  f       ... region free
 *  a       ... region attached to dataspace
 *  r       ... reserved region
 *  area id ... area whicht contains the region
 *****************************************************************************/

#define REGION_UNUSED    0xFFFFFFFF
#define REGION_USED      0x00000000

#define REGION_FREE      0x80000000
#define REGION_ATTACHED  0x40000000
#define REGION_RESERVED  0x20000000

#define AREA_MASK        0x000FFFFF

#define IS_UNUSED_REGION(r)    ((r)->flags == REGION_UNUSED)
#define IS_FREE_REGION(r)      ((r)->flags & REGION_FREE)
#define IS_ATTACHED_REGION(r)  ((r)->flags & REGION_ATTACHED)
#define IS_RESERVED_AREA(r)    ((r)->flags & REGION_RESERVED)

#define SET_USED(r)            (r)->flags = REGION_USED
#define SET_UNUSED(r)          (r)->flags = REGION_UNUSED
#define SET_REGION_FLAG(r,a)   (r)->flags |= a
#define CLEAR_REGION_FLAG(r,a) (r)->flags &= ~a
#define CLEAR_FLAGS(r)         (r)->flags &= AREA_MASK
#define SET_ATTACHED(r)        (r)->flags = ((r)->flags & ~REGION_FREE) | \
                                            REGION_ATTACHED
#define SET_FREE(r)            (r)->flags = ((r)->flags & ~REGION_ATTACHED) | \
                                            REGION_FREE
#define SET_RESERVED(r)        (r)->flags |= REGION_RESERVED

#define REGION_AREA(r)         ((r)->flags & AREA_MASK)
#define SET_AREA(r,a)          (r)->flags = (a & AREA_MASK) | \
                                            ((r)->flags & ~AREA_MASK)

#define FLAGS_EQUAL(r1,r2)     ((r1)->flags == (r2)->flags)

/* attach / reserve arguments */
#define ADDR_FIND        0xFFFFFFFF

/* attach / reserve flags */
#define MODIFY_DIRECT    0x80000000

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

/* insert/remove/find region */
int 
l4rm_insert_region(l4rm_region_desc_t * region);

void
l4rm_free_region(l4rm_region_desc_t * region);

int
l4rm_find_free_region(l4_size_t size, 
		      l4_uint32_t area, 
		      l4_uint32_t flags,
		      l4_addr_t * addr);

L4_INLINE l4rm_region_desc_t *
l4rm_find_used_region(l4_addr_t addr);

l4rm_region_desc_t *
l4rm_find_region(l4_addr_t addr);

l4_uint32_t
l4rm_get_access_rights(l4dm_dataspace_t * ds);

/* region tree handling */
int
l4rm_tree_insert_region(l4rm_region_desc_t * region, 
			l4_uint32_t flags);

int
l4rm_tree_remove_region(l4_addr_t addr, 
			l4_uint32_t flags, 
			l4rm_region_desc_t ** region);

int
l4rm_tree_lookup_region(l4_addr_t addr, 
			l4rm_region_desc_t ** region);

int
l4rm_tree_add_client(l4_threadid_t client, 
		     l4_uint32_t flags);

int
l4rm_tree_remove_client(l4_threadid_t client);

/* vm areas */
int
l4rm_insert_area(l4rm_region_desc_t * region);

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
  if (!(flags & MODIFY_DIRECT))
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
  if (!(flags & MODIFY_DIRECT))
    /* unlock */
    l4lock_unlock(&region_list_lock);
}

#endif /* !_L4RM___REGION_H */
