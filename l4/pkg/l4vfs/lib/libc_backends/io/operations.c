#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#include <l4/names/libnames.h>
#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/name_server.h>
#include <l4/dietlibc/basic_name_server.h>
#include <l4/dietlibc/basic_io.h>
#include <l4/dietlibc/rw.h>

#include <l4/crtx/ctor.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>

#include "file-table.h"
#include "operations.h"
#include "volumes.h"

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

static l4_threadid_t name_server = L4_INVALID_ID;

/* fix me: The following functions look somewhat similar. Maybe it is
 *         possible to factor out some similarities.
 */
int open(const char *pathname, int flags, ...)
{
    /* 1. go to name server and do a resolve -> object_id
     * 2. lookup the server responsible for the volume_id out of the
     *    object_id
     *    2a. locally at first
     *    2b. if not found ask the name server. It has such a
     *         translation registered
     *    2c. contact the corresponding server and ask for new session
     *      - connection-full servers:
     *         - return a new thread id now
     *      - connection-less servers:
     *         - just return own id or
     *         - don't implement interface at all, so that client-lib
     *           gets an idl-exception, which should be treated as if
     *           the server_id for the connection interface is the
     *           same like for the working interface
     *    2d. The translation betwen volume_id and server thread_id
     *        should be cached for connection-less server and must be
     *        cached for connection-full server by the io-backend (2a).
     * 3. call open at server with the object_id as parameter
     * 4. create local data-structures in the fd table and return
     *    index to it (fd)
     *
     * // fix me
     * x. care for some special flags which want us to behave like create
     *    O_CREAT (if not exist. -> create)
     *    O_CREAT+O_EXCL (if exist. -> error, else -> create)
     *    (see 'man 2 open')
     */

    int local_fd, ret;
    int mode;
    file_desc_t fd_s;
    object_handle_t object_handle;
    object_id_t object_id;
    l4_threadid_t server = L4_NIL_ID;

    LOGd(_DEBUG, "name = '%s', flags = '%d'", pathname, flags);
    // stolen from glibc/sysdeps/generic/open.c
    if (flags & O_CREAT)
    {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);
    }

    // 1.
    object_id = resolve(name_server, cwd, pathname);
    LOGd(_DEBUG, "resolved (%d.%d)", object_id.volume_id, object_id.object_id);

    // check for error
    if ((object_id.volume_id == L4_IO_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4_IO_ILLEGAL_OBJECT_ID))
    { // something went really wrong
        // fix me
        errno = ENOENT;
        return -1;
    }

    // 2.
    if (index_for_volume(object_id.volume_id) >= 0)
    { // we have a local hit
        server = server_for_volume(object_id.volume_id);
        LOGd(_DEBUG, "locally found server = '" IdFmt "'", IdStr(server));
    }
    else
    { // this means work, lookup translation remotely
        server = thread_for_volume(name_server, object_id.volume_id);
        LOGd(_DEBUG, "remotely found server = '" IdFmt "'", IdStr(server));
        // go to server and ask for a connection
        // fix me, currently we only support connection-less servers
        // server = open_connection(server);
        // now that we have it, store it locally
        ret = insert_volume_server(object_id.volume_id, server);
        LOGd(_DEBUG, "insert_volume_server = '%d'", ret);
        LOG_flush();
        if (ret == 1)
        { // should not happen
            abort();
        }
        if (ret == 2)
        { // no more room for new mappings
            errno = ENOMEM;
            return -1;
        }
    }

    // 3.
    object_handle = basic_io_open(server, object_id, flags);
    LOGd(_DEBUG, "basic_io_open = '%d'", object_handle);
    LOG_flush();
    fd_s.server_id = server;
    fd_s.object_handle = object_handle;

    // check for error
    if (fd_s.object_handle < 0)
    {
        // fix me, set errno
        errno = fd_s.object_handle;
        return -1;
    }

    // 4.
    // create local structures and stuff
    // fix me: this error should be checked before any remote contact
    local_fd = ft_get_next_free_entry();
    if (local_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }
    LOGd(_DEBUG, "local fd '%d'", local_fd);

    // now finally copy acquired data to local table and return
    ft_fill_entry(local_fd, fd_s);
    return local_fd;
}

int close(int fd)
{
    // check local file_desc
    // contact server and close file
    // cleanup local stuff

    if (! ft_is_open(fd))
        return EBADF;

    // fix me: impl. this stuff
    /*
    ret = l4x_vfs_close_call(&worker_id, fd, &errno, &_dice_corba_env);
    if (ret != 0)
    {
        // the errno should have been modified from the call above
        // note that we leave the fd open locally
        return -1;
    }

    // close was succesful, so cleanup locally
    ft_free_entry(fd);
	*/
    return 0;
}

int read(int fd, void *buf, size_t count)
{
    /* 1. lookup fd in the filetable, do some sanity checks, ...
     * 2. forward request to server
     * 3. check results, update errno, etc.
     */

    file_desc_t file_desc;
    ssize_t ret;

    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    file_desc = ft_get_entry(fd);
    if (l4_is_nil_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    // 2.
    ret = basic_io_read(file_desc.server_id,
                        file_desc.object_handle,
                        (l4_int8_t **)(&buf),
                        &count);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return -1;
    }
    return ret;
}

int write(int fd, const void *buf, size_t count)
{
    /* 1. lookup fd in the filetable, do some sanity checks, ...
     * 2. forward request to server
     * 3. check results, update errno, etc.
     */

    file_desc_t file_desc;
    ssize_t ret;

    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    file_desc = ft_get_entry(fd);
    if (l4_is_nil_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    // 2.
    ret = basic_io_write(file_desc.server_id,
                         file_desc.object_handle,
                         buf,
                         &count);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return -1;
    }
    return ret;
}

off_t lseek(int fd, off_t offset, int whence)
{
    /* 1. lookup fd in the filetable, do some sanity checks, ...
     * 2. forward request to server
     * 3. check results, update errno, etc.
     */

    file_desc_t file_desc;
    off_t ret;

    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    file_desc = ft_get_entry(fd);
    if (l4_is_nil_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    // 2.
    ret = basic_io_lseek(file_desc.server_id,
                         file_desc.object_handle,
                         offset,
                         whence);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return (off_t)-1;
    }
    return ret;
}

int getdents(int fd, struct dirent *dirp, unsigned int count)
{
    /* 1. lookup fd in the filetable, do some sanity checks, ...
     * 2. forward request to server
     * 3. check results, update errno, etc.
     */

    file_desc_t file_desc;
    off_t ret;

    LOGd(_DEBUG, "getdents(%d, %p, %d)", fd, dirp, count);
    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "is_open");
    file_desc = ft_get_entry(fd);
    if (l4_is_nil_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "is ok");

    // 2.
    ret = basic_io_getdents(file_desc.server_id,
                            file_desc.object_handle,
                            dirp,
                            count);
    LOGd(_DEBUG, "ret = '%d'", ret);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return (off_t)-1;
    }
    return ret;
}

// fix me: This does nothing useful, but only prevents linker errors
int fcntl(int fd, int cmd, ...)
{
    return 0;
}

// fix me
//more operations to come: creat, fsync

void init_io(void)
{
    name_server = get_name_server_threadid();
    // fix me
    // check for error

    init_volume_entries();
    ft_init();
}
L4C_CTOR(init_io, 2100);
