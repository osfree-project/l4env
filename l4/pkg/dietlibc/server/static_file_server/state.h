#ifndef __DIETLIBC_SERVER_STATIC_FILE_SERVER_STATE_H_
#define __DIETLIBC_SERVER_STATIC_FILE_SERVER_STATE_H_

#include <l4/sys/types.h>

#define MAX_STATIC_FILES 32
#define STATIC_FILE_SERVER_VOLUME_ID 56
/* Object-id 0 is reserved for the root dir
 * 1 .. MAX_STATIC_FILES for the files
 */

typedef struct
{
    char * name;
    l4_uint8_t * data;
    int length;
} static_file_t;

extern static_file_t files[MAX_STATIC_FILES];

void state_init(void);

#endif
