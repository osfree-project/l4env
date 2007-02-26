/**
 * \file   l4vfs/name_server/server/clientstate.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "clientstate.h"

#include "dirs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <l4/log/l4log.h>

extern int _DEBUG;

clientstate_t clients[MAX_CLIENTS];

int clientstate_open(int flags, l4_threadid_t client, local_object_id_t object_id)
{
    int ret;

    LOGd(_DEBUG, "check for space ...");
    // check for space
    ret = get_free_clientstate();
    if (ret < 0)
        return -ENOMEM;

    LOGd(_DEBUG, "fill data (flags = %d, o_id = %d)", flags, object_id);
    // fill data and return handle
    clients[ret].open      = true;
    clients[ret].rw_mode   = flags;
    clients[ret].seek_pos  = 0;
    clients[ret].client    = client;
    clients[ret].object_id = object_id;

    return ret;
}

int clientstate_close(int handle, l4_threadid_t client)
{
    // check for open and client
    if (clients[handle].open == false)
        return -EBADF;
    if (! l4_thread_equal(clients[handle].client, client))
        return -EBADF;

    // clean data
    free_clientstate(handle);
    return 0;
}

int get_free_clientstate(void)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].open == false)
            return i;
    }
    return -1;
}

void free_clientstate(int handle)
{
    clients[handle].open      = false;
    clients[handle].rw_mode   = 0;
    clients[handle].seek_pos  = 0;
    clients[handle].client    = L4_INVALID_ID;
    clients[handle].object_id = L4VFS_ILLEGAL_OBJECT_ID;
}

int clientstate_check_access(int flags,
                             l4_threadid_t client,
                             local_object_id_t object_id)
{
    if ((flags & O_ACCMODE) != O_RDONLY)
        return -EROFS;
    if (map_get_dir(object_id) == NULL)
        return -ENOENT;
    return 0;
}


int clientstate_getdents(int fd, l4vfs_dirent_t *dirp,
                         int count, l4_threadid_t client)
{
    obj_t * dir;

    LOGd(_DEBUG, "error checks ...");
    // some error checks
    if ((fd < 0) || (fd > MAX_CLIENTS))
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    if (! l4_thread_equal(clients[fd].client, client))
        return -EBADF;
    if (count <= 0)
        return -EINVAL;

    dir = map_get_dir(clients[fd].object_id);
    // should not happen ...
    if (dir == NULL)
        return -EBADF;

    clients[fd].seek_pos = dir_fill_dirents(dir, clients[fd].seek_pos,
                                            dirp, &count);

    if (count < 0)
        return -EINVAL;
    else
        return count;
}

off_t clientstate_lseek(int fd, off_t offset,
                        int whence, l4_threadid_t client)
{
    LOGd(_DEBUG, "error checks ...");
    // some error checks
    if ((fd < 0) || (fd > MAX_CLIENTS))
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    if (! l4_thread_equal(clients[fd].client, client))
        return -EBADF;

    // now seek, we support just reset for now
    if (whence != SEEK_SET)
        return -EINVAL;
    if (offset != 0)
        return -EINVAL;

    clients[fd].seek_pos = offset;
    return clients[fd].seek_pos;
}
