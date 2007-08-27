/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/include/l4/l4rm/l4rm.h
 * \brief  L4 region mapper public interface.
 *
 * \date   06/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_L4RM_H
#define _L4_L4RM_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>
#include <l4/sys/utcb.h>
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
#define L4RM_SUPERPAGE_ALIGNED 0x08000000 /**< \ingroup api_attach
					   ** Align to superpage size
					   **/

/* region types for l4rm_lookup_region */
#define L4RM_REGION_FREE            1  /**< \ingroup api_vm
                                        **  Free region
                                        **/
#define L4RM_REGION_RESERVED        2  /**< \ingroup api_vm
                                        **  Reserved region
                                        **/
#define L4RM_REGION_DATASPACE       3  /**< \ingroup api_vm
                                        **  Region with dataspace
                                        **  attached to it
                                        **/
#define L4RM_REGION_PAGER           4  /**< \ingroup api_vm
                                        **  Region with external pager
                                        **/
#define L4RM_REGION_EXCEPTION       5  /**< \ingroup api_vm
                                        **  Region in which pagefaults are
                                        **  forwared as exceptions
                                        **/
#define L4RM_REGION_BLOCKED         6  /**< \ingroup api_vm
                                        **  Blocked (unavailable) region
                                        **/
#define L4RM_REGION_UNKNOWN         7  /**< \ingroup api_vm
                                        **  Unknown region type
                                        **/

/**
 * \ingroup api_vm
 * Default region area id
 */
#define L4RM_DEFAULT_REGION_AREA    0

/* pagefault callback return types */
#define L4RM_REPLY_EMPTY      0   /**< \ingroup api_vm
                                   **  Reply with empty message
                                   **/
#define L4RM_REPLY_EXCEPTION  1   /**< \ingroup api_vm
                                   **  Forward exception to source thread
                                   **/
#define L4RM_REPLY_NO_REPLY   2   /**< \ingroup api_vm
                                   **  Send no reply
                                   **/
#define L4RM_REPLY_SUCCESS    3   /**< \ingroup api_vm
                                   **  Successfully handled request
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

/**
 * Pagefault callback function prototype
 * \ingroup api_vm
 *
 * \param  addr          Pagefault address
 * \param  eip           Pagefault instruction pointer
 * \param  src           Pagefault source thread
 *
 * \return The callback function must return what type of reply the region
 *         mapper should send back to the source thread:
 *         - #L4RM_REPLY_EMPTY      Send an empty reply message, this should be
 *                                  used if the callback function could resolve
 *                                  the pagefault
 *         - #L4RM_REPLY_EXCEPTION  Forward a pagefault exception to the
 *                                  source thread, the thread should have
 *                                  registered an appropriate exception handler
 *         - #L4RM_REPLY_NO_REPLY   Send no reply, this blocks the source thread
 */
typedef int (*l4rm_pf_callback_fn_t)(l4_addr_t addr, l4_addr_t eip,
                                     l4_threadid_t src);

/**
 * Callback function type for unknown faults
 * \ingroup api_vm
 *
 * \param tag         IPC msgtag
 * \param utcb        UTCB pointer of the fault receiver
 * \param src         Fault source thread
 *
 * \return The callback function must return what type of reply the region
 *         mapper should send back to the source thread:
 *         - #L4RM_REPLY_EMPTY      Send an empty reply message, this should be
 *                                  used if the callback function could resolve
 *                                  the pagefault
 *         - #L4RM_REPLY_EXCEPTION  Forward a pagefault exception to the
 *                                  source thread, the thread should have
 *                                  registered an appropriate exception handler
 *         - #L4RM_REPLY_NO_REPLY   Send no reply, this blocks the source thread
 */
typedef int (*l4rm_unknown_fault_callback_fn_t)(l4_msgtag_t tag,
                                                l4_utcb_t *utcb,
                                                l4_threadid_t src);

/*****************************************************************************
 *** global configuration data
 *****************************************************************************/

/**
 * L4RM heap map address
 * \ingroup api_init
 */
