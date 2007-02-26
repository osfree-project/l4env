/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__avl_tree.h
 * \brief  Data types / public prototypes used by the AVL tree implementation.
 *
 * \date   05/27/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___AVL_TREE_H
#define _L4RM___AVL_TREE_H

/* L4 includes */
#include <l4/sys/types.h>

/*****************************************************************************
 * search key / node data type definition, it can be modified to use the
 * AVL tree implementation together with other search keys / data types.
 *****************************************************************************/

/**
 * search key
 */
typedef struct avlt_key
{
  l4_uint32_t start;               ///< start address of map region
  l4_uint32_t end;                 ///< end address od map region */
} avlt_key_t;

/* macros to test keys */
#define AVLT_IS_LOWER(a,b)      ((a).end < (b).start)
#define AVLT_IS_GREATER(a,b)    ((a).start > (b).end)
#define AVLT_IS_EQUAL(a,b)      (((a).start == (b).start) && \
                                 ((a).end == (b).end))

/* check if key a fits into key b */
#define AVLT_IS_IN(a,b)         (((a).start >= (b).start) && \
                                 ((a).end <= (b).end))

/**
 * node data type
 */
typedef void * avlt_data_t;     ///< pointer to region descriptor

/*****************************************************************************
 *** internal data types
 *****************************************************************************/

/**
 * tree node
 */
typedef struct avlt
{
  avlt_key_t     key;            ///< node search key
  avlt_data_t    data;           ///< node data

  /* internal stuff */
  l4_uint16_t    flags;          ///< node flags
  l4_int16_t     b;              ///< balance factor
  struct avlt *  left;           ///< left subtree
  struct avlt *  right;          ///< right subtree
} avlt_t;

/*****************************************************************************
 *** error codes
 *****************************************************************************/

#define AVLT_OK             0
#define AVLT_NEW            1
#define AVLT_NOT_FOUND      2
#define AVLT_NO_MEM         3
#define AVLT_EXISTS         4 
#define AVLT_INVAL          5

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init AVL tree handling */
int
avlt_init(void);

/* insert new node */
int
avlt_insert(avlt_key_t key, avlt_data_t data);

/* remove node */
int 
avlt_remove(avlt_key_t key);

/* search node */
int
avlt_find(avlt_key_t key, avlt_data_t * data);

/* DEBUG */
void
avlt_show_tree(void);

int
AVLT_insert(l4_uint32_t start, l4_uint32_t end, l4_uint32_t data);

int
AVLT_remove(l4_uint32_t start, l4_uint32_t end);

int 
AVLT_find(l4_uint32_t start, l4_uint32_t end);

#endif /* !_L4RM___AVL_TREE_H */ 
