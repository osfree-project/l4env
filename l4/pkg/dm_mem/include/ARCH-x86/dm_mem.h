/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_mem/include/l4/dm_mem/dm_mem.h
 * \brief   Memory dataspace manager client API
 * \ingroup api
 *
 * \date    11/23/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#ifndef _DM_MEM_DM_MEM_H
#define _DM_MEM_DM_MEM_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/l4rm/l4rm.h>               /* we also use some L4RM constants */
#include <l4/dm_generic/dm_generic.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem-client.h>

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Address region
 * \ingroup api_mem
 */
typedef struct l4dm_mem_addr
{
  l4_addr_t addr;   ///< region start address
  l4_size_t size;   ///< region size 
} l4dm_mem_addr_t;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Create new dataspace
 * \ingroup api_open
 * 
 * \param   dsm_id       Dataspace manager id, set to #L4DM_DEFAULT_DSM 
 *                       to use default dataspace manager provided by the
 *                       L4 environment
 * \param   size         Dataspace size
 * \param   align        Alignment
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS allocate dataspace on phys.
 *                         contiguous memory
 *                       - #L4DM_PINNED allocate "pinned" memory, there will
 *                         be no pagefaults once the dataspace is mapped
 * \param   name         Dataspace name
 * \retval  ds           Dataspace id
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_ENOMEM  out of memory
 *          - -#L4_ENODM   no dataspace manager found
 *
 * Call dataspace manager \a dsm_id to create a new memory dataspace.
 */
