#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_VOLUMES_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_VOLUMES_H_

#include <l4/dietlibc/io-types.h>

#define NAME_SERVER_MAX_VOLUMES 32

typedef struct
{
    volume_id_t   volume_id;
    l4_threadid_t server_id;
} volume_entry_t;

extern volume_entry_t volume_entries[NAME_SERVER_MAX_VOLUMES];

void init_volume_entries(void);
l4_threadid_t server_for_volume(volume_id_t volume_id);
int index_for_volume(volume_id_t volume_id);
int insert_volume_server(volume_id_t volume_id, l4_threadid_t server_id);
int remove_volume_server(volume_id_t volume_id, l4_threadid_t server_id);
int first_empty_volume_entry(void);
int is_own_volume(volume_id_t volume_id);

#endif