extern const l4_addr_t l4rm_heap_start_addr;

/*****************************************************************************
 *** internal defines
 *****************************************************************************/

/**
 * \internal
 * Find suitable address for attach / reserve
 */
#define L4RM_ADDR_FIND     0xFFFFFFFF


/**
 * \internal
 * Do not lock region list / do not call region mapper thread
 */
#define L4RM_MODIFY_DIRECT 0x20000000

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
l4rm_init(int have_l4env, l4rm_vm_range_t used[], int num_used);

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
l4rm_service_loop(void) __attribute__((noreturn));

/*****************************************************************************
 *** attach / detach dataspaces
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
 *                       - #L4RM_SUPERPAGE_ALIGNED find a
 *                                            superpage aligned region
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
L4_INLINE int
l4rm_attach(const l4dm_dataspace_t * ds, l4_size_t size, l4_offs_t ds_offs,
	    l4_uint32_t flags, void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region.
 * \ingroup api_attach
 *
 * \param   ds           Dataspace id
 * \param   addr         Start address, must be page aligned
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
L4_INLINE int
l4rm_attach_to_region(const l4dm_dataspace_t * ds, const void * addr,
                      l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags);

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
 *                       - #L4RM_SUPERPAGE_ALIGNED find a
 *                                            superpage aligned region
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
L4_INLINE int
l4rm_area_attach(const l4dm_dataspace_t * ds, l4_uint32_t area, l4_size_t size,
		 l4_offs_t ds_offs, l4_uint32_t flags, void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region in area.
 * \ingroup api_attach
 *
 * \param   ds           Dataspace id
 * \param   area         Area id
 * \param   addr         Start address, must be page aligned
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
L4_INLINE int
l4rm_area_attach_to_region(const l4dm_dataspace_t * ds, l4_uint32_t area,
			   const void * addr, l4_size_t size,
			   l4_offs_t ds_offs, l4_uint32_t flags);

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
l4rm_detach(const void * addr);

/*****************************************************************************
 *** pager / exception / blocked regions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Setup VM area
 * \ingroup api_vm
 *
 * \param   size         Region size
 * \param   area         Area id, set to #L4RM_DEFAULT_REGION_AREA to use
 *                       default area (i.e. an area not reserved)
 * \param   type         Region type:
 *                       - #L4RM_REGION_PAGER      region with external pager,
 *                                                 \a pager must contain the id
 *                                                 of the external pager
 *                       - #L4RM_REGION_EXCEPTION  region with exception forward
 *                       - #L4RM_REGION_BLOCKED    blocked (unavailable) region
 * \param   flags        Flags:
 *                       - #L4RM_LOG2_ALIGNED
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED
 *                         reserve a superpage aligned region
 *                       - #L4RM_LOG2_ALLOC
 *                         reserve the whole 2^(log2(size) + 1) sized region
 * \param   pager        External pager (if type is #L4RM_REGION_PAGER), if set
 *                       to L4_INVALID_ID, the pager of the region mapper thread
 *                       is used
 * \retval  addr         Region start address
 *
 * \return  0 on success (setup region), error code otherwise:
 *          - -#L4_ENOMAP     no region found
 *          - -#L4_ENOMEM     out of memory allocation region descriptor
 *          - -#L4_EINVAL     invalid area / type
 *          - -#L4_EIPC       error calling region mapper
 */
/*****************************************************************************/
L4_INLINE int
l4rm_area_setup(l4_size_t size, l4_uint32_t area, int type, l4_uint32_t flags,
                l4_threadid_t pager, l4_addr_t * addr);

