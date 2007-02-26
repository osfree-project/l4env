#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_ATTACHABLE_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_ATTACHABLE_H_

#include <l4/dietlibc/io-types.h>
#include <l4/sys/types.h>

int attach_namespace(l4_threadid_t server,
                     volume_id_t volume_id,
                     char * mounted_dir,
                     char * mount_dir);

int deattach_namespace(l4_threadid_t server,
                       char * mount_dir);

#endif
