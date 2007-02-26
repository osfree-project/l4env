/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__debug.h
 * \brief  Debug config. 
 *
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___DEBUG_H
#define _DM_PHYS___DEBUG_H

/* enable map fpage sanity checks */
#define MAP_FPAGE_PARANOIA         1

/* enable debug assertions */
#define DEBUG_ASSERTIONS           1

/* be more verbose on errors */
#define DEBUG_ERRORS               1

/* 1 enables debug output, 0 disables */
#define DEBUG_ARGS                 0
#define DEBUG_SIGMA0               0
#define DEBUG_INT_ALLOC            0
#define DEBUG_MEMMAP_POOLS         0
#define DEBUG_MEMMAP_RESERVE       0
#define DEBUG_MEMMAP_MAP           0
#define DEBUG_MEMMAP_PAGESIZE      0
#define DEBUG_PAGES_INIT           0
#define DEBUG_PAGES_VERBOSE_INIT   0
#define DEBUG_PAGES_DESC           0
#define DEBUG_PAGES_ADD            0
#define DEBUG_PAGES_SPLIT          0
#define DEBUG_PAGES_FIND_SINGLE    0
#define DEBUG_PAGES_ALLOCATE       0
#define DEBUG_PAGES_ALLOCATE_AREA  0
#define DEBUG_PAGES_ALLOCATE_ADD   0
#define DEBUG_PAGES_RELEASE        0
#define DEBUG_PAGES_ENLARGE        0
#define DEBUG_PAGES_SHRINK         0
#define DEBUG_FLICK_REQUEST        0
#define DEBUG_MAP                  0
#define DEBUG_UNMAP                0
#define DEBUG_FAULT                0
#define DEBUG_OPEN                 0
#define DEBUG_CLOSE                0
#define DEBUG_SHARE                0
#define DEBUG_REVOKE               0
#define DEBUG_TRANSFER             0
#define DEBUG_COPY                 0
#define DEBUG_PHYS_ADDR            0
#define DEBUG_LOCK                 0
#define DEBUG_RESIZE               0
#define DEBUG_PAGESIZE             0

#endif /* !_DM_PHYS___DEBUG_H */