/*****************************************************************************/
/**
 * \brief   Setup VM area
 * \ingroup api_vm
 *
 * \param   addr         Region start address
 * \param   size         Region size
 * \param   area         Area id, set to #L4RM_DEFAULT_REGION_AREA to use
 *                       default area (i.e. an area not reserved)
 * \param   type         Region type:
 *                       - #L4RM_REGION_PAGER      region with external pager,
 *                                                 \a pager must contain the id
 *                                                 of the external pager
 *                       - #L4RM_REGION_EXCEPTION  region with exception forward
 *                       - #L4RM_REGION_BLOCKED    blocked (unavailable) region
 * \param   flags        Flags:
 *                       - #L4RM_LOG2_ALIGNED
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED
 *                         reserve a superpage aligned region
 *                       - #L4RM_LOG2_ALLOC
 *                         reserve the whole 2^(log2(size) + 1) sized region
 * \param   pager        External pager (if type is #L4RM_REGION_PAGER), if set
 *                       to L4_INVALID_ID, the pager of the region mapper thread
 *                       is used
 *
 * \return  0 on success (setup region), error code otherwise:
 *          - -#L4_ENOMEM     out of memory allocation region descriptor
 *          - -#L4_EUSED      address region already used
 *          - -#L4_EINVAL     invalid area / type
 *          - -#L4_EIPC       error calling region mapper
 */
/*****************************************************************************/
L4_INLINE int
l4rm_area_setup_region(l4_addr_t addr, l4_size_t size, l4_uint32_t area,
                       int type, l4_uint32_t flags, l4_threadid_t pager);

/*****************************************************************************/
/**
 * \brief   Clear VM area
 * \ingroup api_vm
 *
 * \param   addr         Address of VM area
 *
 * \return  0 on success, error code otherwise
 *          - -#L4_ENOTFOUND  no region found at address \a addr
 *          - -#L4_EINVAL     invalid region type
 *          - -#L4_EIPC       IPC error calling region mapper thread
 *
 * Clear region which was set up with l4rm_area_setup_region().
 */
/*****************************************************************************/
int
l4rm_area_clear_region(l4_addr_t addr);

/*****************************************************************************
 *** reserve vm areas
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
 *                       - #L4RM_SUPERPAGE_ALIGNED find a
 *                                            superpage aligned region
 *                       - #L4RM_LOG2_ALLOC   allocate the whole
 *                                            \f$2^{(log_2(size) + 1)}\f$
 *                                            sized region
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
L4_INLINE int
l4rm_area_reserve(l4_size_t size, l4_uint32_t flags,
		  l4_addr_t * addr, l4_uint32_t * area);

/*****************************************************************************/
/**
 * \brief   Reserve specified area.
 * \ingroup api_vm
 *
 * \param   addr         Address
 * \param   size         Area Size
 * \param   flags        Flags:
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
L4_INLINE int
l4rm_area_reserve_region(l4_addr_t addr, l4_size_t size,
			 l4_uint32_t flags, l4_uint32_t * area);

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
l4rm_area_release_addr(const void * ptr);

/*****************************************************************************/
/**
 * \brief   Store a user-defined pointer for region
 * \ingroup api_vm
 *
 * \param   addr  addr inside region
 * \param   ptr   user pointer
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid address, address belongs to no used region
 */
/*****************************************************************************/
int
l4rm_set_userptr(const void * addr, void * ptr);

/*****************************************************************************/
/**
 * \brief   Read user-defined pointer for region
 * \ingroup api_vm
 *
 * \param   addr  addr inside region
 *
 * \return  stored user pointer or 0 if none
 */
/*****************************************************************************/
void *
l4rm_get_userptr(const void * addr);


