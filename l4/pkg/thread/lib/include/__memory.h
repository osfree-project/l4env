/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__memory.h
 * \brief  Memory allocation.
 *
 * \date   08/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___MEMORY_H
#define _THREAD___MEMORY_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/dm_generic/types.h>

/*****************************************************************************
 *** data types
 *****************************************************************************/

/**
 * Memory descriptor.
 *
 * It stores all the necessary information about memory allocated by
 * l4th_pages_allocate(). 
 */
typedef struct l4th_mem_desc
{
  l4dm_dataspace_t ds;         ///< dataspace (returned by l4dm_mem_open)
  l4_addr_t        map_addr;   ///< map address
  l4_size_t        size;       ///< size
} l4th_mem_desc_t;

/* vm area allocation */
#define VM_FIND_REGION    0xFFFFFFFF   ///< find suitable vm region
#define VM_DEFAULT_AREA   0xFFFFFFFF   ///< use default vm area

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
l4th_pages_allocate(l4_size_t size, l4_addr_t map_addr, l4_uint32_t vm_area,
		    l4_threadid_t owner, const char * name, l4_uint32_t flags, 
		    l4th_mem_desc_t * desc);

int 
l4th_pages_free(l4th_mem_desc_t * desc);

int
l4th_pages_map(l4th_mem_desc_t * desc, l4_offs_t offs);

#endif /* !_THREAD___MEMORY_H */
