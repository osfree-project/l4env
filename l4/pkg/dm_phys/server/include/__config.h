/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__config.h
 * \brief  DMphys internal configuration
 *
 * \date   08/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___CONFIG_H
#define _DM_PHYS___CONFIG_H

/*****************************************************************************
 *** Memory pools
 *****************************************************************************/

/**
 * Number of memory pools.
 *
 * Memory pools can be used to explicitly reserve memory regions for special
 * purposes. A pool must be defined with the --poolx= command line argument.
 * On the dataspace creation the pool is specified in a flag-field of the
 * dataspace open call.
 *
 * Currently we have two default pools:
 * - ISA DMA capable memory, this must be memory below 16MB
 *   the amount of memory used for this pool can be specified with the
 *   --isa_dma_size command line argument, the default value is
 *   DMPHYS_MEM_ISA_DMA
 * - the rest of the memory (the default pool if no pool is specified)
 */
#define DMPHYS_NUM_POOLS              8

/*****************************************************************************
 *** Physical memory
 *****************************************************************************/

/**
 * start address of memory to be used
 */
#define DMPHYS_MEM_LOW                0x00100000

/**
 * page size -> smallest dataspace size
 */
#define DMPHYS_PAGESIZE               L4_PAGESIZE

/**
 * page log2 size -> smallest alignment
 */
#define DMPHYS_LOG2_PAGESIZE          L4_LOG2_PAGESIZE

/**
 * 4MB-page size
 */
#define DMPHYS_SUPERPAGESIZE          L4_SUPERPAGESIZE

/**
 * 4MB-page log2 size
 */
#define DMPHYS_LOG2_SUPERPAGESIZE     L4_LOG2_SUPERPAGESIZE

/**
 * address space log2 size
 */
#define DMPHYS_AS_LOG2_SIZE           32

/**
 * page mask
 */
#define DMPHYS_PAGEMASK               (~(DMPHYS_PAGESIZE - 1))

/*
 * default memory pool, use every page we can get,
 * range 0 - end of phys. memory
 */
#define DMPHYS_MEM_DEFAULT_POOL       0           ///< pool number
#define DMPHYS_MEM_DEFAULT_SIZE      -1           ///< default pool size
#define DMPHYS_MEM_DEFAULT_LOW        0           ///< default range low
#define DMPHYS_MEM_DEFAULT_HIGH      -1           ///< default range high
#define DMPHYS_MEM_DEFAULT_RESERVED   32768       ///< reserved for int. use

/*
 * ISA DMA memory pool, default is to leave it empty
 */
#define DMPHYS_MEM_ISA_DMA_POOL       7           ///< pool number
#define DMPHYS_MEM_ISA_DMA_SIZE       0           ///< ISA DMA pool size
#define DMPHYS_MEM_ISA_DMA_LOW        0           ///< ISA DMA range low
#define DMPHYS_MEM_ISA_DMA_HIGH       0x01000000  ///< ISA DMA range high
#define DMPHYS_MEM_ISA_DMA_RESERVED   0           ///< no reserved memory

/**
 * max. length of page pool name
 */
#define DMPHYS_MEM_POOL_NAME_LEN      32

/*****************************************************************************
 *** Map areas for physical memory / L4 kernel info page
 *****************************************************************************/

/**
 * memory map area start address
 */
#define DMPHYS_MEMMAP_START           0x00000000U

/**
 * memory map area size
 */
#define DMPHYS_MEMMAP_LOG2_SIZE       32

/*****************************************************************************
 *** Page area lists / free lists
 *****************************************************************************/

/**
 * number of free lists
 *
 * The available sizes are divided into groups with sizes of a power of two,
 * the free list i contains all free areas with a size of 2^i * PAGESIZE to
 * 2^(i+1) * PAGESIZE - 1.
 */
#define DMPHYS_NUM_FREE_LISTS         16

/*****************************************************************************
 *** Page allocation
 *****************************************************************************/

/**
 * max. number of page areas in a dataspace
 *
 * We try to assemble regular dataspaces (not contiguous) from several
 * smaller page areas to avoid splitting up larger areas. However, we must
 * restrict the number of used areas to avoid large overheads for searching
 * a page e.g. to map it.
 */
#define DMPHYS_MAX_DS_AREAS           32

/*****************************************************************************
 *** Descriptor allocation
 *****************************************************************************/

/**
 * Number of initial pages in memory pool
 */
#define DMPHYS_INT_POOL_INITIAL       32

/**
 * Max. number of pages allocated from default page pool in memory pool
 */
#define DMPHYS_INT_POOL_MAX_ALLOC     32

/**
 * Min. number of pages in memory pool
 */
#define DMPHYS_INT_POOL_MIN           8

/**
 * Max. number of released memory pool page areas
 */
#define DMPHYS_INT_MAX_FREED          32

#endif /* !_DM_PHYS___CONFIG_H */
