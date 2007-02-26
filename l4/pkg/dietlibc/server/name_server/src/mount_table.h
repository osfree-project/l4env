#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_MOUNT_TABLE_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_MOUNT_TABLE_H_

#define NAME_SERVER_MAX_MOUNTS 32

#include <l4/dietlibc/io-types.h>

typedef struct
{
    object_id_t mount_point;
    object_id_t dest;
} mount_table_entry_t;

extern mount_table_entry_t mount_table[NAME_SERVER_MAX_MOUNTS];

void init_mount_table(void);
int mount_insert_entry(object_id_t mount_point, object_id_t dest);
int find_empty_entry(void);
int mount_check_boundary_conditions(object_id_t mount_point, object_id_t dest);
int is_volume_mounted(volume_id_t volume_id);
object_id_t translate_mount_point(object_id_t mount_point);
object_id_t translate_mounted_point(object_id_t mounted_point);
int is_mount_point(object_id_t mount_point);
int is_mounted_point(object_id_t mount_point);
object_id_t get_mount_point(volume_id_t volume_id);
object_id_t get_mounted_point(volume_id_t volume_id);

#endif
