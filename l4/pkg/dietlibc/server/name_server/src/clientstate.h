#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_CLIENTSTATE_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_CLIENTSTATE_H_

#include <l4/dietlibc/io-types.h>
#include <l4/sys/types.h>
#include <dirent.h>

#define MAX_CLIENTS 1024

typedef struct
{
    int               open;
    int               rw_mode;
    int               seek_pos;
    l4_threadid_t     client;
    local_object_id_t object_id;
    unsigned char *   data;
    int               len;
} clientstate_t;

int clientstate_open(int mode, l4_threadid_t client, local_object_id_t object_id);
int clientstate_close(int handle, l4_threadid_t client);
int get_free_clientstate(void);
void free_clientstate(int handle);
int clientstate_check_access(int flags,
                             l4_threadid_t client,
                             local_object_id_t object_id);
int clientstate_getdents(int fd, struct dirent *dirp,
                         int count, l4_threadid_t client);
off_t clientstate_lseek(int fd, off_t offset,
                        int whence, l4_threadid_t client);

#endif
