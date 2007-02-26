/**
 * \file   dietlibc/lib/backends/io/fcntl.c
 * \brief  
 *
 * \date   09/13/2004
 * \author Carsten Weinhold  <cw183155@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

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
#include <l4/util/mbi_argv.h>   // for argc and argv[]
#include <l4/names/libnames.h>

int fcntl( int fd, int cmd, ... )
{
    file_desc_t fdesc;
    int ret;
    long arg;
    struct flock *lock;
    va_list list;

    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }

    fdesc = ft_get_entry(fd);

    if (l4_is_invalid_id(fdesc.server_id))
    { // should not happen
        errno = EBADF;
        return -1;
    }

    lock = NULL;

    va_start( list, cmd );
    switch( cmd )
    {
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
        lock = va_arg( list, struct flock * );
        break;
    case F_DUPFD:
    case F_SETFD:
    case F_SETFL:
    //case F_SETOWN:
    //case F_SETSIG:
    //case F_SETLEASE:
    //case F_NOTIFY:
        arg = va_arg( list, long );
        break;
    default:
        // no arg
        arg = 0;
        break;
    }
    va_end( list );

    // fixme: we don't need to call the server allways, e.g. for
    //        F_DUPFD which works locally completely! others?

    // now we can call the server
    if (lock)
    {
        // hmm. dice doesn't recognize struct flock definition in fcntl.h
        // ret = l4vfs_fcntl_flock(fdesc.server_id,
        //                         fdesc.object_handle,
        //                         cmd,
        //                         lock,
        //                         sizeof(*lock) );
        LOG("F_GETLK, F_SETLK, F_SETLKW not supported");
        ret = -EINVAL;
    }
    else
        ret = l4vfs_fcntl(fdesc.server_id,
                          fdesc.object_handle,
                          cmd,
                          (l4_int32_t *) &arg );
    return ret;
}
