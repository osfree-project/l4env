/**
 * \file   l4vfs/include/common_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_COMMON_IO_H_
#define __L4VFS_INCLUDE_COMMON_IO_H_

#include <sys/types.h> /* for ssize_t */
#include <l4/l4vfs/types.h>
#include <l4/l4vfs/common_io-client.h>

EXTERN_C_BEGIN

ssize_t l4vfs_read(l4_threadid_t server,
                   object_handle_t fd,
                   l4_int8_t **buf,
                   size_t *count);

ssize_t l4vfs_write(l4_threadid_t server,
                    object_handle_t fd,
                    const l4_int8_t *buf,
                    size_t *count);

int     l4vfs_close(l4_threadid_t server,
                    object_handle_t object_handle);

int     l4vfs_ioctl(l4_threadid_t server,
                    object_handle_t fd,
                    l4_int32_t cmd,
                    char **arg,
                    size_t *count);

int     l4vfs_fcntl(l4_threadid_t server,
                    object_handle_t fd,
                    l4_int32_t cmd,
                    l4_int32_t *arg);

EXTERN_C_END

#endif
