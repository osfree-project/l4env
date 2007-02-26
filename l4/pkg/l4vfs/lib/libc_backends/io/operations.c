/**
 * \file   dietlibc/lib/backends/io/operations.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/container_io.h>
#include <l4/l4vfs/common_io.h>
#include <l4/l4vfs/file-table.h>
#include <l4/l4vfs/volumes.h>

#include <l4/crtx/ctor.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/util/mbi_argv.h>   // for argc and argv[]
#include <l4/names/libnames.h>

#include "operations.h"

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

l4_threadid_t l4vfs_name_server = L4_INVALID_ID;


/* fixme: The following functions look somewhat similar. Maybe it is
 *        possible to factor out some similarities.
 */
int open(const char *pathname, int flags, ...)
{
    /* 1. go to name server and do a resolve -> object_id
     * 2. lookup the server responsible for the volume_id out of the
     *    object_id
     *    2a. locally at first
     *    2b. if not found ask the name server, if it has such a
     *        translation registered
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
     */

    int local_fd, ret;
    mode_t mode = 0; /* prevent compiler warning */
    file_desc_t file_desc;
    object_handle_t object_handle;
    object_id_t object_id;
    l4_threadid_t server = L4_INVALID_ID;

    LOGd(_DEBUG, "name = '%s', flags = '%d'", pathname, flags);
    // taken from glibc/sysdeps/generic/open.c
    if (flags & O_CREAT)
    {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);
    }

    // 4.
    // create local structures and stuff
    // fixed: this error is now checked before any remote contact
    local_fd = ft_get_next_free_entry();
    if (local_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }
    LOGd(_DEBUG, "local fd '%d'", local_fd);

    // 1.
    object_id = l4vfs_resolve(l4vfs_name_server, cwd, pathname);
    LOGd(_DEBUG, "resolved (%d.%d)", object_id.volume_id, object_id.object_id);

    // check for error && ! O_CREATE
    if (((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
         (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID)) &&
        ((flags & O_CREAT) == 0))
    {
        // no file found
        errno = ENOENT;
        return -1;
    }
    // check for file found and O_CREAT && O_EXCL
    if ((object_id.volume_id != L4VFS_ILLEGAL_VOLUME_ID) &&
        (object_id.object_id != L4VFS_ILLEGAL_OBJECT_ID) &&
        (flags & O_CREAT) && (flags & O_EXCL))
    {
            errno = EEXIST;
            return -1;
    }

    // 3.
    if (flags & O_CREAT)
    {
        char * temp, * dir;
        temp = strdup(pathname);
        dir = dirname(temp);
        // now lets see if we can resolve the parent dir to the file
        // to be created
        object_id = l4vfs_resolve(l4vfs_name_server, cwd, dir);
        free(temp);
        if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
            (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
        { // dir not found, cannot create
            errno = ENOENT;
            return -1;
        }
    }

    // now lets get the server responsible
    ret = vol_resolve_thread_for_volume_id(object_id.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    if (flags & O_CREAT)
    {
        char * temp, * file;
        temp = strdup(pathname);
        file = basename(temp);
        object_handle = l4vfs_creat(server, object_id, file, flags, mode);
        free(temp);
        LOGd(_DEBUG, "basic_io_creat = '%d'", object_handle);
    }
    else // ! O_CREATE
    {
        // 2.
        object_handle = l4vfs_open(server, object_id, flags);
        LOGd(_DEBUG, "basic_io_open = '%d'", object_handle);
    }
    // check for error
    if (object_handle < 0)
    {
        errno = -object_handle;
        return -1;
    }

    file_desc.server_id     = server;
    file_desc.object_handle = object_handle;
    file_desc.object_id     = object_id;
    // now finally copy acquired data to local table and return
    ft_fill_entry(local_fd, file_desc);
    return local_fd;
}

/* from: man 2 open
 * "creat is equivalent to open with flags equal to O_CREAT|O_WRONLY|O_TRUNC."
 *
 * If this is really true, we can save us a lot of trouble below and
 * just call open accordingly.
 */

int creat(const char *pathname, mode_t mode)
{
    return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int access(const char *pathname, int mode)
{
    /* 1. try to resolve pathname, else return -ENOENT
     * 2. lookup server as in open
     * 3. call access at server with object_id as parameter
     */
    int ret;
    object_id_t object_id;
    l4_threadid_t server = L4_INVALID_ID;

    // 1.
    object_id = l4vfs_resolve(l4vfs_name_server, cwd, pathname);
    LOGd(_DEBUG, "resolved (%d.%d)", object_id.volume_id, object_id.object_id);

    // check for error
    if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    {
        // invalid path or file does not exist
        errno = ENOENT;
        return -1;
    }

    // 2.
    ret = vol_resolve_thread_for_volume_id(object_id.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    // 3.
    ret = l4vfs_access(server, object_id, mode);
    LOGd(_DEBUG, "basic_io_access = '%d'", ret);
    if (ret)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

int close(int fd)
{
    // check local file_desc
    // contact server and close file
    // cleanup local stuff

    l4_int32_t ret;
    file_desc_t file_desc;

    LOGd(_DEBUG, "local fd '%d'", fd);
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(fd);
    ret = l4vfs_close(file_desc.server_id, file_desc.object_handle);
    LOGd(_DEBUG, "server ret");

    if (ret != 0)
    {
        if (ret == EBADF)
        {
            // we got an inconsistant state to server
            errno = EIO;
            return -1;
        }
        else
        {
            // unknown case, what should I do?
            LOGd(_DEBUG, "Error in close, unknown case!");
            errno = -ret;
            return -1;
        }
    }
    LOGd(_DEBUG, "ok.");

    // everything seems fine, now cleanup the local stuff
    ft_free_entry(fd);
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
    l4_int8_t *b = (l4_int8_t*)buf;

    // 1.
    LOGd(_DEBUG, "fd = '%d', buf = '%p', count = '%d''", fd, buf, count);
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    file_desc = ft_get_entry(fd);
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    if (buf == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    // 2.
    ret = l4vfs_read(file_desc.server_id,
                     file_desc.object_handle,
                     &b,
                     &count);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return -1;
    }
    LOGd(_DEBUG,
         "content: '%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx ...' '%.100s'",
         b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b);
    LOGd(_DEBUG, "ret = '%d'", (int)ret);
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
    LOGd(_DEBUG, "fd = '%d', buf = '%p', count = '%d''", fd, buf, count);
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "1.5 local fd '%d'", fd);
    file_desc = ft_get_entry(fd);
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    if (buf == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    LOGd(_DEBUG,
         "content: '%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx%0hhx ...' '%.100s'",
         ((char *)buf)[0], ((char *)buf)[1], ((char *)buf)[2],
         ((char *)buf)[3], ((char *)buf)[4], ((char *)buf)[5],
         ((char *)buf)[6], ((char *)buf)[7], ((char *)buf)[8],
         ((char *)buf)[9], (char *)buf);

    // 2.
    LOGd(_DEBUG, "2. local fd '%d'", fd);
    ret = l4vfs_write(file_desc.server_id,
                      file_desc.object_handle,
                      buf,
                      &count);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return -1;
    }
    LOGd(_DEBUG, "ret = '%d''", (int)ret);
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

    LOGd(_DEBUG, "fd = '%d', offset = '%d', whence = %d", 
         fd, (int)offset, whence);
    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    file_desc = ft_get_entry(fd);
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    // 2.
    LOGd(_DEBUG, "file_desc = '" l4util_idfmt ":%d'",
         l4util_idstr(file_desc.server_id), file_desc.object_handle);
    ret = l4vfs_lseek(file_desc.server_id,
                      file_desc.object_handle,
                      offset,
                      whence);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return (off_t)-1;
    }
    LOGd(_DEBUG, "ret = %d, ok.", (int)ret);
    return ret;
}

off64_t lseek64(int fd, off64_t offset, int whence)
{
    // sizeof(l4vfs_off_t) = 32
    if (offset > INT_MAX)
        return EINVAL;

    return lseek(fd, offset, whence);
}

#ifdef USE_UCLIBC
int __getdents(int fd, struct dirent *dirp, unsigned int count)
  __attribute__((alias("getdents")));
int __getdents64(int fd, struct dirent *dirp, unsigned int count)
  __attribute__((alias("getdents")));
int getdents(int fd, struct dirent *dirp, unsigned int count);
#endif
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
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "is ok");

    // 2.
    ret = l4vfs_getdents(file_desc.server_id,
                         file_desc.object_handle,
                         (l4vfs_dirent_t*)dirp,
                         count);
    LOGd(_DEBUG, "ret = '%d'", (int)ret);

    // 3.
    if (ret < 0)
    {
        errno = -ret; // is this ok?
        return (off_t)-1;
    }
    return ret;
}

int mkdir(const char *pathname, mode_t mode)
{
    /* 0. some error checks
     * 1. resolve pathname to pair of (parent, local_name)
     * 2. get server for parent.volume_id
     * 3. call container_io_mkdir(server, parent, local_name, mode)
     */

    object_id_t parent;
    l4_threadid_t server = L4_INVALID_ID;
    char * base;
    char * dir;
    char * temp;
    int ret;

    // 0.

    // 1.
    temp = strdup(pathname);
    dir = dirname(temp);
    parent = l4vfs_resolve(l4vfs_name_server, cwd, dir);
    free(temp);
    // check for error
    if ((parent.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (parent.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    { // something went really wrong, probably path was not resolvable
        errno = ENOENT;
        return -1;
    }

    // 2.
    ret = vol_resolve_thread_for_volume_id(parent.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    // 3.
    temp = strdup(pathname);
    base = basename(temp);
    ret = l4vfs_mkdir(server, &parent, base, mode);
    free(temp);

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }
    return 0;
}

// fixme
//more operations to come: creat, fsync

int chdir(const char* pathname)
{
    /* 1. try to resolve the name
     * 2. check for access rights (maybe just the x bit ?)
     * 3. change the cwd locally to the resolved object_id
     */

    object_id_t object_id;

    // 1.
    object_id = l4vfs_resolve(l4vfs_name_server, cwd, pathname);
    LOGd(_DEBUG, "resolved (%d.%d)", object_id.volume_id, object_id.object_id);

    // check for error
    if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    { // something went really wrong
        errno = ENOENT;
        return -1;
    }

    // 2.
    // fixme, check for access rights here

    // 3.
    // still here? then we got a new cwd ...
    cwd = object_id;

    return 0;
}

int fchdir(int fd)
{
    /* 1. try to find the object_id for the fd (maybe using fstat)
     * (access rights should be ok, as we have the file open)
     * 2. change the cwd locally to the fstated object_id
     */

    // fixme: implement the description above, if we have fstat
    errno = EBADF;
    return -1;
}

int unlink(const char* pathname)
{
    /* 1. go to name server and do a resolve -> object_id
     * 2. unlink the object
     */

    int ret;
    object_id_t object_id;
    l4_threadid_t server = L4_INVALID_ID;

    // 1.
    object_id = l4vfs_resolve(l4vfs_name_server, cwd, pathname);
    LOGd(_DEBUG, "resolved (%d.%d)", object_id.volume_id, object_id.object_id);

    // check for error
    if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    {
        // no file found
        errno = ENOENT;
        return -1;
    }

    // now lets get the server responsible
    ret = vol_resolve_thread_for_volume_id(object_id.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    // 2. unlink the file
    ret = l4vfs_unlink(server, object_id);
    LOGd(_DEBUG, "basic_io_unlink = '%d'", ret);

    // check for error
    if (ret)
    {
        errno = ret;
        return -1;
    }

    return 0;
}

int rmdir(const char* pathname)
{
// fixme: do something useful here
    errno = EPERM;
    return -1;
}

int stat(const char* pathname, struct stat *buf)
{
    /* 1. resolve name
     * 2. lookup object server
     * 3. do a stat on it
     * 4. convert stat type and return it
     */

    object_id_t object_id;
    int ret;
    l4vfs_stat_t stat_buf;
    l4_threadid_t server = L4_INVALID_ID;

    // 1.
    object_id = l4vfs_resolve(l4vfs_name_server, cwd, pathname);
    // check for error
    if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    {
        // no file found
        errno = ENOENT;
        return -1;
    }

    // 2.
    ret = vol_resolve_thread_for_volume_id(object_id.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    // 3.
    ret = l4vfs_stat(server, object_id, &stat_buf);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    // 4.
    // fixme: support more fields here
    buf->st_dev   = stat_buf.st_dev;
    buf->st_ino   = stat_buf.st_ino;
    buf->st_mode  = stat_buf.st_mode;
    buf->st_nlink = stat_buf.st_nlink;
    buf->st_size  = stat_buf.st_size;

    return 0;
}

int fstat(int fd, struct stat *buf)
{
    /* 1. check 'fd'
     * 2. do a stat on it
     * 3. convert stat type and return it
     */

    int ret;
    l4vfs_stat_t stat_buf;
    file_desc_t file_desc;

    LOGd(_DEBUG, "fstat(%d, %p)", fd, buf);
    // 1.
    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "is_open");
    file_desc = ft_get_entry(fd);
    if (l4_is_invalid_id(file_desc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }
    LOGd(_DEBUG, "is ok");

    // 2.
    ret = l4vfs_stat(file_desc.server_id, file_desc.object_id, &stat_buf);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    // 3.
    // fixme: support more fields here
    buf->st_dev   = stat_buf.st_dev;
    buf->st_ino   = stat_buf.st_ino;
    buf->st_mode  = stat_buf.st_mode;
    buf->st_nlink = stat_buf.st_nlink;
    buf->st_size  = stat_buf.st_size;

    return 0;
}

int lstat(const char* pathname, struct stat *buf)
{
// fixme: do something useful here
    errno = EACCES;
    return -1;
}

int rename(const char * oldpath, const char * newpath)
{
// fixme: do something useful here
    errno = EACCES;
    return -1;
}

int __syscall_getcwd(char *buf, size_t size);
int __syscall_getcwd(char *buf, size_t size)
{
    char * ret;
    int len;

    LOGd(_DEBUG, "cwd:  (%d.%d)", cwd.volume_id, cwd.object_id);
    LOGd(_DEBUG, "root: (%d.%d)", c_root.volume_id, c_root.object_id);
    ret = l4vfs_rev_resolve(l4vfs_name_server, cwd, &c_root);
    if (ret == NULL) // something went wrong
    {
        errno = ENOENT;  // ???
        return -1;
    }
    
    len = strlen(ret);
    if (len > size)  /// buffer too small
    {
        errno = ERANGE;
        return -1;
    }

    // ok
    strncpy(buf, ret, len);
    return len;
}

int dup(int oldfd)
{
    int new_fd;
    file_desc_t file_desc;

    // error checks
    if (! ft_is_open(oldfd))
    {
        errno = EBADF;
        return -1;
    }
    new_fd = ft_get_next_free_entry();
    if (new_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }
    LOGd(_DEBUG, "new fd '%d'", new_fd);

    file_desc = ft_get_entry(oldfd);

    ft_fill_entry(new_fd, file_desc);
    return new_fd;
}

int getdtablesize(void)
{
    return MAX_FILES_OPEN;
}

void init_io(void)
{
    int i, fd, k;
    char * par[3] = {NULL, NULL, NULL};
    char * arg[3] = {"--stdin", "--stdout", "--stderr"};
    file_desc_t file_desc;

    l4vfs_name_server = l4vfs_get_name_server_threadid();
    // fixme
    // check for error

    init_volume_entries();
    ft_init();

    // fixme: we could setup stdin, stdout, and stderr here
    /* ok, here is what we do:
     * we scan all argument, beginning from 1 to argc for the strings
     * "--stdin", "--stdout", and "--stderr".
     * The next argument after each of the strings found is treated as its
     * value.
     * After the scan, all the strings found and their values are removed
     * from the argument list, argv[] and argc are fixed.
     *
     * Example: "testme --stdin /dev/vc/1 -x --stdout /dev/vc2 abc -sd -g" will
     * be scanned and shortened to "testme abc -sd -g".
     *
     * The last element of the arguments will not be searched for the
     * special strings as an argument is allways required.
     */

    for (fd = 0; fd < 3; fd++)
    {
        for (i = 1; i < l4util_argc - 1; i++)
        {
            // compare, including the trailing '\0' to prevent prefix-only
            // equals
            LOGd(_DEBUG, "checking %s", l4util_argv[i]);
            if (strncmp(arg[fd], l4util_argv[i], 9) == 0)
            {  // ok, we found a matching argument
                par[fd] = l4util_argv[i + 1];  // save the value

                // remove both from the argument list
                for (k = i; k < l4util_argc - 1; k++)
                    l4util_argv[k] = l4util_argv[k + 2];
                l4util_argc -= 2;

                break;  // ok, search for the next argument
            }
        }
        LOGd(_DEBUG, "Parameter for fd %d, %s", fd, par[fd]);
    }

    // setup dummy descriptor to order fds the correct way
    // we want stdin to be 0, stdout to be 1, and stderr to be 2
    file_desc.server_id = L4_NIL_ID;
    file_desc.object_handle = -1;
    if (par[0] != NULL)
    {
        for (i = 0; i < 10; i++)
        {
            fd = open(par[0], O_RDONLY);
            if (fd < 0)
            {
                LOG("Failed to open stdin, errno = %d, retrying ...", errno);
                l4_sleep(200);
            }
            else
                break;
        }
        if (fd != 0)
        {
            LOG("Failed to open stdin, fd = '%d', errno = %d", fd, errno);
            ft_fill_entry(0, file_desc);  // fill in dummy entry
        }
    }
    else
        ft_fill_entry(0, file_desc);      // fill in dummy entry

    if (par[1] != NULL)
    {
        for (i = 0; i < 10; i++)
        {
            fd = open(par[1], O_WRONLY);
            if (fd < 0)
            {
                LOG("Failed to open stdout, errno = %d, retrying ...", errno);
                l4_sleep(200);
            }
            else
                break;
        }
        if (fd != 1)
        {
            LOG("Failed to open stdout, fd = '%d', errno = %d", fd, errno);
            ft_fill_entry(1, file_desc);  // fill in dummy entry
        }
    }
    else
        ft_fill_entry(1, file_desc);      // fill in dummy entry

    if (par[2] != NULL)
    {
        for (i = 0; i < 10; i++)
        {
            fd = open(par[2], O_WRONLY);
            if (fd < 0)
            {
                LOG("Failed to open stderr, errno = %d, retrying ...", errno);
                l4_sleep(200);
            }
            else
                break;
        }
        if (fd != 2)
            LOG("Failed to open stderr, fd = '%d', errno = %d", fd, errno);
    }

    // now cleanup all the dummy entries again
    file_desc = ft_get_entry(0);
    if (l4_is_nil_id(file_desc.server_id))
        ft_free_entry(0);
    file_desc = ft_get_entry(1);
    if (l4_is_nil_id(file_desc.server_id))
        ft_free_entry(1);
}
L4C_CTOR(init_io, 2100);
