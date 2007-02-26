#ifndef __RT_MON_SERVER_L4VFS_COORD_CLIENTS_H_
#define __RT_MON_SERVER_L4VFS_COORD_CLIENTS_H_

#include <l4/dm_generic/types.h>
#include <l4/l4vfs/tree_helper.h>
#include <l4/l4vfs/types.h>
#include <l4/rt_mon/types.h>

#define MAX_CLIENTS 1024

typedef struct file_s
{
    l4dm_dataspace_t ds;
    int              next_instance;
    int              shared;        // flag whether this is a shared sensor
} file_t;

typedef struct client_s
{
    int               open;
    int               mode;
    l4vfs_size_t      seek;
    l4_threadid_t     client;
    l4vfs_th_node_t * node;
} client_t;

extern client_t clients[MAX_CLIENTS];

int  client_get_free(void);
int  client_is_open(int handle);
void init_clients(void);

#endif
