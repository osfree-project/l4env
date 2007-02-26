/**
 * \file   l4vfs/include/basic_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_BASIC_IO_H_
#define __L4VFS_INCLUDE_BASIC_IO_H_

#include <l4/sys/compiler.h>
#include <l4/l4vfs/types.h>
#include <l4/l4vfs/common_io.h>
#include <l4/l4vfs/basic_io-client.h>

#include <dirent.h>

EXTERN_C_BEGIN

object_handle_t l4vfs_open(l4_threadid_t server,
                           object_id_t object_id,
                           l4_int32_t flags);

object_handle_t l4vfs_creat(l4_threadid_t server,
                            object_id_t parent,
                            const char* name,
                            l4_int32_t flags,
                            mode_t mode);

off_t           l4vfs_lseek(l4_threadid_t server,
                            object_handle_t fd,
                            off_t offset,
                            l4_int32_t whence);

l4_int32_t      l4vfs_fsync(l4_threadid_t server,
                            object_handle_t fd);

l4_int32_t      l4vfs_getdents(l4_threadid_t server,
                               object_handle_t fd,
                               l4vfs_dirent_t *dirp,
                               l4_uint32_t count);

l4_int32_t      l4vfs_stat(l4_threadid_t server,
                           object_id_t object_id,
                           l4vfs_stat_t * buf);

l4_int32_t      l4vfs_access(l4_threadid_t server,
                             object_id_t object_id,
                             l4_int32_t mode);

l4_int32_t      l4vfs_unlink(l4_threadid_t server,
                             object_id_t object_id);

EXTERN_C_END

#endif
