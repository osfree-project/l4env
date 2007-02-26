/* $Id$ */
/*****************************************************************************/
/**
 * \file   ds_phys/server/include/__memmap.h
 * \brief  DMphys low level memory map
 *
 * \date   02/05/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _DM_PHYS___MEMMAP_H
#define _DM_PHYS___MEMMAP_H

/* L4/L4Env includes */
#include <l4/sys/types.h>

/* DMphys includes */
#include "__config.h"

/*****************************************************************************
 *** typedefs 
 *****************************************************************************/

/**
 * memory map entry
 */
typedef struct memmap
{
  l4_addr_t       addr;      ///< memory area address
  l4_size_t       size;      ///< memory area size

  l4_uint8_t      pagesize;  ///< pagesize (log2) used to map memory area
  l4_uint8_t      pool;      ///< page pool assigned to the memory area
  l4_uint16_t     flags;     ///< flags

  struct memmap * prev;   
  struct memmap * next;
} memmap_t;

/* memmap flags */
#define MEMMAP_UNUSED          0x0001
#define MEMMAP_MAPPED          0x0002
#define MEMMAP_RESERVED        0x0004
#define MEMMAP_DENIED          0x0008

#define MEMMAP_IS_UNUSED(m)    ((m)->flags & MEMMAP_UNUSED)
#define MEMMAP_IS_MAPPED(m)    ((m)->flags & MEMMAP_MAPPED)
#define MEMMAP_IS_RESERVED(m)  ((m)->flags & MEMMAP_RESERVED)

#define MEMMAP_INVALID_POOL    99

/**
 * memory pool configuration
 */
typedef struct pool_cfg
{
  l4_size_t size;                            ///< pool size
  l4_addr_t low;                             ///< search area low address
  l4_addr_t high;                            ///< search area high address
  l4_size_t reserved;                        ///< reserved memory in pool
  char      name[DMPHYS_MEM_POOL_NAME_LEN];  ///< pool name
} pool_cfg_t;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init memmap handling */
int
dmphys_memmap_init(void);

/* mark memory area reserved for internal use */
int
dmphys_memmap_reserve(l4_addr_t addr, 
		      l4_size_t size);

/* set memory map search low address */
void
dmphys_memmap_set_mem_low(l4_addr_t low);

/* set memory map search high address */
void
dmphys_memmap_set_mem_high(l4_addr_t high);

/* set memory pool configuration */
int
dmphys_memmap_set_pool_config(int pool, 
			      l4_size_t size, 
			      l4_addr_t low, 
			      l4_addr_t high, 
			      const char * name);

/* setup memory pools */
int
dmphys_memmap_setup_pools(int use_rmgr, 
			  int use_4M_pages);

/* check pagesize */
int
dmphys_memmap_check_pagesize(l4_addr_t addr, 
			     l4_size_t size, 
			     int pagesize);

/* DEBUG: show memory map */
void
dmphys_memmap_show(void);

#endif /* !_DM_PHYS___MEMMAP_H */
