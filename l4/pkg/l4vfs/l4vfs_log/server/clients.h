/**
 * \file   l4vfs/l4vfs_log/server/clients.h
 * \brief  
 *
 * \date   11/05/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_L4VFS_LOG_SERVER_CLIENTS_H_
#define __L4VFS_L4VFS_LOG_SERVER_CLIENTS_H_

#include <l4/dm_generic/types.h>
#include <l4/l4vfs/tree_helper.h>
#include <l4/l4vfs/types.h>

#define MAX_CLIENTS 1024

typedef struct client_s
{
    int               open;
    l4_threadid_t     client;
    l4vfs_th_node_t * node;
    l4vfs_size_t      seek;
} client_t;

extern client_t clients[MAX_CLIENTS];

int  client_get_free(void);
int  client_is_open(int handle);
void init_clients(void);

#endif
