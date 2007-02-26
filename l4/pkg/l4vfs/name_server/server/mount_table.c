/**
 * \file   l4vfs/name_server/server/mount_table.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "mount_table.h"

#include <l4/l4vfs/types.h>
#include <l4/log/l4log.h>

extern int _DEBUG;

mount_table_entry_t mount_table[NAME_SERVER_MAX_MOUNTS];

void init_mount_table()
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        mount_table[i].mount_point.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        mount_table[i].mount_point.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        mount_table[i].dest.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        mount_table[i].dest.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    }
}

int mount_insert_entry(object_id_t mount_point, object_id_t dest)
{
    int i;

    // check some boundary conditions

    i = find_empty_entry();
    if (i < 0)
        return -1; // no space in mount table left

    mount_table[i].mount_point.volume_id = mount_point.volume_id;
    mount_table[i].mount_point.object_id = mount_point.object_id;
    mount_table[i].dest.volume_id = dest.volume_id;
    mount_table[i].dest.object_id = dest.object_id;

    LOGd( _DEBUG, "<%d.%d>:<%d.%d>",
          mount_point.volume_id, mount_point.object_id,
          dest.volume_id, dest.object_id);

    return 0;
}

int find_empty_entry()
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if (mount_table[i].mount_point.volume_id == L4VFS_ILLEGAL_VOLUME_ID)
            return i;
    }
    return -1;
}

int mount_check_boundary_conditions(object_id_t mount_point, object_id_t dest)
{
    // check for
    //  - duplicate mounts
    //  - illegal data in parameters

    LOGd(_DEBUG, "(%d.%d), (%d.%d)",
        mount_point.volume_id, mount_point.object_id,
        dest.volume_id, dest.object_id);
    // we don't want duplicate mounts right now
    if (is_volume_mounted(dest.volume_id))
        return 11;

    // anything already mounted here?
    if (is_mount_point(mount_point))
        return 12;
    // yes, both are necessary!
    if (is_mounted_point(mount_point))
        return 13;

    // is this point already mounted somewhere?
    if (is_mount_point(dest))
        return 14;
    // yes, both are necessary!
    if (is_mounted_point(dest))
        return 15;

    return 0;
}

/* Check if the specified volume is mounted somewhere and therefore in use
 *
 */
int is_volume_mounted(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if (mount_table[i].dest.volume_id == volume_id)
            return 1;
    }
    return 0; // not mounted
}

/* Search for the given mount-point an return the mounted object-id
 */
object_id_t translate_mount_point(object_id_t mount_point)
{
    int i;
    object_id_t nil;

    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        LOGd(_DEBUG, "i = %d, mount_point = (%d.%d), mounted_point = (%d.%d) ",
            i,
            mount_table[i].mount_point.volume_id,
            mount_table[i].mount_point.object_id,
            mount_table[i].dest.volume_id,
            mount_table[i].dest.object_id);
        if ((mount_table[i].mount_point.volume_id == mount_point.volume_id) &&
            (mount_table[i].mount_point.object_id == mount_point.object_id))
            return mount_table[i].dest;
    }

    nil.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    nil.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return nil; // mount point not found
}

/* Search for the given mounted-point an return the mount-point
 * where it is mounted to object-id
 */
object_id_t translate_mounted_point(object_id_t mounted_point)
{
    int i;
    object_id_t nil;

    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if ((mount_table[i].dest.volume_id == mounted_point.volume_id) &&
            (mount_table[i].dest.object_id == mounted_point.object_id))
            return mount_table[i].mount_point;
    }

    nil.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    nil.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return nil; // mounted point not found
}

/* Check if given object_id is a mount point
 */
int is_mount_point(object_id_t mount_point)
{
    int i;

    LOGd( _DEBUG, "<%d.%d>", mount_point.volume_id, mount_point.object_id);

    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if ((mount_table[i].mount_point.volume_id == mount_point.volume_id) &&
            (mount_table[i].mount_point.object_id == mount_point.object_id))
            return true;
    }
    return false; // no mount_point
}

/* Check if given object_id is a mounted point
 */
int is_mounted_point(object_id_t mounted_point)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if ((mount_table[i].dest.volume_id == mounted_point.volume_id) &&
            (mount_table[i].dest.object_id == mounted_point.object_id))
            return true;
    }
    return false; // no mount_point
}

object_id_t get_mount_point(volume_id_t volume_id)
{
    int i;
    object_id_t ret;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if (mount_table[i].dest.volume_id == volume_id)
            return mount_table[i].mount_point;
    }
    
    ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return ret; // no mount_point
}

object_id_t get_mounted_point(volume_id_t volume_id)
{
    int i;
    object_id_t ret;
    for (i = 0; i < NAME_SERVER_MAX_MOUNTS; i++)
    {
        if (mount_table[i].dest.volume_id == volume_id)
            return mount_table[i].dest;
    }
    
    ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return ret; // no mount_point
}
