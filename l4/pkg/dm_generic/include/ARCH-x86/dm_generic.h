/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_generic/include/l4/dm_generic/dm_generic.h
 * \brief   Generic dataspaca manager interface, client API
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

#ifndef _DM_GENERIC_DM_GENERIC_H
#define _DM_GENERIC_DM_GENERIC_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic-client.h>
#include <l4/dm_generic/types.h>
#include <l4/dm_generic/consts.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Map dataspace region (IDL wrapper)
 * \ingroup api_map
 * 
 * \param   ds           Dataspace descriptor
 * \param   offs         Offset in dataspace
 * \param   size         Region size
 * \param   rcv_addr     Receive window address
 * \param   rcv_size2    Receive window size (log2)
 * \param   rcv_offs     Offset in receive window
 * \param   flags        Flags:
 *                       - #L4DM_RO          map read-only
 *                       - #L4DM_RW          map read/write
 *                       - #L4DM_MAP_PARTIAL allow partial mappings 
 *                       - #L4DM_MAP_MORE    if possible, map more than the 
 *                                           specified dataspace region
 * \retval  fpage_addr   Map address of receive fpage
 * \retval  fpage_size   Size of receive fpage
 *
 * \return  0 on success (got fpage), error code otherwise:
 *          - -#L4_EIPC         IPC error calling dataspace manager
 *          - -#L4_EINVAL       invalid dataspace id or map / receive window 
 *                              size
 *          - -#L4_EINVAL_OFFS  invalid dataspace / receive window offset
 *          - -#L4_EPERM        permission denied
 *
 * Map the specified dataspace region. \a rcv_addr and \a rcv_size2 must
 * be a valid L4 flexpage receive window specification.
 * For a detailed description of \c L4DM_MAP_PARTIAL and \c L4DM_MAP_MORE
 * see l4dm_map().
 */
/*****************************************************************************/ 
int
l4dm_map_pages(l4dm_dataspace_t * ds, 
	       l4_offs_t offs, 
	       l4_size_t size, 
	       l4_addr_t rcv_addr, 
	       int rcv_size2, 
	       l4_offs_t rcv_offs, 
	       l4_uint32_t flags, 
	       l4_addr_t * fpage_addr, 
	       l4_size_t * fpage_size);

/*****************************************************************************/
/**
 * \brief   Map VM area
 * \ingroup api_map
 * 
 * \param   ptr          VM address
 * \param   size         Area size
 * \param   flags        Flags:
 *                       - #L4DM_RO          map read-only
 *                       - #L4DM_RW          map read/write
 *                       - #L4DM_MAP_PARTIAL allow partial mappings 
 *                       - #L4DM_MAP_MORE    if possible, map more than the 
 *                                           specified VM region
 *	
 * \return  0 on success (mapped VM area), error code otherwise:
 *          - -#L4_EIPC         IPC error calling regione mapper / 
 *                              dataspace manager
 *          - -#L4_ENOTFOUND    No dataspace attached to parts of the VM area
 *          - -#L4_EPERM        Permission denied
 *
 * Map the specified VM area. This will lookup the dataspaces which are
 * attached to the VM area and will call the dataspace managers to map the
 * dataspace pages. 
 * 
 * Flags:
 * - #L4DM_MAP_PARTIAL allow partial mappings of the VM area. If no
 *                     dataspace is attached to a part of the VM area, 
 *                     just stop mapping and return without an error.
 * - #L4DM_MAP_MORE    if possible, map more than the specified VM region. 
 *                     This allows l4dm_map to map more pages than specified
 *                     by \a ptr and \a size if a dataspace is attached to a 
 *                     larger VM region.
 */
