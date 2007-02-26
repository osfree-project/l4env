/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_phys/include/dm_phys.h
 * \brief   Phys. memory dataspace manager client interface
 * \ingroup api
 *
 * \date    11/22/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS_DM_PHYS_H
#define _DM_PHYS_DM_PHYS_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_mem/dm_mem.h>

/* DMphys includes */
#include <l4/dm_phys/consts.h>
#include <l4/dm_phys/dm_phys-client.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Create new dataspace (extended version for DMphys)
 * \ingroup api_open
 * 
 * \param   pool         Memory pool (0-7), predefined pools are:
 *                       - #L4DM_MEMPHYS_DEFAULT default memory pool
 *                       - #L4DM_MEMPHYS_ISA_DMA ISA DMA capable memory 
 *                         (below 16MB)
 * \param   addr         Start address of memory area, set to 
 *                         #L4DM_MEMPHYS_ANY_ADDR to find a suitable memory
 *                         area, otherwise the contiguous area at \a addr
 *                         is allocated.
 * \param   size         Dataspace size
 * \param   align        Memory area alignment, it is only used if \a addr is 
 *                         set to #L4DM_MEMPHYS_ANY_ADDR and the flag
 *                         #L4DM_CONTIGUOUS ist set
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS  allocate contiguous memory area
 *                       - #L4DM_MEMPHYS_SUPERPAGES allocate dataspace memory 
 *                         using super-pages. It implies #L4DM_CONTIGUOUS and 
 *                         \a size and \a align will be adapted if necessary.
 * \param   name         Dataspace name
 * \retval  ds           Dataspace id
 *	
 * \return  0 on success (created dataspace, ds contains a valid dataspace id),
 *          error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_ENOMEM  out of memory
 *
 * Call DMphys to create a new dataspace.
 */
/*****************************************************************************/ 
int
l4dm_memphys_open(int pool, l4_addr_t addr, l4_size_t size, l4_addr_t align, 
		  l4_uint32_t flags, const char * name, 
		  l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Create dataspace copy (extended DMphys version)
 * \ingroup api_open
 * 
 * \param   ds           Source dataspace id
 * \param   src_offs     Offset in source dataspace
 * \param   dst_offs     Offset in destination dataspace
 * \param   num          Number of bytes to copy, set to #L4DM_WHOLE_DS to copy
 *                         the whole dataspace starting at \a src_offs
 * \param   dst_pool     Memory pool to use to allocate destination dataspace
 * \param   dst_addr     Phys. address of destination dataspace, set to 
 *                         #L4DM_MEMPHYS_ANY_ADDR to find an appropriate 
 *                         address
 * \param   dst_size     Size of destination dataspace, if larger than 
 *                         \a dst_offs + \a num it is used as the size of the
 *                         destination dataspace
 * \param   dst_align    Alignment of destination dataspace
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS        create copy on phys. 
 *                                                 contiguos memory
 *                       - #L4DM_MEMPHYS_SAME_POOL use same memory pool like
 *                                                 source to allocate 
 *                                                 destination dataspace
 * \param   name         Destination dataspace name
 * \retval  copy         Copy dataspace id
 *	
 * \return  0 on success (\a copy contains the id of the created copy),
 *          error code otherwise:
 *          - -#L4_EIPC       IPC error calling dataspace manager
 *          - -#L4_EINVAL     Invalid source dataspace id
 *          - -#L4_EPERM      Permission denied
 *          - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *          - -#L4_ENOMEM     Out of memory creating copy
 *
 * \note    DMphys does not support copy-on-write dataspace copies, the 
 *          flag #L4DM_COW is ignored.
 */
/*****************************************************************************/ 
int
l4dm_memphys_copy(const l4dm_dataspace_t * ds, l4_offs_t src_offs, 
                  l4_offs_t dst_offs, l4_size_t num, int dst_pool, 
		  l4_addr_t dst_addr, l4_size_t dst_size, 
		  l4_addr_t dst_align, l4_uint32_t flags, 
		  const char * name, l4dm_dataspace_t * copy);

/*****************************************************************************/
/**
 * \brief   Check pagesize of dataspace region
 * \ingroup api_mem
 * 
 * \param   ds           Dataspace id
 * \param   offs         Offset in dataspace
 * \param   size         Dataspace region size
 * \param   pagesize     Pagesize (log2)
 * \retval  ok           1 if dataspace region can be mapped with given
 *                       pagesize, 0 if not
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC        IPC error calling DMphys
 *          - -#L4_EPERM       operation not permitted
 *          - -#L4_EINVAL      invalid dataspace id
 *          - -#L4_EINVAL_OFFS offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_memphys_pagesize(const l4dm_dataspace_t * ds, l4_offs_t offs, 
                      l4_size_t size, int pagesize, int * ok);

/*****************************************************************************/
/**
 * \brief   Check if dataspaces attached to VM area can be mapped
 *          with a certain pagesize.
 * \ingroup api_mem
 * 
 * \param   ptr          VM area address
 * \param   size         VM area size
 * \param   pagesize     Log2 pagesize
 *	
 * \return 1 if dataspace can be mapped with given pagesize, 0 if not
 */
/*****************************************************************************/ 
int
l4dm_memphys_check_pagesize(const void * ptr, l4_size_t size, int pagesize);

/*****************************************************************************/
/**
 * \brief   Return size of memory pool
 * \ingroup api_mem 
 * 
 * \param   pool         Pool number
 * \retval  size         Memory pool size
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  Invalid memory pool
 *          - -#L4_ENODM   DMphys not found
 *          - -#L4_EIPC    Dataspace manager call failed
 */
/*****************************************************************************/ 
int
l4dm_memphys_poolsize(int pool, l4_size_t * size, l4_size_t * free);

/*****************************************************************************/
/**
 * \brief   Show DMphys memory map 
 * \ingroup api_debug
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_memmap(void);

/*****************************************************************************/
/**
 * \brief   Show DMphys memory pools
 * \ingroup api_debug
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pools(void);

/*****************************************************************************/
/**
 * \brief   Show memory areas of a memory pool
 * \ingroup api_debug
 * 
 * \param   pool         Memory pool number
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pool_areas(int pool);

/*****************************************************************************/
/**
 * \brief   Show free lists of a memory pool
 * \ingroup api_debug
 * 
 * \param   pool         Memory pool number
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_pool_free(int pool);

/*****************************************************************************/
/**
 * \brief   Show descriptor slab cache information
 * \ingroup api_debug
 *
 * \param   show_free     Show slab cache free lists
 */
/*****************************************************************************/ 
void
l4dm_memphys_show_slabs(int show_free);

/*****************************************************************************/
/**
 * \brief   Find DMphys
 * \ingroup api_open
 *	
 * \return  DMphys id, #L4_INVALID_ID if not found.
 */
/*****************************************************************************/ 
l4_threadid_t 
l4dm_memphys_find_dmphys(void);

__END_DECLS;

#endif /* !_DM_PHYS_DM_PHYS_H */
