#include "clientstate.h"

#include "dirs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <l4/log/l4log.h>

extern int _DEBUG;

clientstate_t clients[MAX_CLIENTS];

int clientstate_open(int flags, l4_threadid_t client, local_object_id_t object_id)
{
    int ret;
    dir_t * dirp;

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

    LOGd(_DEBUG, "init. buf");
    // init buffer
    dirp = map_get_dir(object_id);
    dir_childs_to_dirents(dirp, (struct dirent **)&(clients[ret].data),
                          &(clients[ret].len));
    LOGd(_DEBUG, "len = %d", clients[ret].len);
    LOGd(_DEBUG, "%p", clients[ret].data);

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
    clients[handle].client    = L4_NIL_ID;
    clients[handle].object_id = L4_IO_ILLEGAL_OBJECT_ID;
    free(clients[handle].data);
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


int clientstate_getdents(int fd, struct dirent *dirp,
                         int count, l4_threadid_t client)
{
    int len;
    int copied = 0;

    LOGd(_DEBUG, "error checks ...");
    // some error checks
    if ((fd < 0) || (fd > MAX_CLIENTS))
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    if (! l4_thread_equal(clients[fd].client, client))
        return -EBADF;

    LOGd(_DEBUG, "loop ...");
    // do the copy
    do
    {
        LOGd(_DEBUG, "seek_pos ...");
        if (clients[fd].seek_pos >= clients[fd].len)
        {
            return copied; // 0 is legal here, means EOF
        }
        len = ((struct dirent *)(clients[fd].data +
                                 clients[fd].seek_pos))->d_reclen;
        LOGd(_DEBUG, "len = %d, copied = %d, count = %d",
             len, copied, count);
        if (copied + len > count)
        {
            if (copied == 0)
                return -EINVAL;
            else
                return copied;
        }
        LOGd(_DEBUG, "memcpy: dirp = %p, copied = %d, data = %p, seek_pos = %d",
             dirp, copied, clients[fd].data, clients[fd].seek_pos);
        LOG_flush();
        memcpy(((char *)dirp) + copied,
               clients[fd].data + clients[fd].seek_pos,
               len);
        LOGd(_DEBUG, "update vars ...");
        clients[fd].seek_pos += len;
        copied += len;
    } while (1);
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
