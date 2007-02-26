/**
 * \file	loader/server/src/dm-if.h
 * \brief	Helper functions for communication with dataspace managers
 *
 * \date	06/11/2001
 * \author	Frank Mehnert */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __DM_IF_H_
#define __DM_IF_H_

#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>

extern l4_threadid_t app_dsm_id;	/**< default dataspace manager */
extern l4_threadid_t app_image_dsm;	/**< dsm for file image */
extern l4_threadid_t app_text_dsm;	/**< dsm for text section */
extern l4_threadid_t app_data_dsm;	/**< dsm for data section */
extern l4_threadid_t app_bss_dsm;	/**< dsm for bss section */
extern l4_threadid_t app_stack_dsm;	/**< dsm for stack */

int  create_ds(l4_threadid_t dm_id, l4_size_t size,
               l4_addr_t *addr, l4dm_dataspace_t *ds,
	       const char *dbg_name);
int  junk_ds(l4dm_dataspace_t *ds, l4_addr_t addr);
int  phys_ds(l4dm_dataspace_t *ds, l4_size_t size, l4_addr_t *phys_addr);
int  dm_if_init(void);

#endif