/*****************************************************************************/ 
int
l4dm_mem_open(l4_threadid_t dsm_id, 
	      l4_size_t size, 
	      l4_addr_t align, 
	      l4_uint32_t flags, 
	      const char * name, 
	      l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Return dataspace size
 * \ingroup api_mem
 * 
 * \param   ds           Dataspace descriptor
 * \retval  size         Dataspace size
 *	
 * \return  0 on success (\a size contains the dataspace size), 
 *          error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   Caller is not a client of the dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_size(l4dm_dataspace_t * ds, 
	      l4_size_t * size);

/*****************************************************************************/
/**
 * \brief   Resize dataspace
 * \ingroup api_open
 * 
 * \param   ds           Dataspace id
 * \param   new_size     New dataspace size
 *	
 * \return  0 on success (resized dataspace), error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   caller is not the owner of the dataspace
 *          - -#L4_ENOMEM  out of memory
 *
 * Resize the dataspace. If new_size is smaller than the current dataspace
 * size, the remaining area at the end of the dataspace will be freed, if 
 * the new size is 0 the dataspace will be closed. If the new size is 
 * larger than the current size, more memory is added to the dataspace. 
 * If the dataspace was allocated on contiguous memory, enlarge the used
 * contiguous memory area.
 */
/*****************************************************************************/ 
int
l4dm_mem_resize(l4dm_dataspace_t * ds, 
		l4_size_t new_size);

/*****************************************************************************/
/**
 * \brief   Get phys. address of dataspace region
 * \ingroup api_mem
 * 
 * \param   ds           Dataspace id
 * \param   offset       Offset in dataspace
 * \param   size         Region size, #L4DM_WHOLE_DS to get the size of the
 *                       contiguous area at offset
 * \retval  paddr        Phys. address
 * \retval  psize        Size of phys. contiguous dataspace region at offset
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC         IPC error calling dataspace manager
 *          - -#L4_EINVAL       invalid dataspace id
 *          - -#L4_EPERM        caller is not a client of the dataspace
 *          - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 * 
 * Return the phys. address of the dataspace memory at \a offset. The 
 * dataspace manager will return the size of the contiguous memory starting 
 * at \a offset in \a psize (\a psize is set to \a size if the area is 
 * larger than \a size).
 * \note The dataspace region must be locked (using l4dm_mem_ds_lock()) 
 *       or must be allocated on pinned memory, otherwise it is not safe 
 *       to use the returned phys. address (the dataspace region might 
 *       get paged in the meantime).
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_phys_addr(l4dm_dataspace_t * ds, 
		      l4_offs_t offset, 
		      l4_size_t size,
		      l4_addr_t * paddr, 
		      l4_size_t * psize);

/*****************************************************************************/
/**
 * \brief   Get phys. address of a of VM region
 * \ingroup api_mem
 * 
 * \param   ptr          VM region address
 * \param   size         VM region size
 * \param   addrs        Phys. address regions destination buffer
 * \param   num          Number of elements in \a addrs.
 * \retval  psize        Total size of the address regions in \a addrs
 *	
 * \return  On success number of found phys. address regions, 
 *          error code otherwise:
 *          - -#L4_ENOTFOUND  dataspace not found for a VM address
 *          - -#L4_EPERM      caller is not a client of the dataspace
 *          - -#L4_EIPC       error calling region mapper / dataspace manager
 *         
 * Lookup the phys. addresses for the specified virtual memory region. If 
 * several dataspaces are attached to the VM region or the dataspaces are 
 * not phys. contiguous, the different addresses are returned in \a addrs
 * (up to \a num entries). \a psize will contain the total size of the 
 * areas described in \a addrs. Like in l4dm_mem_ds_phys_addr(), the VM 
 * area must be locked.
 */
/*****************************************************************************/ 
int
l4dm_mem_phys_addr(void * ptr, 
		   l4_size_t size, 
		   l4dm_mem_addr_t addrs[], 
		   int num, 
		   l4_size_t * psize);

/*****************************************************************************/
/**
 * \brief   Test if dataspace is allocated on contiguous memory
 * \ingroup api_mem
 * 
 * \param   ds            Dataspace id
 *	
 * \return  1 if dataspace is allocated on contiguous memory, 0 if not.
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_is_contiguous(l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Lock dataspace region
 * \ingroup api_mem
 * 
 * \param   ds           Dataspace id
 * \param   offset       Offset in dataspace
 * \param   size         Region size, #L4DM_WHOLE_DS to lock the whole 
 *                       dataspace starting at \a offset
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC         IPC error calling dataspace manager
 *          - -#L4_EPERM        caller is not a client of the dataspace
 *          - -#L4_EINVAL       invalid dataspace id
 *          - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 *
 * Lock ("pin") the dataspace region. This ensures that the dataspace region
 * will not be unmapped by the dataspace manager.
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_lock(l4dm_dataspace_t * ds, 
		 l4_offs_t offset, 
		 l4_size_t size);

/*****************************************************************************/
/**
 * \brief   Unlock dataspace region
 * \ingroup api_mem
 * 
 * \param   ds           Dataspace id
 * \param   offset       Offset in dataspace
 * \param   size         Region size, #L4DM_WHOLE_DS to unlock the whole 
 *                       dataspace starting at \a offset
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC         IPC error calling dataspace manager
 *          - -#L4_EPERM        caller is not a client of the dataspace
 *          - -#L4_EINVAL       invalid dataspace id
 *          - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_unlock(l4dm_dataspace_t * ds, 
		   l4_offs_t offset, 
		   l4_size_t size);

/*****************************************************************************/
/**
 * \brief   Lock virtual memory region
 * \ingroup api_mem
 * 
 * \param   ptr          VM region start address
 * \param   size         VM region size
 *	
 * \return  0 on success (locked VM region), error code otherwise:
 *          - -#L4_ENOTFOUND  dataspace not found for a VM address
 *          - -#L4_EIPC       error calling region mapper / dataspace manager
 *          - -#L4_EPERM      caller is not a client of a dataspace attached
 *                             to the VM region
 * 
 * Lock ("pin") the VM region. This ensures that the dataspaces attached to
 * this region will not be unmapped by the dataspace managers.
 */
/*****************************************************************************/ 
int
l4dm_mem_lock(void * ptr, 
	      l4_size_t size);

/*****************************************************************************/
/**
 * \brief   Unlock virtual memory region
 * \ingroup api_mem
 * 
 * \param   ptr          VM region start address
 * \param   size         VM region size
 *	
 * \return  0 on success (unlocked VM region), error code otherwise:
 *          - -#L4_ENOTFOUND  dataspace not found for a VM address
 *          - -#L4_EIPC       error calling region mapper / dataspace manager
 *          - -#L4_EPERM      caller is not a client of a dataspace attached
 *                             to the VM region
 */
/*****************************************************************************/ 
int
l4dm_mem_unlock(void * ptr, 
		l4_size_t size);

/*****************************************************************************/
/**
 * \brief   Allocate memory
 * \ingroup api_alloc
 * 
 * \param   size         Memory size
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                             memory
 *                       - #L4DM_PINNED        allocate pinned ("locked")
 *                                             memory
 *                       - #L4RM_MAP           map memory immediately
 *                       - #L4RM_LOG2_ALIGNED  find a \f$2^{log_2(size)+1}\f$ 
 *                                             aligned VM region 
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             \f$2^{log_2(size)+1}\f$ sized 
 *                                             VM region
 *	
 * \return Pointer to allocated memory, #NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager and attach to the
 * address space. The access rights will be set to #L4DM_RW. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_allocate(l4_size_t size, 
		  l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Allocate memory, name dataspace
 * \ingroup api_alloc
 * 
 * \param   size         Memory size
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                             memory
 *                       - #L4DM_PINNED        allocate pinned ("locked")
 *                                             memory
 *                       - #L4RM_MAP           map memory immediately
 *                       - #L4RM_LOG2_ALIGNED  find a \f$2^{log_2(size)+1}\f$ 
 *                                             aligned VM region 
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             \f$2^{log_2(size)+1}\f$ sized 
 *                                             VM region
 * \param   name         Dataspace name
 *	
 * \return Pointer to allocated memory, #NULL if allocation failed.
 *
 * Allocate named memory at the default dataspace manager and attach to the
 * address space. The access rights will be set to #L4DM_RW. 
 */
/*****************************************************************************/ 
void *
l4dm_mem_allocate_named(l4_size_t size, 
			l4_uint32_t flags, 
			const char * name);

/*****************************************************************************/
/** 
 * \brief   Allocate memory
 * \ingroup api_alloc
 * 
 * \param   size         Memory size
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                             memory
 *                       - #L4DM_PINNED        allocate pinned ("locked")
 *                                             memory
 *                       - #L4RM_MAP           map memory immediately
 *                       - #L4RM_LOG2_ALIGNED  find a \f$2^{log_2(size)+1}\f$ 
 *                                             aligned VM region 
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             \f$2^{log_2(size)+1}\f$ sized 
 *                                             VM region
 * \retval  ds           Dataspace descriptor
 *	
 * \return Pointer to allocated memory, #NULL if allocation failed.
 *
 * Allocate memory at the default dataspace manager, attach to the
 * address space and return dataspace id. The access rights will be set 
 * to #L4DM_RW.
 */
/*****************************************************************************/ 
void *
l4dm_mem_ds_allocate(l4_size_t size, 
		     l4_uint32_t flags, 
		     l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Allocate memory, name dataspace
 * \ingroup api_alloc
 * 
 * \param   size         Memory size
 * \param   flags        Flags:
 *                       - #L4DM_CONTIGUOUS    allocate phys. contiguous
 *                                             memory
 *                       - #L4DM_PINNED        allocate pinned ("locked")
 *                                             memory
 *                       - #L4RM_MAP           map memory immediately
 *                       - #L4RM_LOG2_ALIGNED  find a \f$2^{log_2(size)+1}\f$ 
 *                                             aligned VM region 
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             \f$2^{log_2(size)+1}\f$ sized 
 *                                             VM region
 * \param   name         Dataspace name
 * \retval  ds           Dataspace descriptor
 *	
 * \return Pointer to allocated memory, #NULL if allocation failed.
 *
 * Allocate named memory at the default dataspace manager and attach to the
 * address space and return dataspace id. The access rights will be set 
 * to #L4DM_RW.
 */
/*****************************************************************************/ 
void *
l4dm_mem_ds_allocate_named(l4_size_t size, 
			   l4_uint32_t flags, 
			   const char * name,
			   l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Release memory
 * \ingroup api_alloc
 * 
 * \param   ptr          Memory area address
 *
 * Release memory attached to \a ptr.
 */
/*****************************************************************************/ 
void
l4dm_mem_release(void * ptr);

__END_DECLS;

#endif /* !_DM_MEM_DM_MEM_H */
