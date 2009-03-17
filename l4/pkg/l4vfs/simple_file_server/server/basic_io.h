/**
 * \file   l4vfs/simple_file_server/server/basic_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_SIMPLE_FILE_SERVER_SERVER_BASIC_IO_H_
#define __L4VFS_SIMPLE_FILE_SERVER_SERVER_BASIC_IO_H_

#include <l4/sys/types.h>
#include <l4/l4vfs/types.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_CLIENTS 1024

typedef struct
{
    int               open;
    int               rw_mode;
    int               seek_pos;  // bytes for files, entries for dirs
    l4_threadid_t     client;
    local_object_id_t object_id;
} clientstate_t;

int get_free_clientstate(void);
void free_clientstate(int handle);
int clientstate_open(int flags, l4_threadid_t client, local_object_id_t object_id);
int clientstate_close(int handle, l4_threadid_t client);
int clientstate_access(int mode, l4_threadid_t client, local_object_id_t object_id);
int clientstate_stat(l4vfs_stat_t *buf, l4_threadid_t client, local_object_id_t object_id);
int clientstate_read(object_handle_t fd, l4_int8_t * buf, size_t count);
int clientstate_write(object_handle_t fd, const l4_int8_t * buf, size_t count);
int clientstate_seek(object_handle_t fd, off_t offset, int whence);
int clientstate_getdents(int fd, l4vfs_dirent_t *dirp,int count, l4_threadid_t client);
int clientstate_mmap(l4dm_dataspace_t *ds, size_t length, int prot, unsigned flags, object_handle_t fd, off_t offset);
int clientstate_msync(const l4dm_dataspace_t *ds, l4_addr_t start, size_t length, int flags);
int clientstate_munmap(const l4dm_dataspace_t *ds, l4_addr_t start, size_t length);
int clientstate_select_notify(object_handle_t *fd, int *mode);

#endif
