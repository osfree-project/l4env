/**
 * \file   l4vfs/include/volume_ids.h
 * \brief  Global collection of all volume_ids
 *
 * \date   08/19/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_VOLUME_IDS_H_
#define __L4VFS_INCLUDE_VOLUME_IDS_H_

/*********************************************************************
 *
 *  Please do only use the lower 15 Bits, i.e. 0 .. 32767 are valid.
 *
 *********************************************************************/

#define L4VFS_NAME_SERVER_VOLUME_ID  0

#define L4VFS_LOG_VOLUME_ID          10
#define SIMPLE_FILE_SERVER_VOLUME_ID 14
#define STATIC_FILE_SERVER_VOLUME_ID 56
#define RT_MON_L4VFS_COORD_VOLUME_ID 100
#define FERRET_L4VFS_VOLUME_ID       101
#define TMPFS_VOLUME_ID              111
#define TERM_CON_VOLUME_ID           132
#define L4EXT2_VOLUME_BASE           500
// leave some space here (not physical space, I mean numbers),
// note the '_BASE' above
#define L4SFS_VOLUME_ID              600
#define VC_SERVER_VOLUME_ID          1000

#endif
