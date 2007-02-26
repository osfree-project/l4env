/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/include/l4/l4rm/l4rm.h
 * \brief  L4 region mapper public interface.
 *
 * \date   06/01/2000
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
#ifndef _L4_L4RM_H
#define _L4_L4RM_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/dm_generic/dm_generic.h>

/*****************************************************************************
 *** defines
 *****************************************************************************/

/*****************************************************************************
 *** Flags / Access rights, the bit mask consists of several parts:
 ***
 ***   31       23       15        7       0
 ***   +--------+--------+--------+--------+
 ***   | flags  | DMphys | DMgen  | rights |
 ***   +--------+--------+--------+--------+
 ***
 *** flags    Attach / reserve flags
 *** rights   Attach access rights (defined in l4/dm_generic/consts.h)
 *** DMgen    Flags used by DMgeneric (l4/dm_generic/consts.h)
 *** DMphys   Flags used by DMphys (l4/dm_phys/consts.h)
 *****************************************************************************/

/* attach flags, more internal flags defined in lib/include/__region.h */
#define L4RM_MAP           0x01000000  /**< \ingroup api_attach
				        **  Immediately map attached region
					**/
#define L4RM_LOG2_ALIGNED  0x02000000  /**< \ingroup api_attach
					**  Align to 
					**  \f$2^{(log_2(size) + 1)}\f$ 
					**  address
					**/
#define L4RM_LOG2_ALLOC    0x04000000  /**< \ingroup api_attach
					**  Allocate whole
					**  \f$2^{(log_2(size) + 1)}\f$ 
					**  sized region
					**/
#define L4RM_RESERVE_USED  0x08000000  /**< \ingroup api_attach
					**  Mark reserved area as used
					**/

/*****************************************************************************
 *** types
 *****************************************************************************/

/**
 * VM address range
 * \ingroup api_init
 */
typedef struct l4rm_vm_range
{
  l4_addr_t addr;   ///< VM area start address
  l4_size_t size;   ///< VM area size
} l4rm_vm_range_t;

/*****************************************************************************
 *** global configuration data
 *****************************************************************************/

/**
 * L4RM heap map address 
 * \ingroup api_init
 */
extern const l4_addr_t l4rm_heap_start_addr;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************
 *** Initialization
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Init Region Mapper.
 * \ingroup api_init
 *
 * \param   have_l4env   Set to != 0 if started with the L4 environment 
 *                       (L4RM then uses the default dataspace manager to 
 *                       allocate its descriptor heap)
 * \param   used         Used VM address range, do not use for internal data.
 *                       These address ranges are only excluded during 
 *                       l4rm_init, if an address range should further not be
 *                       used, it also has to be reserved 
 *                       (l4rm_area_reserve()) after L4RM is initialized.
 * \param   num_used     Number of elements in \a used.
 *
 * \return  0 on success, -1 if initialization failed 
 */
/*****************************************************************************/ 
int 
l4rm_init(int have_l4env, 
	  l4rm_vm_range_t used[], 
	  int num_used);

/*****************************************************************************/
/**
 * \brief   Return service id of region mapper thread
 * \ingroup api_init
 *	
 * \return  Id of region mapper thread.
 */
/*****************************************************************************/ 
l4_threadid_t
l4rm_region_mapper_id(void);

/*****************************************************************************/
/**
 * \brief   Region mapper service loop.
 * \ingroup api_init
 *
 * Used in l4env_startup (l4env).
 */
/*****************************************************************************/ 
void
l4rm_service_loop(void);

/*****************************************************************************
 *** attach / detach
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Attach dataspace.
 * \ingroup api_attach
 *
 * \param   ds           Dataspace id
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags:
 *                       - #L4DM_RO           attach read-only
 *                       - #L4DM_RW           attach read/write
 *                       - #L4RM_LOG2_ALIGNED find a 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            aligned region
 *                       - #L4RM_LOG2_ALLOC   allocate the whole 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            sized area
 *                       - #L4RM_MAP          immediately map attached dataspace 
 *                                            area
 * \retval  addr          Start address
 *	
 * \return  0 on success (dataspace attached to region at address \a addr), 
 *          error code otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 *          - -#L4_EIPC    error calling region mapper
 * 
 * Find an unused map region and attach dataspace area 
 * (\a ds_offs, \a ds_offs + \a size) to that region.
 */
