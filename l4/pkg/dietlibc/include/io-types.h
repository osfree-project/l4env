#ifndef _IO_TYPES_H_
#define _IO_TYPES_H_

#include <l4/sys/types.h>

#define MAX_FILES_OPEN 1024
#define true 1
#define false 0
#define L4_IO_ILLEGAL_OBJECT_NAME_CHARS "/:"  // and \0 of course
#define NAME_SERVER_VOLUME_ID 0
#define ROOT_OBJECT_ID 0
#define MAX_NAME 255

typedef l4_int32_t  volume_id_t;
typedef l4_int32_t  local_object_id_t;
typedef l4_int32_t object_handle_t;

#define L4_IO_ILLEGAL_OBJECT_ID -1
#define L4_IO_ILLEGAL_VOLUME_ID -1

typedef struct
{
    l4_threadid_t   server_id;
    object_handle_t object_handle;
} file_desc_t;

/**
 * Global object ids comprises a volume_id and an object_id
 * it defines exactly one object in the namespace (an object might be
 * a directory too)
 *
 * It seems logical to define object_id == 0 as root object_id.
 */
typedef struct
{
    volume_id_t       volume_id;
    local_object_id_t object_id;
} object_id_t;

#endif
