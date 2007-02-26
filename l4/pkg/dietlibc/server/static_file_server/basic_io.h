#ifndef __DIETLIBC_SERVER_STATIC_FILE_SERVER_BASIC_IO_H_
#define __DIETLIBC_SERVER_STATIC_FILE_SERVER_BASIC_IO_H_

#include <l4/sys/types.h>
#include <l4/dietlibc/io-types.h>
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
int clientstate_open(int flags, l4_threadid_t client, local_object_id_t object_id);
int clientstate_close(int handle, l4_threadid_t client);
int clientstate_read(object_handle_t fd, l4_int8_t * buf, size_t count);
int clientstate_seek(object_handle_t fd, off_t offset, int whence);

#endif
