/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__dm_phys.h
 * \brief  DMphys misc. internal defines
 *
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___DM_PHYS_H
#define _DM_PHYS___DM_PHYS_H

/* L4/L4Env includes */
#include <l4/sys/types.h>

/* DMphys includes */
#include "__dataspace.h"
#include "__pages.h"

/*****************************************************************************
 *** symbols
 *****************************************************************************/

/**
 * DMphys service thread id 
 */
extern l4_threadid_t dmphys_service_id;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* DMphys service loop */
void
dmphys_server(void);

/* unmap page area */
void
dmphys_unmap_area(l4_addr_t addr, 
		  l4_size_t size);

/* unmap page area list */
void
dmphys_unmap_areas(page_area_t * areas);

/* create new dataspace */
int
dmphys_open(l4_threadid_t owner, 
	    page_pool_t * pool, 
	    l4_addr_t addr, 
	    l4_size_t size, 
	    l4_addr_t align, 
	    l4_uint32_t flags, 
	    const char * name, 
	    l4dm_dataspace_t * ds);

/* close dataspace */
int
dmphys_close(dmphys_dataspace_t * ds);

/* resize dataspace */
int
dmphys_resize(dmphys_dataspace_t * ds, 
	      l4_size_t new_size);

#endif /* !_DM_PHYS___DM_PHYS_H */
