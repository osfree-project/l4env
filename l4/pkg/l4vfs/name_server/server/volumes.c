/**
 * \file   l4vfs/name_server/server/volumes.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "volumes.h"
#include "mount_table.h"

#include <l4/sys/types.h>

volume_entry_t volume_entries[NAME_SERVER_MAX_VOLUMES];

void init_volume_entries(void)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        volume_entries[i].root_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        volume_entries[i].root_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        volume_entries[i].server_id = L4_INVALID_ID;
    }
}

l4_threadid_t server_for_volume(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].root_id.volume_id == volume_id)
            return volume_entries[i].server_id;
    }
    return L4_INVALID_ID; // nothing found
}

local_object_id_t root_for_volume(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].root_id.volume_id == volume_id)
            return volume_entries[i].root_id.object_id;
    }
    return L4VFS_ILLEGAL_OBJECT_ID; // nothing found
}

int index_for_volume(volume_id_t volume_id)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].root_id.volume_id == volume_id)
            return i;
    }
    return -1; // nothing found
}

int insert_volume_server(object_id_t root_id, l4_threadid_t server_id)
{
    int i;
    l4_threadid_t t;

    // check for duplicates first
    t = server_for_volume(root_id.volume_id);
    if (! l4_is_invalid_id(t))
        return 1; // we have such an entry allready

    i = first_empty_volume_entry();
    if (i < 0)
        return 2; // error, we are full
    volume_entries[i].root_id.volume_id = root_id.volume_id;
    volume_entries[i].root_id.object_id = root_id.object_id;
    volume_entries[i].server_id = server_id;
    return 0;  // ok
}

int remove_volume_server(volume_id_t volume_id, l4_threadid_t server_id)
{
    int i;
    l4_threadid_t t;

    // check if we really have the pair
    t = server_for_volume(volume_id);
    if (! l4_thread_equal(t, server_id))
        return 1; // we dont have the exact pair

    // check if we still have the volume mounted
    if (is_volume_mounted(volume_id))
        return 2; // volume still mounted

    i = index_for_volume(volume_id);
    if (i < 0)
        return 3; // should not happen as we have found volume above

    // reset entry
    volume_entries[i].root_id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    volume_entries[i].root_id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    volume_entries[i].server_id = L4_INVALID_ID;
    return 0;
}

int first_empty_volume_entry(void)
{
    int i;
    for (i = 0; i < NAME_SERVER_MAX_VOLUMES; i++)
    {
        if (volume_entries[i].root_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID)
            return i;
    }
    return -1; // nothing found
}

/* Check if this is (one of) my own id(s)
 */
int is_own_volume(volume_id_t volume_id)
{
    return volume_id == L4VFS_NAME_SERVER_VOLUME_ID;
}
