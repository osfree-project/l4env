/**
 * \file   l4vfs/name_server/server/clientstate.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_CLIENTSTATE_H_
#define __L4VFS_NAME_SERVER_SERVER_CLIENTSTATE_H_

#include <l4/l4vfs/types.h>
#include <l4/sys/types.h>

#include <sys/types.h>

#define MAX_CLIENTS 1024

typedef struct
{
    int               open;
    int               rw_mode;
    int               seek_pos;
    l4_threadid_t     client;
    local_object_id_t object_id;
    int               len;
} clientstate_t;

int clientstate_open(int mode, l4_threadid_t client, local_object_id_t object_id);
int clientstate_close(int handle, l4_threadid_t client);
int get_free_clientstate(void);
void free_clientstate(int handle);
int clientstate_check_access(int flags,
                             l4_threadid_t client,
                             local_object_id_t object_id);
int clientstate_getdents(int fd, l4vfs_dirent_t *dirp,
                         int count, l4_threadid_t client);
off_t clientstate_lseek(int fd, off_t offset,
                        int whence, l4_threadid_t client);

#endif