/*****************************************************************************
 *** lookup regions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Lookup address.
 * \ingroup api_vm
 *
 * \param   addr         Address
 * \retval  map_addr     Map area start address
 * \retval  map_size     Map area size
 * \retval  ds           Dataspace
 * \retval  offset       Offset of ptr in dataspace
 * \retval  pager        External pager
 *
 * \return  Region type on success (> 0):
 *          - #L4RM_REGION_DATASPACE    region with attached dataspace, \a ds
 *                                      and \a offset contain the dataspace id
 *                                      and the map offset in the dataspace
 *          - #L4RM_REGION_PAGER        region with external pager, \a pager
 *                                      contains the id of the external pager
 *          - #L4RM_REGION_EXCEPTION    region with exception forward
 *          - #L4RM_REGION_BLOCKED      blocked (unavailable) region
 *          - #L4RM_REGION_UNKNOWN      unknown region
 *          Error codes (< 0):
 *          - -#L4_ENOTFOUND  dataspace not found for address
 *
 * Return the region type of the region at address \a addr. l4rm_lookup()
 * only returns successfully if the region at \a addr is really used (i.e.
 * a dataspace was attached with l4rm_attach* or the region was set up with
 * l4rm_area_setup*). If the region is not used (either really not used or
 * only reserved using l4rm_reserve*), l4rm_lookup() returns -#L4_ENOTFOUND.
 * To get also a result for free / reserved regions, use l4rm_lookup_region()
 * instead of l4rm_lookup().
 *
 * Note that l4rm_lookup_region() is slower than l4rm_lookup(), you should
 * prefer l4rm_lookup() as long as you do not need to distinguish between free
 * and reserved regions.
 */
/*****************************************************************************/
int
l4rm_lookup(const void * addr, l4_addr_t * map_addr, l4_size_t * map_size,
            l4dm_dataspace_t * ds, l4_offs_t * offset, l4_threadid_t * pager);

/*****************************************************************************/
/**
 * \brief   Lookup address, return size and type of region
 * \ingroup api_vm
 *
 * \param   addr         Address
 * \retval  map_addr     Start address of region
 * \retval  map_size     Size of whole region, beginning at \a map_addr
 * \retval  ds           Dataspace
 * \retval  offset       Offset of ptr in dataspace
 * \retval  pager        External pager
 *
 * \return  Region type on success (> 0):
 *          - #L4RM_REGION_FREE         free region
 *          - #L4RM_REGION_RESERVED     reserved region
 *          - #L4RM_REGION_DATASPACE    region with attached dataspace
 *          - #L4RM_REGION_PAGER        region with external pager
 *          - #L4RM_REGION_EXCEPTION    region with exception forward
 *          - #L4RM_REGION_BLOCKED      blocked (unavailable) region
 *          - #L4RM_REGION_UNKNOWN      unknown region
 *          Error codes (< 0):
 *          - -#L4_ENOTFOUND  no region found at given address
 *
 * See l4rm_lookup() for comments.
 */
/*****************************************************************************/
int
l4rm_lookup_region(const void * addr, l4_addr_t * map_addr,
                   l4_size_t * map_size, l4dm_dataspace_t * ds,
                   l4_offs_t * offset, l4_threadid_t * pager);

/*****************************************************************************
 *** advanced pagefault handling
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Set callback function for unkown pagefaults
 * \ingroup api_vm
 *
 * \param   callback     Callback function, set to #NULL to remove callback
 */
/*****************************************************************************/
void
l4rm_set_unkown_pagefault_callback(l4rm_pf_callback_fn_t callback);

/*****************************************************************************/
/**
 * \brief   Set callback function for unkown faults
 * \ingroup api_vm
 *
 * \param   callback     Callback function, set to #NULL to remove callback
 */
/*****************************************************************************/
void
l4rm_set_unkown_fault_callback(l4rm_unknown_fault_callback_fn_t callback);

/*****************************************************************************/
/**
 * \brief   Enable exception forward for unkown pagefaults
 * \ingroup api_vm
 */
/*****************************************************************************/
void
l4rm_enable_pagefault_exceptions(void);

/*****************************************************************************/
/**
 * \brief   Disable exception forward for unkown pagefaults
 * \ingroup api_vm
 */
/*****************************************************************************/
void
l4rm_disable_pagefault_exceptions(void);

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