/*****************************************************************************/ 
int
l4dm_map(void * ptr, 
	 l4_size_t size, 
	 l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Close dataspace.
 * \ingroup api_open
 * 
 * \param   ds           Dataspace id
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   operation not permitted, only the owner can 
 *                         close a dataspace
 */
/*****************************************************************************/ 
int
l4dm_close(l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Close all dataspaces of a client.
 * \ingroup api_open
 * 
 * \param   dsm_id       Dataspace manager thread id
 * \param   client       Client thread id
 * \param   flags        Flags:
 *                       - #L4DM_SAME_TASK  close all dataspaces owned by
 *                                          threads of the task specified by
 *                                          \a client.
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid client thread id
 *          - -#L4_EPERM   permission denied
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *
 * This function can be called by everyone. It's up to the dataspace manager
 * to decide who is allowed to close that dataspaces.
 */
/*****************************************************************************/ 
int
l4dm_close_all(l4_threadid_t dsm_id, 
	       l4_threadid_t client, 
	       l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Grant dataspace access rights to a client
 * \ingroup api_client
 * 
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 * \param   rights       Access rights:
 *                       - #L4DM_RO     read-only access
 *                       - #L4DM_RW     read/write access
 *                       - #L4DM_RESIZE allow dataspace resize
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   the requested rights for the new client exceed 
 *                         the rights of the caller for the dataspace
 * 
 * Grant / extend dataspace access rights to a client. If the client already 
 * has access to the dataspace, the new rights are added to the existing 
 * rights.
 */
/*****************************************************************************/ 
int
l4dm_share(l4dm_dataspace_t * ds, 
	   l4_threadid_t client, 
	   l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Revoke dataspace access rights.
 * \ingroup api_client
 * 
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 * \param   rights       Access rights:
 *                       - #L4DM_WRITE       revoke write access
 *                       - #L4DM_RESIZE      revoke resize right
 *                       - #L4DM_ALL_RIGHTS  revoke all rights, the client
 *                                           is removed from the client list
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   caller has not the right to revoke access rights
 * 
 * Revoke dataspace access rights. If the resulting rights for the specified 
 * client are 0, the client is removed from the dataspaces client list.
 */
/*****************************************************************************/ 
int
l4dm_revoke(l4dm_dataspace_t * ds, 
	    l4_threadid_t client, 
	    l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Check dataspace access rights
 * \ingroup api_client
 * 
 * \param   ds           Dataspace descriptor
 * \param   rights       Access rights:
 *                       - #L4DM_RO     read-only access
 *                       - #L4DM_RW     read/write access
 *                       - #L4DM_RESIZE resize
 *	
 * \return  0 if caller has the access rights, error code otherwise:
 *          - -#L4_EIPC    IPC error calling dataspace manager
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   requested operations not allowed
 *
 * Check if the caller has the specified access rights for the dataspace.
 */
/*****************************************************************************/ 
int
l4dm_check_rights(l4dm_dataspace_t * ds, 
		  l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Transfer dataspace ownership
 * \ingroup api_client
 * 
 * \param   ds           Dataspace descriptor
 * \param   new_owner    New dataspace owner
 *	
 * \return  0 on success (set owner to \a new_owner), error code otherwise:
 *          - -#L4_EIPC   IPC error calling dataspace manager
 *          - -#L4_EINVAL Invalid dataspace descriptor
 *          - -#L4_EPERM  Permission denied, only the current owner can 
 *                        transfer the ownership
 */
/*****************************************************************************/ 
int
l4dm_transfer(l4dm_dataspace_t * ds, 
	      l4_threadid_t new_owner);

/*****************************************************************************/
/**
 * \brief   Create dataspace copy, short form
 * \ingroup api_open
 * 
 * \param   ds           Source dataspace id
 * \param   flags        Flags:
 *                       - #L4DM_COW         create copy-on-write copy
 *                       - #L4DM_PINNED      create copy on pinned memory
 *                       - #L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                           memory
 * \param   name         Copy name
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
 * Create a copy of the whole dataspace.
 */
/*****************************************************************************/ 
int
l4dm_copy(l4dm_dataspace_t * ds, 
	  l4_uint32_t flags, 
	  const char * name, 
	  l4dm_dataspace_t * copy);

/*****************************************************************************/
/**
 * \brief   Create dataspace copy, long form
 * \ingroup api_open
 * 
 * \param   ds           Source dataspace id
 * \param   src_offs     Offset in source dataspace
 * \param   dst_offs     Offset in destination dataspace
 * \param   num          Number of bytes to copy, set to #L4DM_WHOLE_DS to copy 
 *                       the whole dataspace starting at \a src_offs
 * \param   flags        Flags
 *                       - #L4DM_COW         create copy-on-write copy
 *                       - #L4DM_PINNED      create copy on pinned memory
 *                       - #L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                           memory
 * \param   name         Copy name
 * \retval  copy         Dataspace id of copy
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC       IPC error calling dataspace manager
 *          - -#L4_EINVAL     Invalid source dataspace id
 *          - -#L4_EPERM      Permission denied
 *          - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *          - -#L4_ENOMEM     Out of memory creating copy
 *
 * Create a copy of the dataspace, \a src_offs and \a num specify the area of
 * the source dataspace which should be copied to the destination dataspace
 * at offset \a dst_offs. There are no restrictions to the offsets, in 
 * particular they do not need to be aligned to pagesizes.
 */
/*****************************************************************************/ 
int
l4dm_copy_long(l4dm_dataspace_t * ds, 
	       l4_offs_t src_offs, 
	       l4_offs_t dst_offs,
	       l4_size_t num, 
	       l4_uint32_t flags, 
	       const char * name, 
	       l4dm_dataspace_t * copy);

/*****************************************************************************/
/**
 * \brief   Set dataspace name
 * \ingroup api_debug
 * 
 * \param   ds           Dataspace id
 * \param   name         Dataspace name
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EPERM   caller is not the owner of the dataspace
 *          - -#L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_set_name(l4dm_dataspace_t * ds, 
		 const char * name);

/*****************************************************************************/
/**
 * \brief   Get dataspace name
 * \ingroup api_debug
 * 
 * \param   ds           Dataspace id
 * \retval  name         Dataspace name
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid dataspace id
 *          - -#L4_EIPC    IPC error calling dataspace manager 
 */
/*****************************************************************************/ 
int
l4dm_ds_get_name(l4dm_dataspace_t * ds, 
		 char * name);

/*****************************************************************************/
/**
 * \brief   Show information about dataspace
 * \ingroup api_debug
 *
 * \param   ds           Dataspace id
 */
/*****************************************************************************/ 
void
l4dm_ds_show(l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   Dump dataspaces
 * \ingroup api_debug
 * 
 * \param   dsm_id       Dataspace manager thread id
 * \param   owner        Dataspace owner, if set to #L4_INVALID_ID dump all
 *                       dataspaces
 * \param   flags        Flags:
 *                       - #L4DM_SAME_TASK  dump dataspaces owned by task
 * \retval  ds           Dataspace id
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC       IPC error calling dataspace manager
 *          - -#L4_ENOHANDLE  could not create dataspace descriptor
 *          - -#L4_ENOMEM     out of memory
 */
/*****************************************************************************/ 
int
l4dm_ds_dump(l4_threadid_t dsm_id, 
	     l4_threadid_t owner, 
	     l4_uint32_t flags,
	     l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief   List dataspaces
 * \ingroup api_debug
 * 
 * \param   dsm_id       Dataspace manager id, set to #L4DM_DEFAULT_DSM 
 *                       to use default dataspace manager provided by the
 *                       L4 environment
 * \param   owner        Dataspace owner, if set to #L4_INVALID_ID list
 *                       all dataspaces
 * \param   flags        Flags:
 *                       - #L4DM_SAME_TASK  list dataspaces owned by task
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list(l4_threadid_t dsm_id, 
	     l4_threadid_t owner, 
	     l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   List all dataspaces
 * \ingroup api_debug
 * 
 * \param   dsm_id       Dataspace manager id, set to #L4DM_DEFAULT_DSM 
 *                       to use default dataspace manager provided by the
 *                       L4 environment
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC IPC error calling dataspace manager
 */
/*****************************************************************************/ 
int
l4dm_ds_list_all(l4_threadid_t dsm_id);

__END_DECLS;

#endif /* !_DM_GENERIC_DM_GENERIC_H */
