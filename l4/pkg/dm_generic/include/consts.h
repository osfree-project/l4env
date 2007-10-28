/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_generic/include/consts.h
 * \brief   Generic Dataspace manager interface, common constants
 * \ingroup api
 *
 * \date    11/20/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_DM_GENERIC_CONSTS_H
#define _L4_DM_GENERIC_CONSTS_H

/* L4/L4Env includes */
#include <l4/sys/types.h>

/*****************************************************************************
 *** Flags / Access rights, the bit mask consists of several parts:
 ***
 ***   31       23       15        7       0
 ***   +--------+--------+--------+--------+
 ***   |  l4rm  | DMphys | flags  | rights |
 ***   +--------+--------+--------+--------+
 ***
 *** rights   Dataspace access rights
 *** flags    Open / copy / map flags
 *** DMphys   Additional flags defined by DMphys (l4/dm_phys/consts.h)
 *** l4rm     Flags defined by region mapper (l4/l4rm/l4rm.h)
 *****************************************************************************/

/* Access rights bits */
#define L4DM_READ             0x00000001               /**< \ingroup api_map
                                                        **  Read access
                                                        **/
#define L4DM_WRITE            0x00000002               /**< \ingroup api_map
                                                        **  Write access
                                                        **/
#define L4DM_RESIZE           0x00000080               /**< \ingroup api_open
                                                        **  Allow resize
                                                        **/
#define L4DM_RO               (L4DM_READ)              /**< \ingroup api_map
                                                        **  read-only access
                                                        **/
#define L4DM_RW               (L4DM_READ | L4DM_WRITE) /**< \ingroup api_map
                                                        **  read-write access
                                                        **/
#define L4DM_ALL_RIGHTS       0x000000FF               /**< \ingroup api_client
                                                        **  all rights
                                                        **/
#define L4DM_RIGHTS_MASK      0x000000FF

/* Flags open / copy */
#define L4DM_CONTIGUOUS       0x00000100     /**< \ingroup api_open
                                              **  Allocate phys. contiguous
                                              **  memory
                                              **/
#define L4DM_PINNED           0x00000200     /**< \ingroup api_open
                                              **  Allocated pinned memory
                                              **/
#define L4DM_COW              0x00000400     /**< \ingroup api_open
                                              **  Create copy-on-write
                                              **  dataspace copy
                                              **/

/* Flags map */
#define L4DM_MAP_PARTIAL      0x00000800     /**< \ingroup api_map
                                              **  Allow partial mappings
                                              **/
#define L4DM_MAP_MORE         0x00001000     /**< \ingroup api_map
                                              **  Allow larger mappings
                                              **  than requested
                                              **/

/* Flags close_all */
#define L4DM_SAME_TASK        0x00002000     /**< \ingroup api_open
                                              **  Close dataspace owned by
                                              **  threads of the same task
                                              **/

/*****************************************************************************
 *** Misc.
 *****************************************************************************/

/* Dataspace name length */
#define L4DM_DS_NAME_MAX_LEN  32

/* Special arguments */
#define L4DM_WHOLE_DS         (-1)           /**< \ingroup api_open
                                              **  Copy: create copy of the
                                              **    whole dataspace,
                                              **  phys_addr: return the phys.
                                              **    addresses of the whole
                                              **    dataspace
                                              **/

/* Dataspace manager */
#define L4DM_DEFAULT_DSM      L4_INVALID_ID  /**< \ingroup api_open
                                              **  Open: use default dataspcae
                                              **    manager
                                              **/

#endif /* !_L4_DM_GENERIC_CONSTS_H */