/*****************************************************************************/ 
int
l4rm_attach(l4dm_dataspace_t * ds, 
	    l4_size_t size, 
	    l4_offs_t ds_offs,
	    l4_uint32_t flags, 
	    void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region.
 * \ingroup api_attach
 * 
 * \param   ds           Dataspace id
 * \param   addr         Start address
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags:
 *                       - #L4DM_RO   attach read-only
 *                       - #L4DM_RW   attach read/write
 *                       - #L4RM_MAP  immediately map attached dataspace area
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EUSED   region already used
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 *          - -#L4_EIPC    error calling region mapper
 *         
 * Attach dataspace to region at \a addr.
 */
/*****************************************************************************/ 
int
l4rm_attach_to_region(l4dm_dataspace_t * ds, 
		      void * addr, 
		      l4_size_t size,
		      l4_offs_t ds_offs, 
		      l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to area.
 * \ingroup api_attach
 *  
 * \param   ds           Dataspace id
 * \param   area         Area id
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags:
 *                       - #L4DM_RO           attach read-only
 *                       - #L4DM_RW           attach read/write
 *                       - #L4RM_LOG2_ALIGNED find a 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            aligned region
 *                       - #L4RM_LOG2_ALLOC   allocate the whole 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            sized area
 *                       - #L4RM_MAP          immediately map attached dataspace
 *                                            area
 * \retval addr          Start address 
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 *          - -#L4_EIPC    error calling region mapper
 * 
 * Attach dataspace to area \a area. An area is a region in the address space
 * reserved by l4rm_area_reserve().
 */
/*****************************************************************************/ 
int 
l4rm_area_attach(l4dm_dataspace_t * ds, 
		 l4_uint32_t area, 
		 l4_size_t size,
		 l4_offs_t ds_offs, 
		 l4_uint32_t flags,
		 void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region in area.
 * \ingroup api_attach
 * 
 * \param   ds           Dataspace id
 * \param   area         Area id
 * \param   addr         Start address
 * \param   size         Size
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags:
 *                       - #L4DM_RO   attach read-only
 *                       - #L4DM_RW   attach read/write
 *                       - #L4RM_MAP  immediately map attached dataspace area
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EUSED   region already used
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 *          - -#L4_EIPC    error calling region mapper
 *
 * Attach dataspace to region at \a addr in area \a area.
 */
/*****************************************************************************/ 
int 
l4rm_area_attach_to_region(l4dm_dataspace_t * ds, 
			   l4_uint32_t area, 
			   void * addr, 
			   l4_size_t size, 
			   l4_offs_t ds_offs, 
			   l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Detach dataspace.
 * \ingroup api_attach
 * 
 * \param   addr         Address of VM area
 *	
 * \return  0 on success (dataspace detached from region \a id), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid region id
 *          - -#L4_EIPC    error calling region mapper
 *
 * Detach dataspace which is attached to address \a addr.
 */
/*****************************************************************************/ 
int
l4rm_detach(void * addr);

/*****************************************************************************/
/**
 * \brief   Lookup address.
 * \ingroup api_attach
 * 
 * \param   addr         Address
 * \retval  ds           Dataspace
 * \retval  offset       Offset of ptr in dataspace
 * \retval  map_addr     Map area start address
 * \retval  map_size     Map area size
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_ENOTFOUND  dataspace not found for address
 *          - -#L4_EIPC       error calling region mapper
 *         
 * Return the dataspace and region which is attached to \a addr.
 */
/*****************************************************************************/ 
int
l4rm_lookup(void * addr, 
	    l4dm_dataspace_t * ds, 
	    l4_offs_t * offset, 
	    l4_addr_t * map_addr, 
	    l4_size_t * map_size);

/*****************************************************************************
 *** region mapper client
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Add new region mapper client
 * \ingroup api_client
 *
 * \param   client       Client thread id
 *	
 * \return  0 on success (added client), error code otherwise:
 *          - -#L4_EIPC  calling region mapper thread failed
 */
/*****************************************************************************/ 
int
l4rm_add_client(l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Remove region mapper client
 * \ingroup api_client
 *
 * \param   client       Client thread id
 *	
 * \return  0 on success (remove client), error code otherwise:
 *          - -#L4_EIPC  calling region mapper thread failed
 */
/*****************************************************************************/ 
int
l4rm_remove_client(l4_threadid_t client);

/*****************************************************************************
 *** regions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Reserve area.
 * \ingroup api_vm
 *
 * \param   size         Area size
 * \param   flags        Flags:
 *                       - #L4RM_LOG2_ALIGNED find a 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            aligned region
 *                       - #L4RM_LOG2_ALLOC   allocate the whole 
 *                                            \f$2^{(log_2(size) + 1)}\f$ 
 *                                            sized region
 *                       - #L4RM_RESERVE_USED mark reserved area as used
 * \retval  addr         Start address 
 * \retval  area         Area id
 *	
 * \return  0 on success (reserved area at \a addr), error code otherwise:
 *          - -#L4_ENOTFOUND  no free area of size \a size found
 *          - -#L4_ENOMEM     out of memory allocating descriptors
 *          - -#L4_EIPC       error calling region mapper
 *
 * Reserve area of size \a size. The reserved area will not be used 
 * in l4rm_attach or l4rm_attach_to_region, dataspace can only be attached 
 * to this area calling l4rm_area_attach or l4rm_area_attach_to region with 
 * the appropriate area id. 
 */
/*****************************************************************************/ 
int
l4rm_area_reserve(l4_size_t size, 
		  l4_uint32_t flags, 
		  l4_addr_t * addr, 
		  l4_uint32_t * area);

/*****************************************************************************/
/**
 * \brief   Reserve specified area.
 * \ingroup api_vm
 *
 * \param   addr         Address
 * \param   size         Area Size
 * \param   flags        Flags:
 *                       - #L4RM_RESERVE_USED mark reserved area as used
 * \retval  area         Area id
 *	
 * \return  0 on success (reserved area at \a addr), error code otherwise:
 *          - -#L4_EUSED   specified are aalready used
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_EIPC    error calling region mapper
 *
 * Reserve area at \a addr.
 */
/*****************************************************************************/ 
int
l4rm_area_reserve_region(l4_addr_t addr, 
			 l4_size_t size, 
			 l4_uint32_t flags,
			 l4_uint32_t * area);

/*****************************************************************************/
/**
 * \brief   Release area.
 * \ingroup api_vm
 *
 * \param   area         Area id
 *	
 * \return  0 on success (area released), error code otherwise:
 *          - -#L4_EIPC  error calling region mapper.
 *
 * Release area.
 */
/*****************************************************************************/
int
l4rm_area_release(l4_uint32_t area);

/*****************************************************************************/
/**
 * \brief   Release area at given address
 * \ingroup api_vm
 *
 * \param   ptr          VM address
 *	
 * \return  0 on success (area released), error code otherwise:
 *          - -#L4_EINVAL  invalid address, address belongs not to a reserved 
 *                         area
 */
/*****************************************************************************/ 
int
l4rm_area_release_addr(void * ptr);

/*****************************************************************************
 * Modify the region list / region tree directly without locking the region
 * list or calling the region mapper thread. This is necessary during the 
 * task startup where the other environment libraries (thread/lock) are not
 * yet initialized.
 * DO NOT USE AT OTHER PLACES!
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Attach dataspace.
 * \ingroup api_setup
 * 
 * \param   ds           Dataspace id
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags
 * \retval  addr         Start address
 *	
 * \return  0 on success (dataspace attached to region at address \a addr), 
 *          error code otherwise:
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/ 
int
l4rm_direct_attach(l4dm_dataspace_t * ds, 
		   l4_size_t size, 
		   l4_offs_t ds_offs, 
		   l4_uint32_t flags, 
		   void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region.
 * \ingroup api_setup
 *
 * \param   ds           Dataspace id
 * \param   addr         Start address
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid or used region
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/ 
int 
l4rm_direct_attach_to_region(l4dm_dataspace_t * ds, 
			     void * addr, 
			     l4_size_t size, 
			     l4_offs_t ds_offs, 
			     l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to area.
 * \ingroup api_setup
 * 
 * \param   ds           Dataspace id
 * \param   area         Area id
 * \param   size         Size 
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags
 * \retval  addr         Start address 
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid area id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/ 
int
l4rm_direct_area_attach(l4dm_dataspace_t * ds, 
			l4_uint32_t area, 
			l4_size_t size,
			l4_offs_t ds_offs, 
			l4_uint32_t flags, 
			void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region in area.
 * \ingroup api_setup
 *
 * \param   ds            Dataspace id
 * \param   area          Area id
 * \param   addr          Start address
 * \param   size          Size
 * \param   ds_offs       Offset in dataspace
 * \param   flags         Flags
 *	
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid map region or area id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/ 
int 
l4rm_direct_area_attach_to_region(l4dm_dataspace_t * ds, 
				  l4_uint32_t area,
				  void * addr, 
				  l4_size_t size, 
				  l4_offs_t ds_offs, 
				  l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Add new client without calling region mapper thread
 * \ingroup api_setup
 *
 * \param   client       Client thread id
 *	
 * \return  0 on success (added client)
 */
/*****************************************************************************/ 
int
l4rm_direct_add_client(l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Reserve area.
 * \ingroup api_setup
 * 
 * \param   size         Area size
 * \param   flags        Flags
 * \retval  addr         Start address 
 * \retval  area         Area id
 *      
 * \return  0 on success (reserved area at \a addr), error code otherwise:
 *          - -#L4_ENOTFOUND  no free area of size \a size found
 *          - -#L4_ENOMEM     out of memory allocating descriptors
 */
/*****************************************************************************/ 
int
l4rm_direct_area_reserve(l4_size_t size, 
			 l4_uint32_t flags, 
                         l4_addr_t * addr, 
			 l4_uint32_t * area);

/*****************************************************************************/
/**
 * \brief   Reserve specified area.
 * \ingroup api_setup
 * 
 * \param   addr         Address
 * \param   size         Area Size
 * \param   flags        Flags
 * \retval  area         Area id
 *      
 * \return  0 on success (reserved area at \a addr), error code otherwise:
 *          - -#L4_EUSED   specified are aalready used
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 */
/*****************************************************************************/ 
int
l4rm_direct_area_reserve_region(l4_addr_t addr, 
				l4_size_t size, 
                                l4_uint32_t flags, 
				l4_uint32_t * area);

/*****************************************************************************
 *** DEBUG
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Show a list of all regions (debug).
 * \ingroup api_debug
 */
/*****************************************************************************/ 
void
l4rm_show_region_list(void);

__END_DECLS;

#endif /* !_L4_L4RM_H */