/*****************************************************************************
 * Modify the region list / region tree directly without locking the region
 * list or calling the region mapper thread. This is necessary during the
 * task startup where the other environment libraries (thread/lock) are not
 * yet initialized.
 * DO NOT USE AT OTHER PLACES!
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region.
 * \ingroup api_setup
 *
 * \param   ds           Dataspace id
 * \param   addr         Address
 * \param   size         Size
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags
 *
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid area id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/
L4_INLINE int
l4rm_direct_attach_to_region(const l4dm_dataspace_t *ds, const void *addr,
			     l4_size_t size, l4_offs_t ds_offs,
			     l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to area.
 * \ingroup api_setup
 *
 * \param   ds           Dataspace id
 * \param   area         Area id, set to #L4RM_DEFAULT_REGION_AREA to use
 *                       default area (i.e. an area not reserved)
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
L4_INLINE int
l4rm_direct_area_attach(const l4dm_dataspace_t * ds, l4_uint32_t area,
                        l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags,
                        void ** addr);

/*****************************************************************************/
/**
 * \brief   Attach dataspace to specified region in area.
 * \ingroup api_setup
 *
 * \param   ds           Dataspace id
 * \param   area         Area id, set to #L4RM_DEFAULT_REGION_AREA to use
 *                       default area (i.e. an area not reserved)
 * \param   addr         Start address
 * \param   size         Size
 * \param   ds_offs      Offset in dataspace
 * \param   flags        Flags
 *
 * \return  0 on success (dataspace attached to region at \a addr), error code
 *          otherwise:
 *          - -#L4_EINVAL  invalid map region or area id
 *          - -#L4_ENOMEM  out of memory allocating descriptors
 *          - -#L4_ENOMAP  no region found
 */
