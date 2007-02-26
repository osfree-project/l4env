/**
 * \file   l4vfs/static_file_server/server/basic_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_STATIC_FILE_SERVER_SERVER_BASIC_IO_H_
#define __L4VFS_STATIC_FILE_SERVER_SERVER_BASIC_IO_H_

#include <l4/sys/types.h>
#include <l4/l4vfs/types.h>
#include <unistd.h>

#define MAX_CLIENTS 1024

typedef struct
{
    int               open;
    int               rw_mode;
    int               seek_pos;
    l4_threadid_t     client;
    local_object_id_t object_id;
} clientstate_t;

int get_free_clientstate(void);
void free_clientstate(int handle);
int clientstate_open(int flags, l4_threadid_t client,
                     local_object_id_t object_id);
int clientstate_close(int handle, l4_threadid_t client);
int clientstate_read(object_handle_t fd, l4_int8_t * buf, size_t count);
int clientstate_seek(object_handle_t fd, off_t offset, int whence);

#endif
