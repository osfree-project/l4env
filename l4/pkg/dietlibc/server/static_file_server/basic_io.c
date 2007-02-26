#include "basic_io.h"
#include "state.h"

#include <errno.h>
#include <fcntl.h>

#include <l4/dietlibc/io-types.h>

#include <l4/log/l4log.h>

#define MIN(a, b) ((a)<(b)?(a):(b))

clientstate_t clients[MAX_CLIENTS];

extern int _DEBUG;

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
}

int clientstate_open(int flags, l4_threadid_t client, local_object_id_t object_id)
{
    int ret;

    LOGd(_DEBUG, "check for space ...");
    // check for space
    ret = get_free_clientstate();
    if (ret < 0)
        return -ENOMEM;
    LOGd(_DEBUG, "1");

    // check some error conditions
    if ((flags & O_ACCMODE) != O_RDONLY)
        return -EROFS;
    LOGd(_DEBUG, "2");
    if (object_id < 0 || object_id > MAX_STATIC_FILES)
        return -ENOENT;
    LOGd(_DEBUG, "3");
    // fix me: check for dir and translate to index
    if (object_id == 0)
        return -EACCES;
    if (files[object_id - 1].name == NULL)
        return -ENOENT;

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
    if (handle < 0 || handle > MAX_CLIENTS)
        return -EBADF;
    if (clients[handle].open == false)
        return -EBADF;
    if (! l4_thread_equal(clients[handle].client, client))
        return -EBADF;

    // clean data
    free_clientstate(handle);
    return 0;
}

int clientstate_read(object_handle_t fd, l4_int8_t * buf, size_t count)
{
    int ret;
    local_object_id_t oid;

    LOGd(_DEBUG, "fd = %d, buf = %p, count = %d", fd, buf, count);
    // check errors
    if (fd < 0 || fd > MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    oid = clients[fd].object_id;
    if (oid == 0)
        return -EISDIR;

    LOGd(_DEBUG, "iod = %d, len = %d, seek_pos = %d",
         oid, files[oid - 1].length, clients[fd].seek_pos);
    ret = MIN(count, files[oid - 1].length - clients[fd].seek_pos);
    memcpy(buf, files[oid - 1].data + clients[fd].seek_pos, ret);
    return ret;
}

int clientstate_seek(object_handle_t fd, off_t offset, int whence)
{
    local_object_id_t oid;
    // check errors
    if (fd < 0 || fd > MAX_CLIENTS)
        return -EBADF;
    if (clients[fd].open == false)
        return -EBADF;
    // fix me: check seek in dir here
    // ...

    oid = clients[fd].object_id;

    switch (whence)
    {
    case SEEK_SET:
        if (offset < 0 || offset > files[oid - 1].length)
            return -EINVAL;
        clients[fd].seek_pos = offset;
        break;
    case SEEK_CUR:
        if ((clients[fd].seek_pos + offset < 0) ||
            (clients[fd].seek_pos + offset > files[oid - 1].length))
            return -EINVAL;
        clients[fd].seek_pos += offset;
        break;
    case SEEK_END:
        if (offset < 0 || offset > files[oid - 1].length)
            return -EINVAL;
        clients[fd].seek_pos = files[oid - 1].length - offset;
        break;
    default:
        return -EINVAL;
    }
    return clients[fd].seek_pos;
}