/*****************************************************************************/
L4_INLINE int
l4rm_direct_area_attach_to_region(const l4dm_dataspace_t * ds, l4_uint32_t area,
				  const void * addr, l4_size_t size,
                                  l4_offs_t ds_offs, l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Setup VM area
 * \ingroup api_setop
 *
 * \param   size         Region size
 * \param   area         Area id, ser to #L4RM_DEFAULT_REGION_AREA to use
 *                       default area (i.e. an area not reserved)
 * \param   type         Region type:
 *                       - #L4RM_REGION_PAGER      region with external pager,
 *                                                 \a pager must contain the id
 *                                                 of the external pager
 *                       - #L4RM_REGION_EXCEPTION  region with exception forward
 *                       - #L4RM_REGION_BLOCKED    blocked (unavailable) region
 * \param   flags        Flags:
 *                       - #L4RM_LOG2_ALIGNED
 *                         reserve a 2^(log2(size) + 1) aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED
 *                         reserve a superpage aligned region
 *                       - #L4RM_LOG2_ALLOC
 *                         reserve the whole 2^(log2(size) + 1) sized region
 * \param   pager        External pager (if type is #L4RM_REGION_PAGER), if set
 *                       to L4_INVALID_ID, the pager of the region mapper thread
 *                       is used
 * \retval  addr         Region start address
 *
 * \return  0 on success (setup region), error code otherwise:
 *          - -#L4_ENOMAP     no region found
 *          - -#L4_ENOMEM     out of memory allocation region descriptor
 *          - -#L4_EINVAL     invalid area / type
 *          - -#L4_EIPC       error calling region mapper
 */
/*****************************************************************************/
L4_INLINE int
l4rm_direct_area_setup(l4_size_t size, l4_uint32_t area, int type,
                       l4_uint32_t flags, l4_threadid_t pager,
                       l4_addr_t * addr);

/*****************************************************************************/
/**
 * \brief   Setup VM area
 * \ingroup api_setup
 *
 * \param   addr         Region start address
 * \param   size         Region size
 * \param   area         Area id
 * \param   type         Region type:
 * \param   flags        Flags:
 * \param   pager        External pager (if type is #L4RM_REGION_PAGER)
 *
 * \return  0 on success (setup region), error code otherwise:
 *          - -#L4_ENOTFOUND  no suitable address area found
 *          - -#L4_ENOMEM     out of memory allocation region descriptor
 *          - -#L4_EUSED      address region already used
 *          - -#L4_EINVAL     invalid area / type
 */
/*****************************************************************************/
L4_INLINE int
l4rm_direct_area_setup_region(l4_addr_t addr, l4_size_t size, l4_uint32_t area,
                              int type, l4_uint32_t flags, l4_threadid_t pager);

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
L4_INLINE int
l4rm_direct_area_reserve(l4_size_t size, l4_uint32_t flags, l4_addr_t * addr,
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
L4_INLINE int
l4rm_direct_area_reserve_region(l4_addr_t addr, l4_size_t size,
                                l4_uint32_t flags, l4_uint32_t * area);

/*****************************************************************************
 *** internal
 *****************************************************************************/

/*****************************************************************************/
/**
 * \internal
 * \brief Really attach dataspace to region
 */
/*****************************************************************************/
int
l4rm_do_attach(const l4dm_dataspace_t * ds, l4_uint32_t area, l4_addr_t * addr,
               l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags);

/*****************************************************************************/
/**
 * \internal
 * \brief Really setup vm area
 */
/*****************************************************************************/
int
l4rm_do_area_setup(l4_addr_t * addr, l4_size_t size, l4_uint32_t area,
                   int type, l4_uint32_t flags, l4_threadid_t pager);

/*****************************************************************************/
/**
 * \internal
 * \brief Really mark address area reserved.
 */
/*****************************************************************************/
int
l4rm_do_reserve(l4_addr_t * addr, l4_size_t size, l4_uint32_t flags,
                l4_uint32_t * area);

__END_DECLS;

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

/*****************************************************************************
 *** l4rm_attach
 *****************************************************************************/
L4_INLINE int
l4rm_attach(const l4dm_dataspace_t * ds, l4_size_t size, l4_offs_t ds_offs,
	    l4_uint32_t flags, void ** addr)
{
  /* attach */
  *addr = (void *)L4RM_ADDR_FIND;
  return l4rm_do_attach(ds, L4RM_DEFAULT_REGION_AREA, (l4_addr_t *)addr,
                        size, ds_offs, flags);
}

/*****************************************************************************
 *** l4rm_attach_to_region
 *****************************************************************************/
L4_INLINE int
l4rm_attach_to_region(const l4dm_dataspace_t * ds, const void * addr,
                      l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags)
{
  /* attach */
  l4_addr_t _addr = (l4_addr_t)addr;
  return l4rm_do_attach(ds, L4RM_DEFAULT_REGION_AREA, &_addr /*ignore retval*/,
                        size, ds_offs, flags);
}

/*****************************************************************************
 *** l4rm_area_attach
 *****************************************************************************/
L4_INLINE int
l4rm_area_attach(const l4dm_dataspace_t * ds, l4_uint32_t area, l4_size_t size,
		 l4_offs_t ds_offs, l4_uint32_t flags, void ** addr)
{
  /* attach */
  *addr = (void *)L4RM_ADDR_FIND;
  return l4rm_do_attach(ds, area, (l4_addr_t *)addr, size, ds_offs, flags);
}

/*****************************************************************************
 *** l4rm_area_attach_to_region
 *****************************************************************************/
L4_INLINE int
l4rm_area_attach_to_region(const l4dm_dataspace_t * ds, l4_uint32_t area,
			   const void * addr, l4_size_t size, l4_offs_t ds_offs,
			   l4_uint32_t flags)
{
  /* attach */
  l4_addr_t _addr = (l4_addr_t)addr;
  return l4rm_do_attach(ds, area, &_addr /*ignore retval*/,
			size, ds_offs, flags);
}

/*****************************************************************************
 *** l4rm_direct_attach_to_region
 *****************************************************************************/
L4_INLINE int
l4rm_direct_attach_to_region(const l4dm_dataspace_t *ds, const void *addr,
			     l4_size_t size, l4_offs_t ds_offs,
			     l4_uint32_t flags)
{
  l4_addr_t _addr = (l4_addr_t)addr;
  return l4rm_do_attach(ds, L4RM_DEFAULT_REGION_AREA, &_addr /*ignore retval*/,
                        size, ds_offs, flags | L4RM_MODIFY_DIRECT);
}

/*****************************************************************************
 *** l4rm_direct_area_attach
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_attach(const l4dm_dataspace_t * ds, l4_uint32_t area,
                        l4_size_t size, l4_offs_t ds_offs, l4_uint32_t flags,
                        void ** addr)
{
  /* attach */
  *addr = (void *)L4RM_ADDR_FIND;
  return l4rm_do_attach(ds, area, (l4_addr_t *)addr, size, ds_offs,
                        flags | L4RM_MODIFY_DIRECT);
}

/*****************************************************************************
 *** l4rm_direct_area_attach_to_region
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_attach_to_region(const l4dm_dataspace_t * ds, l4_uint32_t area,
				  const void * addr, l4_size_t size,
				  l4_offs_t ds_offs, l4_uint32_t flags)
{
  /* attach */
  l4_addr_t _addr = (l4_addr_t)addr;
  return l4rm_do_attach(ds, area, &_addr /*ignore retval*/, size, ds_offs,
                        flags | L4RM_MODIFY_DIRECT);
}

/*****************************************************************************
 *** l4rm_area_setup
 *****************************************************************************/
L4_INLINE int
l4rm_area_setup(l4_size_t size, l4_uint32_t area, int type, l4_uint32_t flags,
                l4_threadid_t pager, l4_addr_t * addr)
{
  /* setup */
  *addr = L4RM_ADDR_FIND;
  return l4rm_do_area_setup(addr, size, area, type, flags, pager);
}

/*****************************************************************************
 *** l4rm_area_setup_region
 *****************************************************************************/
L4_INLINE int
l4rm_area_setup_region(l4_addr_t addr, l4_size_t size, l4_uint32_t area,
                       int type, l4_uint32_t flags, l4_threadid_t pager)
{
  /* setup */
  return l4rm_do_area_setup(&addr, size, area, type, flags, pager);
}

/*****************************************************************************
 *** l4rm_area_setup
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_setup(l4_size_t size, l4_uint32_t area, int type,
                       l4_uint32_t flags, l4_threadid_t pager, l4_addr_t * addr)
{
  /* setup */
  *addr = L4RM_ADDR_FIND;
  return l4rm_do_area_setup(addr, size, area, type, flags | L4RM_MODIFY_DIRECT,
                            pager);
}

