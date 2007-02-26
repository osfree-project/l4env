/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__debug.h
 * \brief  Debug config. 
 *
 * \date   05/31/2000 
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___DEBUG_H
#define _L4RM___DEBUG_H

/* enable debug assertions */
#define DEBUG_ASSERTIONS        1

/* be more verbose on errors */
#define DEBUG_ERRORS            1

/* 1 enables debug output, 0 disables */
#define DEBUG_PAGEFAULT         0 /* be careful: both DEBUG_PAGEFAULT and */
#define DEBUG_IO_PAGEFAULT	0
#define DEBUG_REQUEST           0 /* DEBUG_REQUEST might cause deadlocks if
				   * used with the log server */
#define DEBUG_ATTACH            0
#define DEBUG_DETACH            0
#define DEBUG_LOOKUP            0
#define DEBUG_AVLT              0
#define DEBUG_AVLT_ALLOC        0
#define DEBUG_AVLT_INSERT       0
#define DEBUG_AVLT_REMOVE       0
#define DEBUG_AVLT_SHOW         0
#define DEBUG_REGION_INIT       0
#define DEBUG_REGION_NEW        0
#define DEBUG_REGION_INSERT     0
#define DEBUG_REGION_FIND       0
#define DEBUG_REGION_MODIFY     0
#define DEBUG_REGION_RESERVE    0
#define DEBUG_REGION_SETUP      0
#define DEBUG_REGION_UNMAP      0
#define DEBUG_REGION_TREE       0
#define DEBUG_ALLOC_INIT        0
#define DEBUG_ALLOC_PAGE        0

/* build some debug functions in AVL tree implementation */
#define DO_DEBUG_AVLT           1

#endif /* !_L4RM___DEBUG_H */
