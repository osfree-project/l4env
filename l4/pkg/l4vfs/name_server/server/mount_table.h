/**
 * \file   l4vfs/name_server/server/mount_table.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_MOUNT_TABLE_H_
#define __L4VFS_NAME_SERVER_SERVER_MOUNT_TABLE_H_

#define NAME_SERVER_MAX_MOUNTS 32

#include <l4/l4vfs/types.h>

typedef struct
{
    object_id_t mount_point;
    object_id_t dest;
} mount_table_entry_t;

extern mount_table_entry_t mount_table[NAME_SERVER_MAX_MOUNTS];

void init_mount_table(void);
int mount_insert_entry(object_id_t mount_point, object_id_t dest);
int find_empty_entry(void);
int mount_check_boundary_conditions(object_id_t mount_point, object_id_t dest);
int is_volume_mounted(volume_id_t volume_id);
object_id_t translate_mount_point(object_id_t mount_point);
object_id_t translate_mounted_point(object_id_t mounted_point);
int is_mount_point(object_id_t mount_point);
int is_mounted_point(object_id_t mount_point);
object_id_t get_mount_point(volume_id_t volume_id);
object_id_t get_mounted_point(volume_id_t volume_id);

#endif