/*****************************************************************************
 *** l4rm_area_setup_region
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_setup_region(l4_addr_t addr, l4_size_t size, l4_uint32_t area,
                              int type, l4_uint32_t flags, l4_threadid_t pager)
{
  /* setup */
  return l4rm_do_area_setup(&addr, size, area, type,
                            flags |  L4RM_MODIFY_DIRECT, pager);
}

/*****************************************************************************
 *** l4rm_area_reserve
 *****************************************************************************/
L4_INLINE int
l4rm_area_reserve(l4_size_t size, l4_uint32_t flags, l4_addr_t * addr,
		  l4_uint32_t * area)
{
  /* reserve */
  *addr = L4RM_ADDR_FIND;
  return l4rm_do_reserve(addr, size, flags, area);
}

/*****************************************************************************
 *** l4rm_area_reserve_region
 *****************************************************************************/
L4_INLINE int
l4rm_area_reserve_region(l4_addr_t addr, l4_size_t size, l4_uint32_t flags,
			 l4_uint32_t * area)
{
  /* reserve */
  return l4rm_do_reserve(&addr, size, flags, area);
}

/*****************************************************************************
 *** l4rm_direct_area_reserve
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_reserve(l4_size_t size, l4_uint32_t flags, l4_addr_t * addr,
                         l4_uint32_t * area)
{
  /* reserve */
  *addr = L4RM_ADDR_FIND;
  return l4rm_do_reserve(addr, size, flags | L4RM_MODIFY_DIRECT, area);
}

/*****************************************************************************
 *** l4rm_direct_area_reserve_region
 *****************************************************************************/
L4_INLINE int
l4rm_direct_area_reserve_region(l4_addr_t addr, l4_size_t size,
                                l4_uint32_t flags, l4_uint32_t * area)
{
  /* reserve */
  return l4rm_do_reserve(&addr, size,flags | L4RM_MODIFY_DIRECT, area);
}

#endif /* !_L4_L4RM_H */
